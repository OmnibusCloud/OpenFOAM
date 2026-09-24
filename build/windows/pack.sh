#!/bin/sh
# Assemble the Windows kit from a finished cross-build and zip it.
# Entered from build/windows/cross.sh; TARGET=windows-x64 sh build/windows/pack.sh
# re-packs an existing build.
#
# The kit follows upstream's own runtime layout for Windows (createMingwRuntime):
# every executable and every DLL - OpenFOAM's, the MinGW runtime's, scotch's,
# fftw's - in one directory, platforms/win64MingwDPInt32Opt/bin, because that
# is where Windows looks for a DLL first, before PATH. There is no ThirdParty
# directory in this kit and no MPI: the parallel Pstream imports the node's
# msmpi.dll. Both Pstreams travel; libPstream.dll itself is the serial one, so
# a kit runs serially on any node, and the controller swaps the MS-MPI variant
# in once, on a node that has MS-MPI (KIT.env names all three files).
set -e

BUILD_DIR=$(cd "$(dirname "$0")/.." && pwd)
REPO_ROOT=$(cd "$BUILD_DIR/.." && pwd)
WORK="${WORK:-$REPO_ROOT/.build}"
export BUILD_DIR REPO_ROOT WORK TARGET=windows-x64

. "$BUILD_DIR/lib.sh"
. "$BUILD_DIR/config.sh"

need zip

detect_platform
need "$CROSS_PREFIX-objdump"
need "$CROSS_PREFIX-g++"
SRC="$WORK/$OPENFOAM_DIR"
TP="$WORK/$THIRDPARTY_DIR"
KIT_ROOT="$WORK/kit"
KIT="$KIT_ROOT/openfoam/$KIT_FOLDER"
OUT="$WORK/out"
BUILD_BIN="$SRC/platforms/$WM_OPTIONS_EXPECTED/bin"
BUILD_LIB="$SRC/platforms/$WM_OPTIONS_EXPECTED/lib"
KIT_BIN_REL="$OPENFOAM_DIR/platforms/$WM_OPTIONS_RUNTIME/bin"
KIT_BIN="$KIT/$KIT_BIN_REL"

[ -d "$BUILD_BIN" ] || die "no build to pack under $SRC"

rm -rf "$KIT_ROOT"
mkdir -p "$KIT_BIN" "$OUT"
log "assembling the kit at $KIT"

# ---------------------------------------------------------------------------
# 1. Files
# ---------------------------------------------------------------------------

copy_into() {   # copy_into <src-root> <dst-root> <relative path>...
    _from="$1"; _to="$2"; shift 2
    for _p in "$@"; do
        if [ -e "$_from/$_p" ]; then
            mkdir -p "$_to/$(dirname "$_p")"
            cp -R "$_from/$_p" "$_to/$(dirname "$_p")/"
        else
            warn "not in the build, skipped: $_p"
        fi
    done
}

# etc/ and etc-mingw/ as configured (the binaries read etc/controlDict and
# friends through WM_PROJECT_DIR); no bin/ - those are shell scripts, and a
# Windows node has no shell to run them with.
copy_into "$SRC" "$KIT/$OPENFOAM_DIR" \
    etc etc-mingw META-INFO LICENSE.md README.md CONTRIBUTORS.md CITATION.cff
for _t in $KIT_TUTORIALS; do
    [ -e "$SRC/tutorials/$_t" ] || die "build/config.sh names a tutorial the pack does not have: $_t"
    copy_into "$SRC" "$KIT/$OPENFOAM_DIR" "tutorials/$_t"
done
find "$KIT/$OPENFOAM_DIR/tutorials" -type d \( -name 'processor*' -o -name postProcessing \) -prune -exec rm -rf {} + 2>/dev/null || true
find "$KIT/$OPENFOAM_DIR/tutorials" -type f -name 'log.*' -delete

# Executables and OpenFOAM's DLLs, flat.
cp "$BUILD_BIN"/*.exe "$KIT_BIN/"
cp "$BUILD_LIB"/*.dll "$KIT_BIN/"
# The two Pstreams. Any other MPI-specific DLL (there is none in v2606
# besides ptscotchDecomp, a dummy on Windows) travels from the MPI directory.
cp "$BUILD_LIB/dummy/libPstream.dll"          "$KIT_BIN/libPstream.dll-dummy"
cp "$BUILD_LIB/$MSMPI_VERSION/libPstream.dll" "$KIT_BIN/libPstream.dll-msmpi"
cp "$BUILD_LIB/dummy/libPstream.dll"          "$KIT_BIN/libPstream.dll"
for _f in "$BUILD_LIB/$MSMPI_VERSION"/*.dll; do
    case "$(basename "$_f")" in libPstream.dll) ;; *) cp "$_f" "$KIT_BIN/" ;; esac
done

# Zip archives carry no symbolic links a Windows node could use: whatever
# upstream links (two files under etc/ and the tutorials) travels as a copy.
log "dereferencing symbolic links"
dereference_links "$KIT"
_links=$(find "$KIT" -type l | wc -l | tr -d ' ')
[ "$_links" = "0" ] || die "$_links symbolic link(s) survived in the kit"

# ---------------------------------------------------------------------------
# 2. Every DLL the kit's PE files import must be in the kit, or Windows's
# ---------------------------------------------------------------------------
#
# The Windows counterpart of the ldd staging on Linux and the otool check on
# macOS: each executable and DLL is asked what it imports; an import that is
# neither a system DLL (build/config.sh SYSTEM_DLLS_WINDOWS) nor already in
# the kit is looked for in the build - the ThirdParty platforms (scotch,
# fftw), then the cross compiler's own runtime directories (libstdc++-6,
# libgcc_s_seh-1, libwinpthread-1, zlib1) - and copied beside the rest. The
# pass repeats until nothing new is added, since a staged DLL has imports of
# its own. What cannot be found stops the pack. msmpi.dll is allowed for the
# MS-MPI Pstream only.

imports() { "$CROSS_PREFIX-objdump" -p "$1" 2>/dev/null | sed -n 's/^\s*DLL Name: //p' | tr 'A-Z' 'a-z' | sort -u; }
find_dll() {   # find_dll <name>: prints a path or nothing
    _fh=$(find "$TP/platforms" -type f -iname "$1" 2>/dev/null | head -1)
    if [ -z "$_fh" ]; then
        _fp=$("$CROSS_PREFIX-g++" -print-file-name="$1" 2>/dev/null)
        [ -n "$_fp" ] && [ -f "$_fp" ] && [ "$_fp" != "$1" ] && _fh="$_fp"
    fi
    printf '%s' "$_fh"
}

# Lists, not loops: ~400 PE files with ~10 imports each against ~150 DLLs
# is 600 000 comparisons, which as subprocesses took longer than the build.
log "staging the DLLs the kit imports"
: > "$WORK/staged-dlls.txt"
printf '%s
' $SYSTEM_DLLS_WINDOWS | tr 'A-Z' 'a-z' | sort -u > "$WORK/system-dlls.txt"
_round=0
while :; do
    _round=$((_round + 1)); [ "$_round" -le 6 ] || die "DLL staging did not converge in 6 rounds"
    _added=0
    for _f in "$KIT_BIN"/*.dll; do basename "$_f"; done | tr 'A-Z' 'a-z' | sort -u > "$WORK/kit-dlls.txt"
    for _f in "$KIT_BIN"/*.exe "$KIT_BIN"/*.dll "$KIT_BIN"/libPstream.dll-*; do
        for _imp in $(imports "$_f"); do
            case "$_imp" in api-ms-win-*) continue ;; esac
            grep -qxF "$_imp" "$WORK/system-dlls.txt" && continue
            grep -qxF "$_imp" "$WORK/kit-dlls.txt" && continue
            if [ "$_imp" = "msmpi.dll" ]; then
                case "$(basename "$_f")" in
                    libPstream.dll-msmpi) continue ;;
                    *) die "$(basename "$_f") imports msmpi.dll - only the MS-MPI Pstream may" ;;
                esac
            fi
            _src=$(find_dll "$_imp")
            [ -n "$_src" ] || die "$(basename "$_f") imports $_imp, which is neither in the kit, nor a system DLL, nor anywhere in the build"
            cp "$_src" "$KIT_BIN/$(basename "$_src")"
            echo "$(basename "$_src") <- $_src" >> "$WORK/staged-dlls.txt"
            echo "$_imp" >> "$WORK/kit-dlls.txt"
            _added=1
        done
    done
    [ "$_added" = "1" ] || break
done
log "  staged: $(wc -l < "$WORK/staged-dlls.txt" | tr -d ' ') DLL(s)"
sed 's/^/      /' "$WORK/staged-dlls.txt"

# Link-time products never travel: import libraries, static archives, objects.
find "$KIT" -type f \( -name '*.a' -o -name '*.la' -o -name '*.o' -o -name '*.def' -o -name '*.lib' \) -delete

# No two paths that differ by case alone, and no path with a space (D-16).
_collisions=$( (cd "$KIT" && find . | tr 'A-Z' 'a-z' | sort | uniq -d) )
[ -z "$_collisions" ] || { echo "$_collisions" | sed 's/^/      /' >&2; die "paths that differ by case alone"; }
_spaces=$( (cd "$KIT" && find . -name '* *') )
[ -z "$_spaces" ] || { echo "$_spaces" | sed 's/^/      /' >&2; die "paths with a space in the kit"; }

# ---------------------------------------------------------------------------
# 3. KIT.env - written from what the runtime needs, not from bashrc
# ---------------------------------------------------------------------------
#
# On Linux and macOS KIT.env is recorded by sourcing the kit's bashrc; here
# the bashrc describes the Linux cross-tree, not the Windows runtime. The
# runtime needs the set OpenCFD's own setEnvVariables.bat sets, proven on a
# real Windows machine from a plain folder with a cleared environment
# (2026-09-23): the executables' directory on PATH, WM_PROJECT_DIR, a HOME
# and a TEMP of its own. The controller appends %SystemRoot%\System32 and
# %SystemRoot% to PATH and passes SystemRoot through; nothing else.

log "generating KIT.env"
{
    echo "# OpenFOAM $OPENFOAM_VERSION kit, $PLATFORM ($WM_OPTIONS_RUNTIME)."
    echo "# Set every line on the solver process, with @KIT@ = the kit folder and"
    echo "# @SCRATCH@ = the task's private scratch (both without spaces, D-16)."
    echo "# Append %SystemRoot%\\System32 and %SystemRoot% to PATH yourself and pass"
    echo "# SystemRoot through; forward slashes are fine for Windows here."
    echo "PATH=@KIT@/$KIT_BIN_REL"
    echo "WM_PROJECT=OpenFOAM"
    echo "WM_PROJECT_VERSION=$OPENFOAM_VERSION"
    echo "WM_PROJECT_DIR=@KIT@/$OPENFOAM_DIR"
    echo "WM_OPTIONS=$WM_OPTIONS_RUNTIME"
    echo "WM_ARCH=win64"
    echo "WM_COMPILER=$WM_COMPILER"
    echo "WM_PRECISION_OPTION=$WM_PRECISION_OPTION"
    echo "WM_LABEL_SIZE=$WM_LABEL_SIZE"
    echo "WM_COMPILE_OPTION=$WM_COMPILE_OPTION"
    echo "WM_MPLIB=MSMPI"
    echo "WM_OSTYPE=MSwindows"
    echo "FOAM_API=$OPENFOAM_API"
    echo "FOAM_MPI=msmpi"
    echo "FOAM_APPBIN=@KIT@/$KIT_BIN_REL"
    echo "FOAM_LIBBIN=@KIT@/$KIT_BIN_REL"
    echo "FOAM_ETC=@KIT@/$OPENFOAM_DIR/etc"
    echo "FOAM_TUTORIALS=@KIT@/$OPENFOAM_DIR/tutorials"
    echo "HOME=@SCRATCH@/home"
    echo "USERPROFILE=@SCRATCH@/home"
    echo "TEMP=@SCRATCH@/tmp"
    echo "TMP=@SCRATCH@/tmp"
    echo "TMPDIR=@SCRATCH@/tmp"
    echo "WM_PROJECT_USER_DIR=@SCRATCH@/home/OpenFOAM/user-$OPENFOAM_VERSION"
    echo "FOAM_SIGFPE=true"
    echo "FOAM_SETNAN=false"
    # MPI is the node's. The controller runs mpiexec from the node's MS-MPI
    # (MSMPI_BIN, set system-wide by Microsoft's installer) and, once per
    # install, copies KIT_PSTREAM_MSMPI over KIT_PSTREAM_TARGET when the
    # node has msmpi.dll; otherwise the serial default stays.
    echo "KIT_PSTREAM_TARGET=@KIT@/$KIT_BIN_REL/libPstream.dll"
    echo "KIT_PSTREAM_DUMMY=@KIT@/$KIT_BIN_REL/libPstream.dll-dummy"
    echo "KIT_PSTREAM_MSMPI=@KIT@/$KIT_BIN_REL/libPstream.dll-msmpi"
    echo "KIT_PSTREAM_DEFAULT=dummy"
    echo "KIT_MSMPI_BUILT_AGAINST=$MSMPI_VERSION"
    echo "KIT_PLATFORM=$PLATFORM"
    echo "KIT_WM_OPTIONS=$WM_OPTIONS_RUNTIME"
    echo "KIT_OPENFOAM_VERSION=$OPENFOAM_VERSION"
    echo "KIT_MPI=msmpi-external"
} > "$KIT/KIT.env"
expect_in_file "$KIT/KIT.env" "^WM_PROJECT_DIR=@KIT@/$OPENFOAM_DIR\$" "WM_PROJECT_DIR"
expect_in_file "$KIT/KIT.env" "^PATH=@KIT@/$KIT_BIN_REL\$"            "PATH"

# ---------------------------------------------------------------------------
# 4. Licences, source offer, BUILDINFO, zip
# ---------------------------------------------------------------------------

cp "$REPO_ROOT/LICENSE" "$KIT/LICENSE-GPL-3.0.txt"
cp "$REPO_ROOT/NOTICE.md" "$KIT/NOTICE.md"
cp "$REPO_ROOT/redistribution/SOURCE-OFFER.md" "$KIT/SOURCE-OFFER.md"

_git=$(cd "$REPO_ROOT" && git rev-parse --short HEAD 2>/dev/null || echo unknown)
_cc=$("$CROSS_PREFIX-g++" --version | head -1)
{
    echo "OpenFOAM $OPENFOAM_VERSION (api $OPENFOAM_API) - OmnibusCloud kit"
    echo "platform:        $PLATFORM"
    echo "wm-options:      $WM_OPTIONS_RUNTIME (cross-built as $WM_OPTIONS_EXPECTED)"
    echo "mpi:             MS-MPI, the node's own (built against the $MSMPI_VERSION SDK; libPstream.dll = serial, libPstream.dll-msmpi = MS-MPI)"
    echo "scotch:          $SCOTCH_VERSION (pt-scotch: not built for MinGW, upstream's dummy)"
    echo "kahip:           $KAHIP_VERSION"
    echo "fftw:            $FFTW_VERSION"
    echo "cgal/boost:      off"
    echo "adios2/hdf5:     off"
    echo "metis:           off"
    echo "upstream tag:    $UPSTREAM_TAG ($UPSTREAM_COMMIT)"
    echo "source pack:     $OPENFOAM_SRC_SHA256"
    echo "thirdparty pack: $THIRDPARTY_SHA256"
    echo "msmpi sdk:       $MSMPI_SDK_SHA256"
    echo "built from:      OmnibusCloud/OpenFOAM $_git"
    echo "built on:        $(uname -srm), $_cc"
    echo "built at:        $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    echo "staged DLLs:"
    sed 's/^/  /' "$WORK/staged-dlls.txt"
    echo
    echo "sha256 of every file (relative to the kit folder):"
    (cd "$KIT" && find . -type f ! -name BUILDINFO.txt | sort | while IFS= read -r _f; do
        printf '%s  %s\n' "$(sha256_of "$_f")" "${_f#./}"
    done)
} > "$KIT/BUILDINFO.txt"

log "zipping"
rm -f "$OUT/openfoam-$PLATFORM.zip"
(cd "$KIT_ROOT" && zip -r -q -X "$OUT/openfoam-$PLATFORM.zip" "openfoam/$KIT_FOLDER")
(cd "$OUT" && sha256_of "openfoam-$PLATFORM.zip" | sed "s|\$|  openfoam-$PLATFORM.zip|" > SHA256SUMS)
log "kit:     $(du -sh "$KIT" | cut -f1)  ($(find "$KIT" -type f | wc -l | tr -d ' ') files, $(ls "$KIT_BIN"/*.exe | wc -l | tr -d ' ') executables, $(ls "$KIT_BIN"/*.dll | wc -l | tr -d ' ') DLLs)"
log "archive: $(du -sh "$OUT/openfoam-$PLATFORM.zip" | cut -f1)  $OUT/openfoam-$PLATFORM.zip"
log "sha256:  $(sha256_of "$OUT/openfoam-$PLATFORM.zip")"
