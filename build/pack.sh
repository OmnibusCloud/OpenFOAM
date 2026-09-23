#!/bin/sh
# Assemble the kit from a finished build and zip it.
#
#   sh build/pack.sh          .build/kit/openfoam/<platform>/ and .build/out/openfoam-<platform>.zip
#
# The kit is the runtime subset of the build: etc/, bin/, the platform's
# bin/ and lib/, the tutorials (the acceptance and oracle corpus), and the
# ThirdParty runtime (Open MPI, scotch, kahip, fftw). No sources, no headers,
# no symbolic links. Beside it: KIT.env (the environment the controller sets,
# generated from upstream's own bashrc so the two cannot drift), BUILDINFO.txt,
# the licences and the source offer.
set -e

BUILD_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$BUILD_DIR/.." && pwd)
WORK="${WORK:-$REPO_ROOT/.build}"
export BUILD_DIR REPO_ROOT WORK

. "$BUILD_DIR/lib.sh"
. "$BUILD_DIR/config.sh"

need zip
need bash

detect_platform
SRC="$WORK/$OPENFOAM_DIR"
TP="$WORK/$THIRDPARTY_DIR"
KIT_ROOT="$WORK/kit"
KIT="$KIT_ROOT/openfoam/$KIT_FOLDER"
OUT="$WORK/out"

[ -d "$SRC/platforms/$WM_OPTIONS_EXPECTED/bin" ] || die "no build to pack under $SRC"

rm -rf "$KIT_ROOT"
mkdir -p "$KIT/$OPENFOAM_DIR/platforms/$WM_OPTIONS_EXPECTED" "$KIT/$THIRDPARTY_DIR" "$OUT"
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

copy_into "$SRC" "$KIT/$OPENFOAM_DIR" \
    etc bin META-INFO LICENSE.md README.md CONTRIBUTORS.md CITATION.cff tutorials \
    "platforms/$WM_OPTIONS_EXPECTED/bin" "platforms/$WM_OPTIONS_EXPECTED/lib"

# The whole ThirdParty runtime for this platform, minus what only a compiler
# needs (headers, pkg-config, libtool archives, manuals).
copy_into "$TP" "$KIT/$THIRDPARTY_DIR" platforms COPYING README.md SOURCES.md
find "$KIT/$THIRDPARTY_DIR/platforms" -type d \( -name include -o -name pkgconfig -o -name man -o -name doc \) -prune -exec rm -rf {} + 2>/dev/null || true
find "$KIT/$THIRDPARTY_DIR/platforms" -type f \( -name '*.la' -o -name '*.a' \) -delete

# The build's own scratch that would otherwise travel: lnInclude trees,
# logs, the wmake object directories are not under platforms/bin|lib and
# were never copied; but tutorials may hold results if someone ran one.
find "$KIT/$OPENFOAM_DIR/tutorials" -type d \( -name 'processor*' -o -name postProcessing \) -prune -exec rm -rf {} + 2>/dev/null || true
find "$KIT/$OPENFOAM_DIR/tutorials" -type f -name 'log.*' -delete

log "dereferencing symbolic links"
dereference_links "$KIT"
_links=$(find "$KIT" -type l | wc -l | tr -d ' ')
[ "$_links" = "0" ] || die "$_links symbolic link(s) survived in the kit"

# ---------------------------------------------------------------------------
# 2. Runtime libraries the node is not expected to have
# ---------------------------------------------------------------------------
#
# Every ELF in the kit is asked what it needs; anything resolved from outside
# the kit that is not on the platform's system list is copied beside the
# kit's own libraries. libgomp is the expected case. The foreign-image start
# check in verify.sh is what proves this list complete.

OF_LIB="$KIT/$OPENFOAM_DIR/platforms/$WM_OPTIONS_EXPECTED/lib"
case "$PLATFORM" in
    linux-x64)
        need ldd
        _kitlibs=$(find "$KIT" -type d -name lib | tr '\n' ':')
        _staged=""
        find "$KIT" -type f \( -path '*/bin/*' -o -name "*.$SO_EXT*" \) | while IFS= read -r _f; do
            file "$_f" 2>/dev/null | grep -q ELF || continue
            LD_LIBRARY_PATH="$_kitlibs" ldd "$_f" 2>/dev/null | awk '/=> \//{print $1, $3}' | while read -r _name _path; do
                case " $SYSTEM_LIBS_LINUX " in *" $_name "*) continue ;; esac
                case "$_path" in "$KIT"/*) continue ;; esac
                if [ ! -e "$OF_LIB/$_name" ]; then
                    cp -L "$_path" "$OF_LIB/$_name"
                    echo "$_name <- $_path" >> "$WORK/staged-libs.txt"
                fi
            done
        done
        if [ -f "$WORK/staged-libs.txt" ]; then
            sort -u "$WORK/staged-libs.txt" > "$WORK/staged-libs.sorted"
            mv "$WORK/staged-libs.sorted" "$WORK/staged-libs.txt"
            log "staged from the build host:"; sed 's/^/      /' "$WORK/staged-libs.txt"
        else
            log "no host library needed staging"
        fi
        ;;
    macos-arm64)
        warn "macOS: dylib staging and @loader_path fixups arrive with Phase 0.4; the kit is packed as built"
        ;;
esac

# ---------------------------------------------------------------------------
# 3. KIT.env - generated from the kit's own etc/bashrc
# ---------------------------------------------------------------------------
#
# The kit's bashrc is sourced ONCE, here, in an empty environment with a
# throwaway HOME, and the variables it establishes are recorded with the kit
# path replaced by @KIT@ and the throwaway HOME by @SCRATCH@/home. The
# controller substitutes both and sets the result on the solver process;
# nothing on a node ever sources anything. PATH and the library path keep
# only their kit entries: the controller appends the system directories.

log "generating KIT.env"
_home=/tmp/openfoam-kit-home.$$
_before="$WORK/env.before"; _after="$WORK/env.after"
env -i HOME="$_home" USER=user PATH=/usr/bin:/bin bash -c 'env' | sort > "$_before"
env -i HOME="$_home" USER=user PATH=/usr/bin:/bin bash -c "unset WM_PROJECT_DIR; source '$KIT/$OPENFOAM_DIR/etc/bashrc' >/dev/null 2>&1; env" | sort > "$_after"

{
    echo "# OpenFOAM $OPENFOAM_VERSION kit, $PLATFORM ($WM_OPTIONS_EXPECTED)."
    echo "# Set every line on the solver process, with @KIT@ = the kit folder and"
    echo "# @SCRATCH@ = the task's private scratch. Append the system directories to"
    echo "# PATH yourself. Never source anything from the kit."
    comm -13 "$_before" "$_after" \
        | grep -E '^(WM_|FOAM_|MPI_|OPAL_|SCOTCH_|KAHIP_|FFTW_|LD_LIBRARY_PATH=|DYLD_LIBRARY_PATH=|PATH=)' \
        | grep -v -E '^(WM_PROJECT_USER_DIR|FOAM_RUN|FOAM_USER_APPBIN|FOAM_USER_LIBBIN|FOAM_SETTINGS|FOAM_SRC|FOAM_SOLVERS|FOAM_UTILITIES|FOAM_APP)=' \
        | while IFS= read -r _line; do
            _name=${_line%%=*}; _value=${_line#*=}
            case "$_name" in
                PATH|LD_LIBRARY_PATH|DYLD_LIBRARY_PATH|FOAM_LD_LIBRARY_PATH)
                    # kit entries only, in order
                    _kept=""
                    _old_ifs=$IFS; IFS=:
                    for _e in $_value; do
                        case "$_e" in "$KIT"/*) _kept="${_kept:+$_kept:}$_e" ;; esac
                    done
                    IFS=$_old_ifs
                    _value=$_kept
                    ;;
            esac
            [ -n "$_value" ] || continue
            printf '%s=%s\n' "$_name" "$_value"
        done \
        | sed -e "s|$KIT|@KIT@|g" -e "s|$_home|@SCRATCH@/home|g"
    echo "HOME=@SCRATCH@/home"
    echo "TMPDIR=@SCRATCH@/tmp"
    echo "WM_PROJECT_USER_DIR=@SCRATCH@/home/OpenFOAM/user-$OPENFOAM_VERSION"
    echo "FOAM_SIGFPE=true"
    echo "FOAM_SETNAN=false"
    # Single-node Open MPI inside a container or a service: shared memory
    # only, no network interfaces to probe, no cross-memory attach (blocked
    # by ptrace restrictions in containers), never an rsh agent.
    echo "OMPI_MCA_btl=self,vader"
    echo "OMPI_MCA_btl_vader_single_copy_mechanism=none"
    echo "OMPI_MCA_plm_rsh_agent="
    echo "OMPI_MCA_rmaps_base_oversubscribe=true"
    # What the controller marks executable after unpacking (zip keeps no modes).
    echo "KIT_EXECUTABLE_DIRS=@KIT@/$OPENFOAM_DIR/platforms/$WM_OPTIONS_EXPECTED/bin:@KIT@/$OPENFOAM_DIR/bin:@KIT@/$THIRDPARTY_DIR/platforms/$TP_MPI_PLATFORM/$OPENMPI_VERSION/bin"
    echo "KIT_PLATFORM=$PLATFORM"
    echo "KIT_WM_OPTIONS=$WM_OPTIONS_EXPECTED"
    echo "KIT_OPENFOAM_VERSION=$OPENFOAM_VERSION"
    echo "KIT_MPI=$OPENMPI_VERSION"
} > "$KIT/KIT.env"
rm -rf "$_home"

expect_in_file "$KIT/KIT.env" "^WM_PROJECT_DIR=@KIT@/$OPENFOAM_DIR\$"  "WM_PROJECT_DIR"
expect_in_file "$KIT/KIT.env" "^FOAM_APPBIN=@KIT@/"                      "FOAM_APPBIN"
expect_in_file "$KIT/KIT.env" "^FOAM_LIBBIN=@KIT@/"                      "FOAM_LIBBIN"
expect_in_file "$KIT/KIT.env" "^FOAM_MPI=$OPENMPI_VERSION\$"             "FOAM_MPI"
expect_in_file "$KIT/KIT.env" "^MPI_ARCH_PATH=@KIT@/$THIRDPARTY_DIR/"    "MPI_ARCH_PATH"
expect_in_file "$KIT/KIT.env" "^OPAL_PREFIX=@KIT@/"                      "OPAL_PREFIX"
case "$PLATFORM" in
    linux-x64) expect_in_file "$KIT/KIT.env" "^LD_LIBRARY_PATH=@KIT@/" "LD_LIBRARY_PATH" ;;
esac

# ---------------------------------------------------------------------------
# 4. Licences, source offer, BUILDINFO
# ---------------------------------------------------------------------------

cp "$REPO_ROOT/LICENSE" "$KIT/LICENSE-GPL-3.0.txt"
cp "$REPO_ROOT/NOTICE.md" "$KIT/NOTICE.md"
cp "$REPO_ROOT/redistribution/SOURCE-OFFER.md" "$KIT/SOURCE-OFFER.md"

_git=$(cd "$REPO_ROOT" && git rev-parse --short HEAD 2>/dev/null || echo unknown)
_cc=$( (cc --version 2>/dev/null || gcc --version 2>/dev/null || clang --version 2>/dev/null) | head -1)
{
    echo "OpenFOAM $OPENFOAM_VERSION (api $OPENFOAM_API) - OmnibusCloud kit"
    echo "platform:        $PLATFORM"
    echo "wm-options:      $WM_OPTIONS_EXPECTED"
    echo "mpi:             $OPENMPI_VERSION (ThirdParty, bundled)"
    echo "scotch:          $SCOTCH_VERSION"
    echo "kahip:           $KAHIP_VERSION"
    echo "fftw:            $FFTW_VERSION"
    echo "cgal/boost:      off"
    echo "adios2/hdf5:     off"
    echo "metis:           off"
    echo "upstream tag:    $UPSTREAM_TAG ($UPSTREAM_COMMIT)"
    echo "source pack:     $OPENFOAM_SRC_SHA256"
    echo "thirdparty pack: $THIRDPARTY_SHA256"
    echo "built from:      OmnibusCloud/OpenFOAM $_git"
    echo "built on:        $(uname -srm), $_cc"
    echo "built at:        $(date -u +%Y-%m-%dT%H:%M:%SZ)"
    if [ -f "$WORK/staged-libs.txt" ]; then
        echo "staged host libraries:"; sed 's/^/  /' "$WORK/staged-libs.txt"
    fi
    echo
    echo "sha256 of every file (relative to the kit folder):"
    ( cd "$KIT" && find . -type f ! -name BUILDINFO.txt | LC_ALL=C sort | while IFS= read -r _f; do
        printf '%s  %s\n' "$(sha256_of "$_f")" "${_f#./}"
      done )
} > "$KIT/BUILDINFO.txt"

# ---------------------------------------------------------------------------
# 5. The archive
# ---------------------------------------------------------------------------

_zip="$OUT/openfoam-$KIT_FOLDER.zip"
rm -f "$_zip"
log "zipping"
( cd "$KIT_ROOT" && zip -r -q -X "$_zip" openfoam )
( cd "$OUT" && sha256_of "$(basename "$_zip")" | sed "s|\$|  $(basename "$_zip")|" > SHA256SUMS )

log "kit:     $(du -sh "$KIT" | cut -f1)  ($(find "$KIT" -type f | wc -l | tr -d ' ') files)"
log "archive: $(du -h "$_zip" | cut -f1)  $_zip"
log "sha256:  $(cut -d' ' -f1 "$OUT/SHA256SUMS")"
