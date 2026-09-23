#!/bin/sh
# Build the OpenFOAM kit for the host platform (Linux or macOS).
#
#   sh build/build.sh                  build under .build/, kit in .build/out
#   JOBS=8 sh build/build.sh           parallel width (default: every core)
#   FORCE=1 sh build/build.sh          wipe the build tree first
#   DEPS_DIR=/dl sh build/build.sh     where the pinned downloads live
#
# The build never touches upstream/: it works on a copy. Configuration goes
# through upstream's own foamConfigurePaths on that copy, so the etc/ the kit
# carries says exactly what the kit was built with.
set -e

BUILD_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$BUILD_DIR/.." && pwd)
WORK="${WORK:-$REPO_ROOT/.build}"
DEPS_DIR="${DEPS_DIR:-$WORK/deps}"
export BUILD_DIR REPO_ROOT WORK DEPS_DIR

. "$BUILD_DIR/lib.sh"
. "$BUILD_DIR/config.sh"

need curl
need tar
need make
need bash
need flex
need cmake

detect_platform
JOBS="${JOBS:-$(cpu_count)}"
SRC="$WORK/$OPENFOAM_DIR"
TP="$WORK/$THIRDPARTY_DIR"

log "OpenFOAM $OPENFOAM_VERSION for $PLATFORM ($WM_OPTIONS_EXPECTED), $JOBS jobs"
log "  MPI      $OPENMPI_VERSION (ThirdParty, bundled)"
log "  scotch   $SCOTCH_VERSION   kahip $KAHIP_VERSION   fftw $FFTW_VERSION"
log "  CGAL/boost, ADIOS2, HDF5, METIS: off (build/config.sh)"

mkdir -p "$DEPS_DIR" "$WORK"

if [ "${FORCE:-0}" = "1" ]; then
    log "FORCE=1: removing the build tree"
    rm -rf "$SRC" "$TP"
fi

# ---------------------------------------------------------------------------
# 1. The source tree
# ---------------------------------------------------------------------------
#
# Copied from upstream/ - unless this checkout does not carry it as upstream
# shipped it (a Windows sparse checkout leaves it out entirely; NTFS cannot
# hold its links, case-colliding names and colon-named files, see README.md),
# in which case the pinned source pack is unpacked instead: the same bytes,
# verified by checksum (PROVENANCE.md). Building from a mangled tree would not
# fail loudly, it would fail strangely, so the choice is made here and said
# out loud.

UPSTREAM="$REPO_ROOT/upstream/$OPENFOAM_DIR"
if [ ! -d "$SRC" ]; then
    if [ -L "$UPSTREAM/wmake/scripts/wclean-build" ]; then
        log "copying upstream/$OPENFOAM_DIR into the build area"
        cp -R "$UPSTREAM" "$SRC"
    else
        warn "upstream/ is absent or carries no symbolic links (a Windows checkout) - unpacking the pinned source pack instead"
        fetch_verify "$OPENFOAM_SRC_URL" "$OPENFOAM_SRC_SHA256" "$DEPS_DIR/$OPENFOAM_DIR.tgz"
        tar -xzf "$DEPS_DIR/$OPENFOAM_DIR.tgz" -C "$WORK"
    fi
fi
[ -f "$SRC/etc/bashrc" ] || die "no etc/bashrc under $SRC"
[ -L "$SRC/wmake/scripts/wclean-build" ] || die "the build copy has no symbolic links - it must come from a Unix checkout or the source pack"

# ---------------------------------------------------------------------------
# 2. ThirdParty, as a sibling: upstream's bashrc derives WM_THIRD_PARTY_DIR
#    from the project directory and the version.
# ---------------------------------------------------------------------------

if [ ! -d "$TP" ]; then
    fetch_verify "$THIRDPARTY_URL" "$THIRDPARTY_SHA256" "$DEPS_DIR/$THIRDPARTY_DIR.tar.gz"
    log "unpacking $THIRDPARTY_DIR"
    tar -xzf "$DEPS_DIR/$THIRDPARTY_DIR.tar.gz" -C "$WORK"
fi
[ -x "$TP/Allwmake" ] || die "no Allwmake under $TP"
for _c in "$OPENMPI_VERSION" "$SCOTCH_VERSION" "$KAHIP_VERSION" "$FFTW_VERSION"; do
    [ -d "$TP/sources/$(echo "$_c" | sed 's/[-_].*//' | tr 'A-Z' 'a-z')/$_c" ] \
        || die "ThirdParty pack carries no sources/*/$_c - the pin in build/config.sh does not match the pack"
done

# ---------------------------------------------------------------------------
# 3. Configure the build copy with upstream's own tool
# ---------------------------------------------------------------------------

log "configuring (foamConfigurePaths)"
( cd "$SRC" && bash bin/tools/foamConfigurePaths \
    -system-compiler "$WM_COMPILER" \
    -openmpi="$OPENMPI_VERSION" \
    -DP -int32 \
    -scotch "$SCOTCH_VERSION" \
    -kahip "$KAHIP_VERSION" \
    -fftw "$FFTW_VERSION" \
    -metis "$METIS_VERSION" \
    -cgal "$CGAL_VERSION" \
    -boost "$BOOST_VERSION" \
    -adios "$ADIOS2_VERSION" \
    -hdf5 "$HDF5_VERSION" )

expect_in_file "$SRC/etc/bashrc"          "^export WM_MPLIB=OPENMPI"            "WM_MPLIB=OPENMPI"
expect_in_file "$SRC/etc/bashrc"          "^export WM_COMPILER=$WM_COMPILER"    "WM_COMPILER=$WM_COMPILER"
expect_in_file "$SRC/etc/bashrc"          "^export WM_LABEL_SIZE=32"            "WM_LABEL_SIZE=32"
expect_in_file "$SRC/etc/bashrc"          "^export WM_PRECISION_OPTION=DP"      "WM_PRECISION_OPTION=DP"
expect_in_file "$SRC/etc/config.sh/mpi"   "FOAM_MPI=$OPENMPI_VERSION"           "FOAM_MPI=$OPENMPI_VERSION"
expect_in_file "$SRC/etc/config.sh/scotch" "SCOTCH_VERSION=$SCOTCH_VERSION"     "SCOTCH_VERSION"
expect_in_file "$SRC/etc/config.sh/kahip" "KAHIP_VERSION=$KAHIP_VERSION"        "KAHIP_VERSION"
expect_in_file "$SRC/etc/config.sh/FFTW"  "fftw_version=$FFTW_VERSION"          "fftw_version"
expect_in_file "$SRC/etc/config.sh/CGAL"  "cgal_version=$CGAL_VERSION"          "cgal_version"
expect_in_file "$SRC/etc/config.sh/CGAL"  "boost_version=$BOOST_VERSION"        "boost_version"
expect_in_file "$SRC/etc/config.sh/adios2" "adios2_version=$ADIOS2_VERSION"     "adios2_version"
expect_in_file "$SRC/etc/config.sh/hdf5"  "hdf5_version=$HDF5_VERSION"          "hdf5_version"
expect_in_file "$SRC/etc/config.sh/metis" "METIS_VERSION=$METIS_VERSION"        "METIS_VERSION"

# ---------------------------------------------------------------------------
# 4. Allwmake, under bash, with a HOME of its own - in two stages
# ---------------------------------------------------------------------------
#
# Sourcing etc/bashrc is the one step that needs bash (BASH_SOURCE locates
# the project). HOME points at a build-private directory so that no
# ~/.OpenFOAM/prefs.sh of whoever runs this can change what gets built.
#
# Two stages because of how upstream's environment works: etc/bashrc adds a
# library directory to LD_LIBRARY_PATH only if it exists at the moment of
# sourcing. With a ThirdParty MPI that directory does not exist until
# makeOPENMPI has run, so a single-shell Allwmake builds Open MPI and then
# fails to load it: pt-scotch's mpicc dies with "libopen-pal.so.40: cannot
# open shared object file" (found on the first build, 2026-09-23). Upstream's
# own advice is "open a new shell and source the environment again"; the
# second stage is that new shell.

BUILD_HOME="$WORK/home"
mkdir -p "$BUILD_HOME"

# errexit is switched OFF around the source: inside a file sourced as part of
# an || list, bash 5 suspends -e but bash 3.2 (macOS's /bin/bash) does not,
# and the first failing probe inside etc/bashrc then kills the shell before
# a word is printed - the first macOS run ended exactly so, silently.
_env_prologue="#!/bin/bash
set -e
export HOME='$BUILD_HOME'
export WM_NCOMPPROCS='$JOBS'
unset WM_PROJECT_DIR WM_PROJECT_SITE FOAM_CONFIG_ETC FOAM_CONFIG_MODE
echo \"==> sourcing $SRC/etc/bashrc (bash \$BASH_VERSION)\"
set +e
source '$SRC/etc/bashrc'
set -e
[ \"\$WM_PROJECT_DIR\" = '$SRC' ] || { echo \"WM_PROJECT_DIR is '\$WM_PROJECT_DIR', expected '$SRC'\" >&2; exit 1; }
[ \"\$WM_OPTIONS\" = '$WM_OPTIONS_EXPECTED' ] || { echo \"WM_OPTIONS is '\$WM_OPTIONS', expected '$WM_OPTIONS_EXPECTED'\" >&2; exit 1; }
[ \"\$WM_THIRD_PARTY_DIR\" = '$TP' ] || { echo \"WM_THIRD_PARTY_DIR is '\$WM_THIRD_PARTY_DIR', expected '$TP'\" >&2; exit 1; }
[ \"\$FOAM_MPI\" = '$OPENMPI_VERSION' ] || { echo \"FOAM_MPI is '\$FOAM_MPI', expected '$OPENMPI_VERSION'\" >&2; exit 1; }
"

cat > "$WORK/stage1-mpi.sh" <<EOF
$_env_prologue
cd "\$WM_THIRD_PARTY_DIR"
echo "==> stage 1: Open MPI (\$FOAM_MPI -> \$MPI_ARCH_PATH)"
./makeOPENMPI -test "\$MPI_ARCH_PATH" || ./makeOPENMPI
[ -x "\$MPI_ARCH_PATH/bin/mpicc" ] || { echo "makeOPENMPI left no mpicc under \$MPI_ARCH_PATH" >&2; exit 1; }
EOF

cat > "$WORK/stage2-all.sh" <<EOF
$_env_prologue
case ":\$LD_LIBRARY_PATH:\$DYLD_LIBRARY_PATH:\$FOAM_LD_LIBRARY_PATH:" in
    *":\$MPI_ARCH_PATH/lib:"*|*":\$MPI_ARCH_PATH/lib64:"*) ;;
    *) echo "the re-sourced environment still does not see \$MPI_ARCH_PATH/lib - stage 1 did not leave an MPI behind" >&2; exit 1 ;;
esac
cd "\$WM_PROJECT_DIR"
# -prefix=none: the top-level Allwmake also builds modules/ (the pack ships
# the submodules checked out: OpenQBMM, adios, external-solver, ...) unless
# FOAM_MODULE_PREFIX is none or false. The kit is the core; the modules are
# not built, not shipped, and the first run without this flag ended in a
# wall of OpenQBMM link errors after the core had finished.
echo "==> stage 2: Allwmake -j $JOBS -s -q -l -prefix=none  (WM_OPTIONS=\$WM_OPTIONS, FOAM_MPI=\$FOAM_MPI)"
./Allwmake -j '$JOBS' -s -q -l -prefix=none
EOF
chmod +x "$WORK/stage1-mpi.sh" "$WORK/stage2-all.sh"

_t0=$(date +%s)
log "stage 1: MPI"
bash "$WORK/stage1-mpi.sh"
log "stage 2: ThirdParty (scotch, kahip, fftw) and OpenFOAM  (log: $SRC/log.$WM_OPTIONS_EXPECTED)"
bash "$WORK/stage2-all.sh"
_t1=$(date +%s)
log "Allwmake finished in $(( (_t1 - _t0) / 60 )) min"

# ---------------------------------------------------------------------------
# 5. What must exist before packing is even attempted
# ---------------------------------------------------------------------------

BIN="$SRC/platforms/$WM_OPTIONS_EXPECTED/bin"
LIB="$SRC/platforms/$WM_OPTIONS_EXPECTED/lib"
for _app in simpleFoam pimpleFoam pisoFoam interFoam rhoSimpleFoam buoyantSimpleFoam \
            blockMesh snappyHexMesh surfaceFeatureExtract setFields mapFields topoSet createPatch \
            decomposePar reconstructPar postProcess foamDictionary checkMesh renumberMesh transformPoints; do
    [ -x "$BIN/$_app" ] || die "missing after the build: platforms/$WM_OPTIONS_EXPECTED/bin/$_app"
done
[ -f "$LIB/libOpenFOAM.$SO_EXT" ]                      || die "missing: libOpenFOAM.$SO_EXT"
[ -f "$LIB/$OPENMPI_VERSION/libPstream.$SO_EXT" ]      || die "missing: lib/$OPENMPI_VERSION/libPstream.$SO_EXT (the MPI Pstream)"
[ -x "$TP/platforms/$TP_MPI_PLATFORM/$OPENMPI_VERSION/bin/mpirun" ] || die "missing: ThirdParty $OPENMPI_VERSION/bin/mpirun"

# The ThirdParty libraries must be the real ones. A failed makeSCOTCH is only
# a warning in upstream's Allwmake, and OpenFOAM then links its dummy
# scotchDecomp, which exists and loads and refuses every decomposition at run
# time - so presence of libscotchDecomp proves nothing; what it links does.
#
# Where ThirdParty puts them (v2606): the label-size-dependent libraries go
# into one shared directory, named with the label suffix -
#   platforms/<arch><compiler><prec><label>/lib/libscotch-int32.so
#   platforms/<arch><compiler><prec><label>/lib/libkahip-int32.so
#   platforms/<arch><compiler><prec><label>/lib/<mpi>/libptscotch-int32.so
# and the label-independent ones under their own prefix -
#   platforms/<arch><compiler>/fftw-3.3.10/lib/libfftw3.so.3.*
have_lib() {   # have_lib <what> <glob>...
    _what="$1"; shift
    for _g in "$@"; do
        for _f in $_g; do [ -f "$_f" ] && return 0; done
    done
    die "missing: $_what (looked for: $*)"
}
TPLIB="$TP/platforms/$TP_LIB_PLATFORM/lib"
have_lib "ThirdParty scotch (did not build)"    "$TPLIB/libscotch*.$SO_EXT"
have_lib "ThirdParty pt-scotch (did not build)" "$TPLIB/$OPENMPI_VERSION/libptscotch*.$SO_EXT"
have_lib "ThirdParty kahip (did not build)"     "$TPLIB/libkahip*.$SO_EXT"
have_lib "ThirdParty fftw (did not build)"      "$TP/platforms/$TP_MPI_PLATFORM/$FFTW_VERSION/lib/libfftw3.$SO_EXT*" "$TP/platforms/$TP_MPI_PLATFORM/$FFTW_VERSION/lib64/libfftw3.$SO_EXT*"
links_to() {   # links_to <binary> <library name fragment>
    case "$PLATFORM" in
        linux-x64)   readelf -d "$1" 2>/dev/null | grep NEEDED | grep -q "$2" ;;
        macos-arm64) otool -L "$1" 2>/dev/null | grep -q "$2" ;;
    esac
}
links_to "$LIB/libscotchDecomp.$SO_EXT" "libscotch"                       || die "libscotchDecomp.$SO_EXT does not link libscotch - it is the dummy"
links_to "$LIB/$OPENMPI_VERSION/libptscotchDecomp.$SO_EXT" "libptscotch"  || die "libptscotchDecomp.$SO_EXT does not link libptscotch - it is the dummy"
links_to "$LIB/libkahipDecomp.$SO_EXT" "libkahip"                         || die "libkahipDecomp.$SO_EXT does not link libkahip - it is the dummy"
links_to "$LIB/$OPENMPI_VERSION/libPstream.$SO_EXT" "libmpi"              || die "libPstream.$SO_EXT does not link libmpi"

sh "$BUILD_DIR/pack.sh"

log "done"
