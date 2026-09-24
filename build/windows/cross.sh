#!/bin/sh
# The Windows kit: cross-compiled on Linux with MinGW-w64, upstream's own
# way (wmake/rules/linux64Mingw, etc-mingw/, bin/tools/createMingwRuntime
# describe it; this script follows them and packs the result as a kit).
# Entered from build/build.sh after the source tree and the ThirdParty pack
# are in place; not meant to be run by hand.
#
# What differs from the Linux and macOS builds:
#   - the compiler is x86_64-w64-mingw32-g++ (GCC, posix threads);
#   - MPI is MS-MPI, the node's own: the build links the parallel Pstream
#     against the SDK's headers and import library, which are fetched by
#     checksum and laid out where upstream's etc/config.sh/mpi looks
#     (ThirdParty/platforms/linux64Mingw/<msmpi>/{include,lib/x64}); nothing
#     of the SDK travels in the kit;
#   - pt-scotch is not built (upstream's makeSCOTCH skips it for MinGW), so
#     ptscotchDecomp is upstream's dummy on Windows, as in OpenCFD's build;
#   - the tree is named after the host (linux64MingwDPInt32Opt) and the kit
#     after the target (win64MingwDPInt32Opt), and every DLL is placed beside
#     the executables: that is where Windows looks first, before PATH.
set -e

: "${BUILD_DIR:?}" "${REPO_ROOT:?}" "${WORK:?}" "${DEPS_DIR:?}" "${SRC:?}" "${TP:?}" "${JOBS:?}"
. "$BUILD_DIR/lib.sh"
. "$BUILD_DIR/config.sh"
detect_platform
[ "$PLATFORM" = "windows-x64" ] || die "cross.sh is the Windows build (TARGET=windows-x64)"

need "$CROSS_PREFIX-gcc"
need "$CROSS_PREFIX-g++"
need "$CROSS_PREFIX-objdump"
need msiextract
need bison

# The thread model decides whether std::thread and std::mutex exist. Ubuntu's
# alternatives can point x86_64-w64-mingw32-g++ at the win32 variant; the
# build must not depend on which one a machine happens to select.
_tm=$("$CROSS_PREFIX-g++" -v 2>&1 | sed -n 's/^Thread model: //p')
[ "$_tm" = "posix" ] || die "$CROSS_PREFIX-g++ has thread model '$_tm', posix is required (install the -posix variant and select it)"
log "cross toolchain: $("$CROSS_PREFIX-g++" --version | head -1), thread model $_tm"

# ---------------------------------------------------------------------------
# 1. The MS-MPI SDK, laid out where upstream's environment finds it
# ---------------------------------------------------------------------------
#
# etc/config.sh/mpi resolves WM_MPLIB=msmpi-<ver> to
# ThirdParty/platforms/<arch><compiler>/msmpi-<ver> and requires lib/x64
# under it; wmake/rules/linux64Mingw/mplibMSMPI adds -I<that>/include and
# -L<that>/lib/x64 -lmsmpi. The SDK's msi unpacks to "Program Files/Microsoft
# SDKs/MPI/{Include,Lib/x64}" - capitalised, and this is a case-sensitive
# host, so the files are copied into lowercase directories. The import
# library is an MSVC-style archive of short import entries, which GNU ld
# reads as it is; it is offered under both names ld searches for -lmsmpi.

MSMPI_DIR="$TP/platforms/$TP_MPI_PLATFORM/$MSMPI_VERSION"
if [ ! -f "$MSMPI_DIR/include/mpi.h" ] || [ ! -f "$MSMPI_DIR/lib/x64/msmpi.lib" ]; then
    fetch_verify "$MSMPI_SDK_URL" "$MSMPI_SDK_SHA256" "$DEPS_DIR/msmpisdk.msi"
    log "unpacking the MS-MPI SDK into ThirdParty/platforms/$TP_MPI_PLATFORM/$MSMPI_VERSION"
    _x="$WORK/msmpisdk.extract"
    rm -rf "$_x" "$MSMPI_DIR"
    mkdir -p "$_x" "$MSMPI_DIR/include" "$MSMPI_DIR/lib/x64"
    (cd "$_x" && msiextract "$DEPS_DIR/msmpisdk.msi" >/dev/null)
    _sdk=$(find "$_x" -type d -name MPI -path '*Microsoft SDKs*' | head -1)
    [ -n "$_sdk" ] || die "the SDK msi did not unpack to a 'Microsoft SDKs/MPI' directory"
    cp "$_sdk"/Include/*.h "$MSMPI_DIR/include/"
    mkdir -p "$MSMPI_DIR/include/x64" && cp "$_sdk"/Include/x64/*.h "$MSMPI_DIR/include/x64/"
    cp "$_sdk"/Lib/x64/msmpi.lib "$MSMPI_DIR/lib/x64/msmpi.lib"
    cp "$_sdk"/Lib/x64/msmpi.lib "$MSMPI_DIR/lib/x64/libmsmpi.a"
    mkdir -p "$MSMPI_DIR/License" && cp "$_sdk"/License/* "$MSMPI_DIR/License/"
    rm -rf "$_x"
fi
expect_in_file "$MSMPI_DIR/include/mpi.h" "MSMPI_VER" "the MS-MPI header"

# ---------------------------------------------------------------------------
# 2. Configure the build copy: upstream's tool, then upstream's own overlay
# ---------------------------------------------------------------------------
#
# etc-mingw/ is upstream's configuration for exactly this build: prefs.sh
# selects the Mingw compiler and an msmpi WM_MPLIB, config.sh/CGAL turns CGAL
# and boost off, config.sh/scotch points at the cross-built scotch. It is
# activated by FOAM_CONFIG_ETC=etc-mingw before etc/bashrc is sourced. Two
# adjustments, guarded like the ThirdParty ones (patches/README.md):
#   - prefs.sh names msmpi-10.0; the pinned SDK is newer, and the version in
#     WM_MPLIB is the directory name the environment resolves;
#   - config.sh/FFTW.system points at openSUSE's MinGW fftw package, which
#     this host does not have: fftw is cross-built from the pack instead, and
#     the default etc/config.sh/FFTW (ThirdParty layout) applies, so the
#     .system variant is removed from the overlay.

log "configuring (foamConfigurePaths)"
( cd "$SRC" && bash bin/tools/foamConfigurePaths \
    -system-compiler "$WM_COMPILER" \
    -mpi="$MSMPI_VERSION" \
    -DP -int32 \
    -scotch "$SCOTCH_VERSION" \
    -kahip "$KAHIP_VERSION" \
    -fftw "$FFTW_VERSION" \
    -metis "$METIS_VERSION" \
    -cgal "$CGAL_VERSION" \
    -boost "$BOOST_VERSION" \
    -adios "$ADIOS2_VERSION" \
    -hdf5 "$HDF5_VERSION" )

expect_in_file "$SRC/etc/bashrc"          "^export WM_MPLIB=$MSMPI_VERSION"     "WM_MPLIB=$MSMPI_VERSION"
expect_in_file "$SRC/etc/bashrc"          "^export WM_COMPILER=$WM_COMPILER"    "WM_COMPILER=$WM_COMPILER"
expect_in_file "$SRC/etc/bashrc"          "^export WM_LABEL_SIZE=32"            "WM_LABEL_SIZE=32"
expect_in_file "$SRC/etc/bashrc"          "^export WM_PRECISION_OPTION=DP"      "WM_PRECISION_OPTION=DP"
expect_in_file "$SRC/etc/config.sh/scotch" "SCOTCH_VERSION=$SCOTCH_VERSION"     "SCOTCH_VERSION"
expect_in_file "$SRC/etc/config.sh/kahip" "KAHIP_VERSION=$KAHIP_VERSION"        "KAHIP_VERSION"
expect_in_file "$SRC/etc/config.sh/FFTW"  "fftw_version=$FFTW_VERSION"          "fftw_version"
expect_in_file "$SRC/etc/config.sh/CGAL"  "cgal_version=$CGAL_VERSION"          "cgal_version"

_prefs="$SRC/etc-mingw/prefs.sh"
if ! grep -q "^export WM_MPLIB=$MSMPI_VERSION" "$_prefs"; then
    expect_in_file "$_prefs" "^export WM_MPLIB=msmpi-" "the msmpi WM_MPLIB line of etc-mingw/prefs.sh"
    sed -i "s|^export WM_MPLIB=msmpi-.*|export WM_MPLIB=$MSMPI_VERSION|" "$_prefs"
    expect_in_file "$_prefs" "^export WM_MPLIB=$MSMPI_VERSION" "the adjusted WM_MPLIB"
    log "etc-mingw/prefs.sh: WM_MPLIB=$MSMPI_VERSION"
fi
expect_in_file "$_prefs" "^export WM_COMPILER=Mingw" "WM_COMPILER=Mingw in etc-mingw/prefs.sh"
# No switch jump tables (patches/README.md). GCC 13 for x86_64-w64-mingw32
# emits the jump table of a switch inside an inline (COMDAT) function into
# the plain .rdata section, addressed by a 32-bit section-relative
# relocation into that function's .text$ section. Upstream partially links
# OSspecific and Pstream into single objects (ld -r), which merges the
# duplicated inline functions and discards all but one copy - and a jump
# table that pointed at a discarded copy is left dangling: "relocation
# truncated to fit: IMAGE_REL_AMD64_REL32 against .text$_ZN4Foam5token5resetEv"
# when libOpenFOAM.dll is linked (second cross-build, 2026-09-24). Without
# jump tables a switch compiles to compares and branches; not measurable on
# OpenFOAM's hot paths, which are loops over fields. Set in prefs.sh, not in
# the environment: etc/bashrc clears FOAM_EXTRA_CXXFLAGS before prefs.sh runs.
if ! grep -q -- "-fno-jump-tables" "$_prefs"; then
    printf '\n# OmnibusCloud build: see build/windows/cross.sh\nexport FOAM_EXTRA_CXXFLAGS="${FOAM_EXTRA_CXXFLAGS} -fno-jump-tables"\n' >> "$_prefs"
    expect_in_file "$_prefs" "fno-jump-tables" "the jump-table flag in etc-mingw/prefs.sh"
    log "etc-mingw/prefs.sh: FOAM_EXTRA_CXXFLAGS += -fno-jump-tables"
fi
if [ -f "$SRC/etc-mingw/config.sh/FFTW.system" ]; then
    rm -f "$SRC/etc-mingw/config.sh/FFTW.system"
    log "etc-mingw/config.sh/FFTW.system removed: fftw is cross-built from the pack"
fi
expect_in_file "$SRC/etc-mingw/config.sh/CGAL" "cgal_version=CGAL-none" "CGAL off in the mingw overlay"

# ---------------------------------------------------------------------------
# 3. Allwmake, under bash, with a HOME of its own - one stage
# ---------------------------------------------------------------------------
#
# One stage, unlike the Linux and macOS builds: there is no MPI to build
# first, the MS-MPI directory exists before the environment is sourced.

BUILD_HOME="$WORK/home"
mkdir -p "$BUILD_HOME"

cat > "$WORK/cross-all.sh" <<EOF
#!/bin/bash
set -e
export HOME='$BUILD_HOME'
export WM_NCOMPPROCS='$JOBS'
unset WM_PROJECT_DIR WM_PROJECT_SITE FOAM_CONFIG_MODE
export FOAM_CONFIG_ETC=etc-mingw
echo "==> sourcing $SRC/etc/bashrc with FOAM_CONFIG_ETC=etc-mingw (bash \$BASH_VERSION)"
set +e
source '$SRC/etc/bashrc'
set -e
[ "\$WM_PROJECT_DIR" = '$SRC' ] || { echo "WM_PROJECT_DIR is '\$WM_PROJECT_DIR', expected '$SRC'" >&2; exit 1; }
[ "\$WM_OPTIONS" = '$WM_OPTIONS_EXPECTED' ] || { echo "WM_OPTIONS is '\$WM_OPTIONS', expected '$WM_OPTIONS_EXPECTED'" >&2; exit 1; }
[ "\$WM_THIRD_PARTY_DIR" = '$TP' ] || { echo "WM_THIRD_PARTY_DIR is '\$WM_THIRD_PARTY_DIR', expected '$TP'" >&2; exit 1; }
[ "\$WM_OSTYPE" = 'MSwindows' ] || { echo "WM_OSTYPE is '\$WM_OSTYPE', expected MSwindows" >&2; exit 1; }
[ "\$FOAM_MPI" = '$MSMPI_VERSION' ] || { echo "FOAM_MPI is '\$FOAM_MPI', expected '$MSMPI_VERSION' (MS-MPI not resolved: is $MSMPI_DIR/lib/x64 there?)" >&2; exit 1; }
[ "\$MPI_ARCH_PATH" = '$MSMPI_DIR' ] || { echo "MPI_ARCH_PATH is '\$MPI_ARCH_PATH', expected '$MSMPI_DIR'" >&2; exit 1; }
[ "\$(wmake -show-path-cxx)" = "\$(command -v $CROSS_PREFIX-g++)" ] || { echo "wmake would use '\$(wmake -show-path-cxx)', not $CROSS_PREFIX-g++" >&2; exit 1; }
cd "\$WM_PROJECT_DIR"
# No -q here, unlike the Linux and macOS builds: queue mode collects the
# compilations and runs them at the end, but the MinGW path in src/Allwmake
# links Pstream as one object (wmake libo) right away, before its sources
# have been compiled - the first cross-build died there in three minutes
# ("cannot find .../Pstream/dummy/UPstream.o").
echo "==> Allwmake -j $JOBS -s -l -prefix=none  (WM_OPTIONS=\$WM_OPTIONS, FOAM_MPI=\$FOAM_MPI, cxx=\$(wmake -show-path-cxx))"
./Allwmake -j '$JOBS' -s -l -prefix=none
EOF
chmod +x "$WORK/cross-all.sh"

_t0=$(date +%s)
log "ThirdParty (scotch, fftw) and OpenFOAM  (log: $SRC/log.$WM_OPTIONS_EXPECTED)"
bash "$WORK/cross-all.sh"
_t1=$(date +%s)
log "Allwmake finished in $(( (_t1 - _t0) / 60 )) min"

# ---------------------------------------------------------------------------
# 4. What must exist before packing is even attempted
# ---------------------------------------------------------------------------

BIN="$SRC/platforms/$WM_OPTIONS_EXPECTED/bin"
LIB="$SRC/platforms/$WM_OPTIONS_EXPECTED/lib"
for _app in simpleFoam pimpleFoam pisoFoam interFoam rhoSimpleFoam buoyantSimpleFoam \
            blockMesh snappyHexMesh surfaceFeatureExtract setFields mapFields topoSet createPatch \
            decomposePar reconstructPar postProcess foamDictionary checkMesh renumberMesh transformPoints; do
    [ -f "$BIN/$_app.exe" ] || die "missing after the build: platforms/$WM_OPTIONS_EXPECTED/bin/$_app.exe"
done
[ -f "$LIB/libOpenFOAM.dll" ]                 || die "missing: libOpenFOAM.dll"
[ -f "$LIB/dummy/libPstream.dll" ]            || die "missing: lib/dummy/libPstream.dll (the serial Pstream)"
[ -f "$LIB/$MSMPI_VERSION/libPstream.dll" ]   || die "missing: lib/$MSMPI_VERSION/libPstream.dll (the MS-MPI Pstream)"

# The ThirdParty libraries must be the real ones (see build.sh for why
# presence of libscotchDecomp proves nothing).
imports() { "$CROSS_PREFIX-objdump" -p "$1" 2>/dev/null | sed -n 's/^\s*DLL Name: //p'; }
_scotch=$(find "$TP/platforms" -name 'libscotch*.dll' | head -1)
[ -n "$_scotch" ] || die "missing: ThirdParty scotch (did not build)"
_fftw=$(find "$TP/platforms" -name 'libfftw3*.dll' | head -1)
[ -n "$_fftw" ] || die "missing: ThirdParty fftw (did not build)"
imports "$LIB/libscotchDecomp.dll" | grep -q -i '^libscotch' || die "libscotchDecomp.dll does not import libscotch - it is the dummy"
imports "$LIB/$MSMPI_VERSION/libPstream.dll" | grep -q -i '^msmpi\.dll$' || die "lib/$MSMPI_VERSION/libPstream.dll does not import msmpi.dll"
imports "$LIB/dummy/libPstream.dll" | grep -q -i '^msmpi\.dll$' && die "lib/dummy/libPstream.dll imports msmpi.dll - it is not the serial one"
log "checks: executables, both Pstreams, scotch ($(basename "$_scotch")), fftw ($(basename "$_fftw"))"

sh "$BUILD_DIR/windows/pack.sh"

log "done"
