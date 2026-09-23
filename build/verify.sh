#!/bin/sh
# Acceptance of a kit archive - run where the kit was NOT built.
#
#   sh build/verify.sh [kit.zip]        default .build/out/openfoam-<platform>.zip
#   LONG=1 sh build/verify.sh           also motorBike (snappyHexMesh, six ranks, minutes)
#   VERIFY_ROOT=/somewhere ...          where to unpack and run (default .build/verify)
#
# What a compute node does, replayed exactly: unpack the zip (no links, no
# mode bits), mark the executable directories KIT.env names, set the
# environment KIT.env says and nothing else, HOME and TMPDIR inside the
# scratch, run. Then the two questions that matter:
#
#   1. Does it run?   simpleFoam on pitzDaily serial and on four ranks under
#                     the bundled mpirun, interFoam on damBreak (setFields),
#                     foamDictionary; motorBike when asked (LONG=1).
#   2. Where did it write?   Anything newer than the start marker outside the
#                     kit and the scratch fails the run. The kit itself is
#                     made read-only first, so a write into it fails the step
#                     that tried.
set -e

BUILD_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_ROOT=$(cd "$BUILD_DIR/.." && pwd)
WORK="${WORK:-$REPO_ROOT/.build}"
export BUILD_DIR REPO_ROOT WORK

. "$BUILD_DIR/lib.sh"
. "$BUILD_DIR/config.sh"

need unzip
need find

detect_platform
KIT_ZIP="${1:-$WORK/out/openfoam-$KIT_FOLDER.zip}"
[ -f "$KIT_ZIP" ] || die "no kit archive at $KIT_ZIP"

ROOT="${VERIFY_ROOT:-$WORK/verify}"
rm -rf "$ROOT"
KITDIR="$ROOT/kit dir with space"
SCRATCH="$ROOT/scratch"
mkdir -p "$KITDIR" "$SCRATCH/home" "$SCRATCH/tmp" "$SCRATCH/cases"

log "unpacking $(basename "$KIT_ZIP") under '$KITDIR'"
unzip -q "$KIT_ZIP" -d "$KITDIR"
KIT="$KITDIR/openfoam/$KIT_FOLDER"
[ -f "$KIT/KIT.env" ] || die "the archive carries no KIT.env"
_links=$(find "$KIT" -type l | wc -l | tr -d ' ')
[ "$_links" = "0" ] || die "$_links symbolic link(s) came out of the archive"

# A node's extractor keeps no mode bits: drop them all, then apply the one
# rule the controller applies.
find "$KIT" -type f -exec chmod a-x {} +
ENVFILE="$ROOT/kit.env"
sed -e "s|@KIT@|$KIT|g" -e "s|@SCRATCH@|$SCRATCH|g" "$KIT/KIT.env" \
    | grep -v '^#' \
    | sed -E "s|^([A-Za-z_][A-Za-z0-9_]*)=(.*)\$|\1='\2'|" > "$ENVFILE"
_exec_dirs=$(grep "^KIT_EXECUTABLE_DIRS=" "$ENVFILE" | cut -d= -f2- | tr -d "'")
[ -n "$_exec_dirs" ] || die "KIT.env names no KIT_EXECUTABLE_DIRS"
_old_ifs=$IFS; IFS=:
for _d in $_exec_dirs; do
    [ -d "$_d" ] || die "KIT_EXECUTABLE_DIRS names a directory the kit does not have: $_d"
    find "$_d" -type f -exec chmod a+x {} +
done
IFS=$_old_ifs

# Read-only from here on: a step that writes into the kit fails.
chmod -R a-w "$KIT"

# run_in <case-dir> <command...>: the environment is KIT.env and nothing
# else, plus the system PATH the controller appends; stdout to the caller.
cat > "$ROOT/run.sh" <<'EOF'
#!/bin/sh
envfile="$1"; casedir="$2"; shift 2
set -a
. "$envfile"
set +a
PATH="$PATH:/usr/local/bin:/usr/bin:/bin"
FOAM_CASE="$casedir"
export PATH FOAM_CASE
cd "$casedir" || exit 97
exec "$@"
EOF
chmod +x "$ROOT/run.sh"
run_in() {
    _case="$1"; shift
    env -i "$ROOT/run.sh" "$ENVFILE" "$_case" "$@"
}

TUT="$KIT/$OPENFOAM_DIR/tutorials"
[ -d "$TUT" ] || die "the kit carries no tutorials"

case_copy() {   # case_copy <tutorial relative path> <name>
    rm -rf "$SCRATCH/cases/$2"
    cp -R "$TUT/$1" "$SCRATCH/cases/$2"
    chmod -R u+w "$SCRATCH/cases/$2"
    echo "$SCRATCH/cases/$2"
}

step() { log "step: $*"; }
elapsed() { _now=$(date +%s); echo "$(( _now - $1 )) s"; }

touch "$ROOT/.start-marker"
sleep 1
_t_all=$(date +%s)

# ---------------------------------------------------------------------------
step "the kit answers"
run_in "$SCRATCH" simpleFoam -help 2>&1 | head -3 | grep -q -i "usage" || die "simpleFoam -help did not print a usage line"
run_in "$SCRATCH" foamDictionary -expand "$TUT/incompressible/simpleFoam/pitzDaily/system/fvSchemes" > "$SCRATCH/fvSchemes.expanded"
grep -q "ddtSchemes" "$SCRATCH/fvSchemes.expanded" || die "foamDictionary -expand produced no ddtSchemes"
_mpirun=$(run_in "$SCRATCH" sh -c 'command -v mpirun')
case "$_mpirun" in "$KIT"/*) ;; *) die "mpirun resolves outside the kit: '$_mpirun'" ;; esac

# ---------------------------------------------------------------------------
step "pitzDaily, serial (blockMesh + simpleFoam)"
_t=$(date +%s)
C=$(case_copy incompressible/simpleFoam/pitzDaily pitzDaily)
run_in "$C" blockMesh > "$C/log.blockMesh" 2>&1 || die "blockMesh failed, see $C/log.blockMesh"
run_in "$C" simpleFoam > "$C/log.simpleFoam" 2>&1 || die "simpleFoam failed, see $C/log.simpleFoam"
grep -q "^End" "$C/log.simpleFoam" || die "simpleFoam did not reach End"
grep -q "SIMPLE solution converged" "$C/log.simpleFoam" || warn "pitzDaily did not report convergence (endTime reached instead)"
_serial_time=$(grep -E "^Time = " "$C/log.simpleFoam" | tail -1)
log "  $_serial_time  ($(elapsed "$_t"))"

# ---------------------------------------------------------------------------
step "pitzDaily, four ranks under the bundled mpirun"
_t=$(date +%s)
C=$(case_copy incompressible/simpleFoam/pitzDaily pitzDaily-par)
cat > "$C/system/decomposeParDict" <<'EOF'
FoamFile { version 2.0; format ascii; class dictionary; object decomposeParDict; }
numberOfSubdomains 4;
method scotch;
EOF
run_in "$C" blockMesh > "$C/log.blockMesh" 2>&1 || die "blockMesh failed"
run_in "$C" decomposePar > "$C/log.decomposePar" 2>&1 || die "decomposePar failed, see $C/log.decomposePar"
run_in "$C" mpirun -np 4 simpleFoam -parallel > "$C/log.simpleFoam" 2>&1 || die "parallel simpleFoam failed, see $C/log.simpleFoam"
grep -q "^End" "$C/log.simpleFoam" || die "parallel simpleFoam did not reach End"
run_in "$C" reconstructPar -latestTime > "$C/log.reconstructPar" 2>&1 || die "reconstructPar failed"
_par_time=$(grep -E "^Time = " "$C/log.simpleFoam" | tail -1)
log "  $_par_time  ($(elapsed "$_t"))"
[ "$_serial_time" = "$_par_time" ] || warn "serial and parallel runs ended at different times: '$_serial_time' vs '$_par_time'"

# ---------------------------------------------------------------------------
step "damBreak, short (setFields + interFoam)"
_t=$(date +%s)
C=$(case_copy multiphase/interFoam/laminar/damBreak/damBreak damBreak)
run_in "$C" blockMesh > "$C/log.blockMesh" 2>&1 || die "blockMesh failed"
run_in "$C" setFields > "$C/log.setFields" 2>&1 || die "setFields failed, see $C/log.setFields"
run_in "$C" foamDictionary -entry endTime -set 0.05 system/controlDict > /dev/null 2>&1 || die "foamDictionary -set failed"
run_in "$C" interFoam > "$C/log.interFoam" 2>&1 || die "interFoam failed, see $C/log.interFoam"
grep -q "^End" "$C/log.interFoam" || die "interFoam did not reach End"
log "  $(grep -E "^Time = " "$C/log.interFoam" | tail -1)  ($(elapsed "$_t"))"

# ---------------------------------------------------------------------------
if [ "${LONG:-0}" = "1" ]; then
    step "motorBike (surfaceFeatureExtract, blockMesh, snappyHexMesh on six ranks, potentialFoam, simpleFoam)"
    _t=$(date +%s)
    C=$(case_copy incompressible/simpleFoam/motorBike motorBike)
    cp -R "$C/0.orig" "$C/0"
    run_in "$C" surfaceFeatureExtract > "$C/log.surfaceFeatureExtract" 2>&1 || die "surfaceFeatureExtract failed"
    run_in "$C" blockMesh > "$C/log.blockMesh" 2>&1 || die "blockMesh failed"
    run_in "$C" decomposePar -decomposeParDict system/decomposeParDict.6 > "$C/log.decomposePar" 2>&1 || die "decomposePar failed"
    run_in "$C" mpirun -np 6 snappyHexMesh -overwrite -parallel -decomposeParDict system/decomposeParDict.6 > "$C/log.snappyHexMesh" 2>&1 || die "snappyHexMesh failed, see $C/log.snappyHexMesh"
    run_in "$C" mpirun -np 6 topoSet -parallel -decomposeParDict system/decomposeParDict.6 > "$C/log.topoSet" 2>&1 || die "topoSet failed"
    for _p in "$C"/processor*; do rm -rf "$_p/0"; cp -R "$C/0.orig" "$_p/0"; done
    run_in "$C" mpirun -np 6 potentialFoam -parallel -writephi -decomposeParDict system/decomposeParDict.6 > "$C/log.potentialFoam" 2>&1 || die "potentialFoam failed"
    run_in "$C" mpirun -np 6 simpleFoam -parallel -decomposeParDict system/decomposeParDict.6 > "$C/log.simpleFoam" 2>&1 || die "simpleFoam failed, see $C/log.simpleFoam"
    grep -q "^End" "$C/log.simpleFoam" || die "motorBike simpleFoam did not reach End"
    run_in "$C" reconstructParMesh -constant > "$C/log.reconstructParMesh" 2>&1 || die "reconstructParMesh failed"
    run_in "$C" reconstructPar -latestTime > "$C/log.reconstructPar" 2>&1 || die "reconstructPar failed"
    log "  $(grep -E "^Time = " "$C/log.simpleFoam" | tail -1)  ($(elapsed "$_t"))"
fi

# ---------------------------------------------------------------------------
step "file-system audit: anything written outside the kit and the scratch?"
_offenders="$ROOT/offenders.txt"
find / -xdev -newer "$ROOT/.start-marker" \
    \( -path /proc -o -path /sys -o -path /dev -o -path /run -o -path "$ROOT" \) -prune \
    -o -print 2>/dev/null > "$_offenders" || true
if [ -s "$_offenders" ]; then
    warn "written outside the kit and the scratch:"
    sed 's/^/      /' "$_offenders" >&2
    die "containment audit failed ($(wc -l < "$_offenders" | tr -d ' ') path(s))"
fi
_home_new=$(find "${HOME:-/nonexistent}" -newer "$ROOT/.start-marker" 2>/dev/null | grep -v "^$ROOT" | head -5 || true)
[ -z "$_home_new" ] || die "the run touched the real HOME: $_home_new"

log "verify: OK  ($(elapsed "$_t_all") in total, platform $PLATFORM, kit $(basename "$KIT_ZIP"))"
