#!/bin/sh
# Fetch, verify and stage everything MIRROR.md lists; optionally publish it.
#
#   sh redistribution/mirror.sh                  stage under redistribution/out
#   sh redistribution/mirror.sh --push-release   also create/update the GitHub release
#
# The release tag is fixed (upstream-mirror-v2606): one mirror per upstream
# version, updated in place if a file is ever added, never renamed.
set -e

HERE=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$HERE/.." && pwd)
. "$REPO_ROOT/build/lib.sh"
. "$REPO_ROOT/build/config.sh"

OUT="$HERE/out"
TAG="upstream-mirror-$OPENFOAM_VERSION"
REPO="${MIRROR_REPO:-OmnibusCloud/OpenFOAM}"

need curl
mkdir -p "$OUT"

fetch_verify "$OPENFOAM_SRC_URL" "$OPENFOAM_SRC_SHA256" "$OUT/$OPENFOAM_DIR.tgz"
fetch_verify "$THIRDPARTY_URL"   "$THIRDPARTY_SHA256"   "$OUT/$THIRDPARTY_DIR.tar.gz"
fetch_verify "$OPENMPI_URL"      "$OPENMPI_SHA256"      "$OUT/$OPENMPI_VERSION.tar.bz2"

( cd "$OUT" && for f in "$OPENFOAM_DIR.tgz" "$THIRDPARTY_DIR.tar.gz" "$OPENMPI_VERSION.tar.bz2"; do
    printf '%s  %s\n' "$(sha256_of "$f")" "$f"
  done > SHA256SUMS )
log "staged:"; sed 's/^/      /' "$OUT/SHA256SUMS"

if [ "${1:-}" = "--push-release" ]; then
    need gh
    if gh release view "$TAG" --repo "$REPO" >/dev/null 2>&1; then
        log "updating release $TAG"
    else
        log "creating release $TAG"
        gh release create "$TAG" --repo "$REPO" \
            --title "Upstream mirror - OpenFOAM $OPENFOAM_VERSION and its corresponding source" \
            --notes-file "$HERE/MIRROR.md"
    fi
    gh release upload "$TAG" --repo "$REPO" --clobber \
        "$OUT/$OPENFOAM_DIR.tgz" "$OUT/$THIRDPARTY_DIR.tar.gz" "$OUT/$OPENMPI_VERSION.tar.bz2" "$OUT/SHA256SUMS"
    log "published: https://github.com/$REPO/releases/tag/$TAG"
fi
