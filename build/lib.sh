# Shared helpers. Sourced, never executed.
# POSIX sh with bash extensions kept out on purpose: this runs under Ubuntu,
# macOS and Git Bash, and the three do not agree on much beyond POSIX. The
# one place bash is required (sourcing upstream's etc/bashrc) is invoked
# explicitly.

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m--> %s\033[0m\n' "$*" >&2; }
die()  { printf '\033[1;31m*** %s\033[0m\n' "$*" >&2; exit 1; }

need() {
    command -v "$1" >/dev/null 2>&1 || die "required tool not on PATH: $1"
}

sha256_of() {
    if command -v sha256sum >/dev/null 2>&1; then
        sha256sum "$1" | cut -d' ' -f1
    elif command -v shasum >/dev/null 2>&1; then
        shasum -a 256 "$1" | cut -d' ' -f1
    else
        die "no sha256sum and no shasum - cannot verify downloads"
    fi
}

# fetch_verify <url> <sha256> <dest>
#
# Downloads once and refuses to proceed on a checksum mismatch. Every input to
# a redistributed binary goes through here; nothing is taken on trust.
fetch_verify() {
    _url="$1"; _sha="$2"; _dest="$3"
    if [ -f "$_dest" ]; then
        _have=$(sha256_of "$_dest")
        if [ "$_have" = "$_sha" ]; then
            log "cached, checksum ok: $(basename "$_dest")"
            return 0
        fi
        warn "cached file has the wrong checksum, refetching: $(basename "$_dest")"
        rm -f "$_dest"
    fi
    log "fetching $(basename "$_dest")"
    curl -fsSL --retry 5 --retry-delay 5 --retry-all-errors \
         --connect-timeout 20 --max-time 1800 \
         -o "$_dest.part" "$_url" \
        || die "download failed after retries: $_url
This is a network failure, not a build failure - the file is pinned by
checksum, so re-running is safe."
    _have=$(sha256_of "$_dest.part")
    [ "$_have" = "$_sha" ] || die "checksum mismatch for $_url
  expected $_sha
  got      $_have"
    mv "$_dest.part" "$_dest"
}

# expect_in_file <file> <pattern> <what>
#
# Guards every assumption we make about upstream's layout after configuring
# it. If upstream changes shape, the build stops instead of silently building
# something else.
expect_in_file() {
    grep -q -- "$2" "$1" || die "expected to find $3 in $1 - upstream layout changed, fix the build script"
}

cpu_count() {
    if command -v nproc >/dev/null 2>&1; then nproc
    elif command -v sysctl >/dev/null 2>&1; then sysctl -n hw.ncpu
    else echo 2
    fi
}

# dereference_links <dir>
#
# Replaces every symbolic link under <dir> with a copy of its target (files
# and directories alike) and removes links whose target is gone. Zip archives
# and the node-side extractor keep no links, so the kit carries none; done
# here, once, rather than hoped for at unpack time.
dereference_links() {
    find "$1" -type l | while IFS= read -r _link; do
        if [ -e "$_link" ]; then
            cp -R -L "$_link" "$_link.deref.tmp"
            rm "$_link"
            mv "$_link.deref.tmp" "$_link"
        else
            rm "$_link"
        fi
    done
}
