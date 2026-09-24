#!/bin/sh
# Local Linux build and verification in Docker, from any checkout (Windows
# included: the build reads the pinned source pack when upstream/ has no
# symbolic links, see README.md).
#
#   sh build/linux/run.sh image      build the two images
#   sh build/linux/run.sh build      build + pack (kit in .build/out)
#   sh build/linux/run.sh verify     verify the kit in the foreign image
#   sh build/linux/run.sh all        image, build, verify
#
#   JOBS=16 LONG=1 sh build/linux/run.sh all
#
# The build tree lives in a named Docker volume (a bind mount from Windows
# is slow and cannot hold symbolic links); the repository is mounted
# read-only; the pinned downloads come from @Downloads/ when present.
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
REPO=$(cd "$HERE/../.." && pwd)
. "$REPO/build/lib.sh"

IMAGE=omnibuscloud/openfoam-build:22.04
VERIFY_IMAGE=omnibuscloud/openfoam-verify:debian12
VOLUME=openfoam-v2606-build
OUT="$REPO/.build/out"
DL="$REPO/@Downloads"

need docker

# Git Bash on Windows rewrites POSIX-looking arguments into Windows paths;
# docker wants Windows paths for -v sources and untouched paths for the rest.
host_path() {
    if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else echo "$1"; fi
}
export MSYS_NO_PATHCONV=1

do_image() {
    log "building $IMAGE"
    docker build -q -t "$IMAGE" -f "$(host_path "$HERE/Dockerfile")" "$(host_path "$HERE")"
    log "building $VERIFY_IMAGE"
    docker build -q -t "$VERIFY_IMAGE" -f "$(host_path "$HERE/Dockerfile.verify")" "$(host_path "$HERE")"
}

do_build() {
    mkdir -p "$OUT" "$DL"
    # A fresh named volume belongs to root; the build runs unprivileged.
    docker run --rm -v "$VOLUME:/build" alpine:3 sh -c 'chown 1000:1000 /build'
    log "build in volume $VOLUME (JOBS=${JOBS:-all})"
    docker run --rm \
        -v "$(host_path "$REPO"):/repo:ro" \
        -v "$(host_path "$DL"):/dl" \
        -v "$(host_path "$OUT"):/out" \
        -v "$VOLUME:/build" \
        -e WORK=/build -e DEPS_DIR=/dl ${JOBS:+-e JOBS="$JOBS"} ${FORCE:+-e FORCE="$FORCE"} \
        "$IMAGE" sh -c 'sh /repo/build/build.sh && cp /build/out/* /out/ && ls -l /out'
}

do_verify() {
    [ -f "$OUT/openfoam-linux-x64.zip" ] || die "no kit at $OUT/openfoam-linux-x64.zip - build first"
    log "verify in $VERIFY_IMAGE (LONG=${LONG:-0})"
    # Open MPI's shared-memory transport lives in /dev/shm; Docker's default
    # 64 MB is enough for pitzDaily, not for motorBike on six ranks.
    docker run --rm --shm-size=1g \
        -v "$(host_path "$REPO"):/repo:ro" \
        -v "$(host_path "$OUT"):/out:ro" \
        -e VERIFY_ROOT=/home/verify/verify -e WORK=/home/verify/work ${LONG:+-e LONG="$LONG"} \
        "$VERIFY_IMAGE" sh /repo/build/verify.sh /out/openfoam-linux-x64.zip
}

case "${1:-all}" in
    image)  do_image ;;
    build)  do_build ;;
    verify) do_verify ;;
    all)    do_image; do_build; do_verify ;;
    *) die "usage: sh build/linux/run.sh image|build|verify|all" ;;
esac
