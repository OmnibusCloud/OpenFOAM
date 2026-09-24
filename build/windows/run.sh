#!/bin/sh
# Local Windows cross-build in Docker, from any checkout, and the acceptance
# of the result on this Windows machine.
#
#   sh build/windows/run.sh image     build the cross-build image
#   sh build/windows/run.sh build     cross-build + pack (kit in .build/out)
#   sh build/windows/run.sh verify    run build/verify.ps1 on this machine (Windows only)
#   sh build/windows/run.sh all       image, build, verify
#
#   JOBS=16 LONG=1 sh build/windows/run.sh all
#
# The build tree lives in a named Docker volume of its own (the Linux build
# has another); the repository is mounted read-only; the pinned downloads
# come from @Downloads/ when present. The acceptance needs a Windows host:
# without MS-MPI installed it runs the serial cases only and says so.
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
. "$REPO/build/lib.sh"

IMAGE=omnibuscloud/openfoam-cross:24.04
VOLUME=openfoam-v2606-cross
OUT="$REPO/.build/out"
DL="$REPO/@Downloads"

need docker

host_path() {
    if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi
}
export MSYS_NO_PATHCONV=1

do_image() {
    log "building $IMAGE"
    docker build -q -t "$IMAGE" -f "$(host_path "$HERE/Dockerfile")" "$(host_path "$HERE")"
}

do_build() {
    mkdir -p "$OUT" "$DL"
    docker run --rm -v "$VOLUME:/build" alpine:3 sh -c 'chown 1000:1000 /build'
    log "cross-build in volume $VOLUME (JOBS=${JOBS:-all})"
    docker run --rm \
        -v "$(host_path "$REPO"):/repo:ro" \
        -v "$(host_path "$DL"):/dl" \
        -v "$(host_path "$OUT"):/out" \
        -v "$VOLUME:/build" \
        -e TARGET=windows-x64 -e WORK=/build -e DEPS_DIR=/dl ${JOBS:+-e JOBS="$JOBS"} ${FORCE:+-e FORCE="$FORCE"} \
        "$IMAGE" sh -c 'sh /repo/build/build.sh && cp /build/out/openfoam-windows-x64.zip /out/ && ls -l /out'
}

do_verify() {
    [ -f "$OUT/openfoam-windows-x64.zip" ] || die "no kit at $OUT/openfoam-windows-x64.zip - build first"
    command -v powershell.exe >/dev/null 2>&1 || die "the Windows acceptance runs on a Windows host (powershell.exe)"
    _root="${VERIFY_ROOT:-C:\\ofverify}"
    log "verify on this machine under $_root (LONG=${LONG:-0})"
    powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(host_path "$REPO/build/verify.ps1")" \
        -Kit "$(host_path "$OUT/openfoam-windows-x64.zip")" -Root "$_root" ${LONG:+-Long}
}

case "${1:-all}" in
    image)  do_image ;;
    build)  do_build ;;
    verify) do_verify ;;
    all)    do_image; do_build; do_verify ;;
    *) die "usage: sh build/windows/run.sh image|build|verify|all" ;;
esac
