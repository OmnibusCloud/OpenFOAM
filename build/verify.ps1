# Acceptance of the Windows kit - run where the kit was NOT built.
#
#   powershell -File build\verify.ps1 -Kit .build\out\openfoam-windows-x64.zip
#   ... -Root C:\ofverify        where to unpack and run (default; no spaces, D-16)
#   ... -Long                    also motorBike (snappyHexMesh, six ranks, minutes)
#   ... -InstallMsmpi            CI only: install Microsoft's MS-MPI runtime first
#   ... -AuditWarnOnly           a developer's desktop: report, do not fail, on stray files
#
# The Windows counterpart of build/verify.sh, replaying what a compute node
# does: unpack the zip, set the environment KIT.env says and nothing else
# (plus SystemRoot and the system directories on PATH), HOME and TEMP inside
# the scratch, run. The kit directory is made unwritable for the running user
# first, so a write into it fails the step that tried. Then the two questions:
#
#   1. Does it run?   As shipped (libPstream.dll = the serial one): simpleFoam
#                     on pitzDaily, interFoam on damBreak, foamDictionary.
#                     Where the machine has MS-MPI: the MS-MPI Pstream swapped
#                     in the way the controller does it, pitzDaily on four
#                     ranks under mpiexec; motorBike when asked (-Long).
#   2. Where did it write?   Anything newer than the start marker, owned by
#                     the running user, under the profile, ProgramData or the
#                     Windows temp directory - outside the scratch - fails
#                     the run.
#
# Windows PowerShell 5.1 is enough (no pwsh required).
param(
    [Parameter(Mandatory = $true)] [string] $Kit,
    [string] $Root = "C:\ofverify",
    [switch] $Long,
    [switch] $InstallMsmpi,
    [switch] $AuditWarnOnly
)
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Log([string] $m)  { Write-Host "==> $m" }
function Warn([string] $m) { Write-Host "--> $m" -ForegroundColor Yellow }
function Die([string] $m)  { Write-Host "*** $m" -ForegroundColor Red; exit 1 }
function Elapsed([datetime] $t) { "{0} s" -f [int]((Get-Date) - $t).TotalSeconds }

if ($Root -match " ") { Die "OpenFOAM cannot work under a path with a space in it - choose another -Root" }
if (-not (Test-Path $Kit)) { Die "no kit archive at $Kit" }
$Kit = (Resolve-Path $Kit).Path
$Me = [System.Security.Principal.WindowsIdentity]::GetCurrent().Name
$RepoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)

# ---------------------------------------------------------------------------
# MS-MPI: the node's own. On a CI runner it is installed here, pinned by the
# checksum build/config.sh records; on a developer's machine nothing is ever
# installed, and the parallel steps are skipped with a warning.
# ---------------------------------------------------------------------------
function Get-ConfigValue([string] $name) {
    $line = Get-Content (Join-Path $RepoRoot "build\config.sh") | Where-Object { $_ -match "^$name=" } | Select-Object -First 1
    if (-not $line) { Die "build/config.sh has no $name" }
    return ($line -replace "^$name=", "") -replace "\s+#.*$", ""
}
if ($InstallMsmpi) {
    $url = Get-ConfigValue "MSMPI_SETUP_URL"; $sha = Get-ConfigValue "MSMPI_SETUP_SHA256"
    $setup = Join-Path $env:TEMP "msmpisetup.exe"
    Log "installing MS-MPI ($url)"
    Invoke-WebRequest -Uri $url -OutFile $setup -UseBasicParsing
    $got = (Get-FileHash $setup -Algorithm SHA256).Hash.ToLower()
    if ($got -ne $sha.ToLower()) { Die "msmpisetup.exe checksum $got, expected $sha" }
    $p = Start-Process -FilePath $setup -ArgumentList "-unattend", "-force" -Wait -PassThru
    if ($p.ExitCode -ne 0) { Die "msmpisetup.exe exited $($p.ExitCode)" }
}
$MsmpiBin = [Environment]::GetEnvironmentVariable("MSMPI_BIN", "Machine")
$MsmpiDll = Join-Path $env:SystemRoot "System32\msmpi.dll"
$HaveMsmpi = ($MsmpiBin) -and (Test-Path (Join-Path $MsmpiBin "mpiexec.exe")) -and (Test-Path $MsmpiDll)
if ($HaveMsmpi) { Log "MS-MPI present: $MsmpiBin" } else { Warn "no MS-MPI on this machine: the parallel steps are skipped, the serial Pstream is what gets verified" }

# ---------------------------------------------------------------------------
# Unpack, protect, environment
# ---------------------------------------------------------------------------
# The deny is set through .NET with the write and delete bits alone. icacls
# adds SYNCHRONIZE to every mask it writes - and every ordinary open asks for
# SYNCHRONIZE, so a kit denied through icacls could neither be read nor run
# ("Access is denied" on the first executable, the first two local runs).
# One inheritable rule on the kit root reaches every file below it.
$DenyRights = [System.Security.AccessControl.FileSystemRights]"WriteData, AppendData, WriteExtendedAttributes, WriteAttributes, DeleteSubdirectoriesAndFiles, Delete"
$DenyRule = New-Object System.Security.AccessControl.FileSystemAccessRule($Me, $DenyRights, "ContainerInherit, ObjectInherit", "None", "Deny")
function Protect-Kit {
    $acl = Get-Acl -LiteralPath $KitDir
    $acl.AddAccessRule($DenyRule)
    Set-Acl -LiteralPath $KitDir -AclObject $acl
}
function Unprotect-Kit {
    if (-not (Test-Path -LiteralPath $KitDir)) { return }
    $acl = Get-Acl -LiteralPath $KitDir
    $null = $acl.RemoveAccessRule($DenyRule)
    Set-Acl -LiteralPath $KitDir -AclObject $acl
}

$KitDir  = Join-Path $Root "kit"
$Scratch = Join-Path $Root "scratch"
if (Test-Path $Root) { Unprotect-Kit; Remove-Item -Recurse -Force $Root }
New-Item -ItemType Directory -Force $KitDir, (Join-Path $Scratch "home"), (Join-Path $Scratch "tmp"), (Join-Path $Scratch "cases") | Out-Null

Log "unpacking $(Split-Path -Leaf $Kit) under '$KitDir'"
Expand-Archive -LiteralPath $Kit -DestinationPath $KitDir
$KitFolder = Join-Path $KitDir "openfoam\windows-x64"
$KitEnvFile = Join-Path $KitFolder "KIT.env"
if (-not (Test-Path $KitEnvFile)) { Die "the archive carries no KIT.env" }

$FoamEnv = @{}
foreach ($line in Get-Content $KitEnvFile) {
    if ($line -match "^\s*#" -or $line -notmatch "=") { continue }
    $name, $value = $line -split "=", 2
    $FoamEnv[$name] = $value.Replace("@KIT@", $KitFolder).Replace("@SCRATCH@", $Scratch)
}
foreach ($k in "PATH", "WM_PROJECT_DIR", "HOME", "TEMP", "KIT_PSTREAM_TARGET", "KIT_PSTREAM_MSMPI") {
    if (-not $FoamEnv.ContainsKey($k)) { Die "KIT.env names no $k" }
}
# What the controller adds: the system directories and SystemRoot.
$FoamEnv["PATH"] = "$($FoamEnv['PATH']);$env:SystemRoot\System32;$env:SystemRoot"
$FoamEnv["SystemRoot"] = $env:SystemRoot
if ($HaveMsmpi) { $FoamEnv["MSMPI_BIN"] = $MsmpiBin }
$Bin = $FoamEnv["FOAM_APPBIN"]
if (-not (Test-Path (Join-Path $Bin "simpleFoam.exe"))) { Die "FOAM_APPBIN has no simpleFoam.exe: $Bin" }
$Tut = Join-Path $FoamEnv["WM_PROJECT_DIR"] "tutorials"

Protect-Kit

# Run-In <case dir> <exe> <args...>: the environment is KIT.env and nothing
# else; returns the exit code; output to the log the caller names.
function Run-In([string] $cwd, [string] $log, [string] $exe, [string[]] $argv) {
    if ($exe -eq "mpiexec") { $file = Join-Path $MsmpiBin "mpiexec.exe" } else { $file = Join-Path $Bin "$exe.exe" }
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $file
    $psi.Arguments = ($argv -join " ")
    $psi.WorkingDirectory = $cwd
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.Environment.Clear()
    foreach ($k in $FoamEnv.Keys) { $psi.Environment[$k] = $FoamEnv[$k] }
    $psi.Environment["FOAM_CASE"] = $cwd
    $p = [System.Diagnostics.Process]::Start($psi)
    $out = $p.StandardOutput.ReadToEndAsync(); $err = $p.StandardError.ReadToEndAsync()
    $p.WaitForExit()
    [System.IO.File]::WriteAllText($log, $out.Result + $err.Result)
    return $p.ExitCode
}
function Step-Run([string] $cwd, [string] $log, [string] $exe, [string[]] $argv) {
    $code = Run-In $cwd $log $exe $argv
    if ($code -ne 0) {
        Warn ("failed: {0} {1} (exit {2}, 0x{2:X8}) - last lines of {3}:" -f $exe, ($argv -join " "), $code, $log)
        Get-Content $log -Tail 30 | ForEach-Object { Write-Host "      $_" }
        Die "step failed"
    }
}
function Copy-Case([string] $tutorial, [string] $name) {
    $c = Join-Path $Scratch "cases\$name"
    if (Test-Path $c) { Remove-Item -Recurse -Force $c }
    Copy-Item -Recurse (Join-Path $Tut $tutorial) $c
    Get-ChildItem -Recurse $c | ForEach-Object { $_.Attributes = $_.Attributes -band (-bnot [IO.FileAttributes]::ReadOnly) }
    if ((Test-Path (Join-Path $c "0.orig")) -and -not (Test-Path (Join-Path $c "0"))) { Copy-Item -Recurse (Join-Path $c "0.orig") (Join-Path $c "0") }
    return $c
}
function Last-Time([string] $log) { (Get-Content $log | Select-String -Pattern "^Time = " | Select-Object -Last 1).Line }
function Has-Line([string] $log, [string] $pattern) { [bool](Get-Content $log | Select-String -Pattern $pattern -Quiet) }
function Step([string] $m) { Log "step: $m" }

$Marker = Get-Date
Start-Sleep -Milliseconds 1100
$TAll = Get-Date

# ---------------------------------------------------------------------------
Step "the kit answers (serial Pstream, as shipped)"
$null = Run-In $Scratch (Join-Path $Scratch "help.log") "simpleFoam" @("-help")
if (-not (Has-Line (Join-Path $Scratch "help.log") "Usage")) { Get-Content (Join-Path $Scratch "help.log") -Tail 10 | ForEach-Object { Write-Host "      $_" }; Die "simpleFoam -help did not print a usage line" }
Step-Run $Scratch (Join-Path $Scratch "fvSchemes.expanded") "foamDictionary" @("-expand", (Join-Path $Tut "incompressible\simpleFoam\pitzDaily\system\fvSchemes"))
if (-not (Has-Line (Join-Path $Scratch "fvSchemes.expanded") "ddtSchemes")) { Die "foamDictionary -expand produced no ddtSchemes" }

# ---------------------------------------------------------------------------
Step "pitzDaily, serial (blockMesh + simpleFoam)"
$t = Get-Date
$C = Copy-Case "incompressible\simpleFoam\pitzDaily" "pitzDaily"
Step-Run $C "$C\log.blockMesh"  "blockMesh"  @()
Step-Run $C "$C\log.simpleFoam" "simpleFoam" @()
if (-not (Has-Line "$C\log.simpleFoam" "^End")) { Die "pitzDaily simpleFoam did not reach End" }
if (-not (Has-Line "$C\log.simpleFoam" "SIMPLE solution converged")) { Warn "pitzDaily did not report convergence (endTime reached instead)" }
$SerialTime = Last-Time "$C\log.simpleFoam"
Log "  $SerialTime  ($(Elapsed $t))"

# ---------------------------------------------------------------------------
Step "damBreak, short (setFields + interFoam)"
$t = Get-Date
$C = Copy-Case "multiphase\interFoam\laminar\damBreak\damBreak" "damBreak"
Step-Run $C "$C\log.blockMesh"      "blockMesh"      @()
Step-Run $C "$C\log.setFields"      "setFields"      @()
Step-Run $C "$C\log.foamDictionary" "foamDictionary" @("-entry", "endTime", "-set", "0.05", "system/controlDict")
Step-Run $C "$C\log.interFoam"      "interFoam"      @()
if (-not (Has-Line "$C\log.interFoam" "^End")) { Die "damBreak interFoam did not reach End" }
Log "  $(Last-Time "$C\log.interFoam")  ($(Elapsed $t))"

# ---------------------------------------------------------------------------
if ($HaveMsmpi) {
    Step "the MS-MPI Pstream swapped in (what the controller does once per install)"
    Unprotect-Kit
    Copy-Item $FoamEnv["KIT_PSTREAM_MSMPI"] $FoamEnv["KIT_PSTREAM_TARGET"] -Force
    Protect-Kit

    Step "pitzDaily, four ranks under the node's mpiexec"
    $t = Get-Date
    $C = Copy-Case "incompressible\simpleFoam\pitzDaily" "pitzDaily-par"
    [System.IO.File]::WriteAllText("$C\system\decomposeParDict", "FoamFile { version 2.0; format ascii; class dictionary; object decomposeParDict; }`nnumberOfSubdomains 4;`nmethod scotch;`n")
    Step-Run $C "$C\log.blockMesh"      "blockMesh"    @()
    Step-Run $C "$C\log.decomposePar"   "decomposePar" @()
    Step-Run $C "$C\log.simpleFoam"     "mpiexec"      @("-n", "4", (Join-Path $Bin "simpleFoam.exe"), "-parallel")
    if (-not (Has-Line "$C\log.simpleFoam" "^End")) { Die "parallel pitzDaily did not reach End" }
    Step-Run $C "$C\log.reconstructPar" "reconstructPar" @("-latestTime")
    $ParTime = Last-Time "$C\log.simpleFoam"
    if ($SerialTime -ne $ParTime) { Warn "serial and parallel runs ended at different times: '$SerialTime' vs '$ParTime'" }
    Log "  $ParTime  ($(Elapsed $t))"
}

# ---------------------------------------------------------------------------
if ($Long) {
    if (-not $HaveMsmpi) {
        Warn "motorBike needs the parallel path (snappyHexMesh on six ranks): skipped without MS-MPI"
    } else {
        Step "motorBike (surfaceFeatureExtract, blockMesh, snappyHexMesh on six ranks, potentialFoam, simpleFoam)"
        $t = Get-Date
        $C = Copy-Case "incompressible\simpleFoam\motorBike" "motorBike"
        New-Item -ItemType Directory -Force "$C\constant\triSurface" | Out-Null
        Copy-Item (Join-Path $Tut "resources\geometry\motorBike.obj.gz") "$C\constant\triSurface\"
        $dd = @("-decomposeParDict", "system/decomposeParDict.6")
        Step-Run $C "$C\log.surfaceFeatureExtract" "surfaceFeatureExtract" @()
        Step-Run $C "$C\log.blockMesh"             "blockMesh"             @()
        Step-Run $C "$C\log.decomposePar"          "decomposePar"          $dd
        Step-Run $C "$C\log.snappyHexMesh"         "mpiexec" (@("-n", "6", (Join-Path $Bin "snappyHexMesh.exe"), "-overwrite", "-parallel") + $dd)
        Step-Run $C "$C\log.topoSet"               "mpiexec" (@("-n", "6", (Join-Path $Bin "topoSet.exe"), "-parallel") + $dd)
        foreach ($p in Get-ChildItem -Directory $C -Filter "processor*") {
            if (Test-Path "$($p.FullName)\0") { Remove-Item -Recurse -Force "$($p.FullName)\0" }
            Copy-Item -Recurse "$C\0.orig" "$($p.FullName)\0"
        }
        Step-Run $C "$C\log.potentialFoam"         "mpiexec" (@("-n", "6", (Join-Path $Bin "potentialFoam.exe"), "-parallel", "-writephi") + $dd)
        Step-Run $C "$C\log.simpleFoam"            "mpiexec" (@("-n", "6", (Join-Path $Bin "simpleFoam.exe"), "-parallel") + $dd)
        if (-not (Has-Line "$C\log.simpleFoam" "^End")) { Die "motorBike simpleFoam did not reach End" }
        Step-Run $C "$C\log.reconstructParMesh"    "reconstructParMesh"    @("-constant")
        Step-Run $C "$C\log.reconstructPar"        "reconstructPar"        @("-latestTime")
        Log "  $(Last-Time "$C\log.simpleFoam")  ($(Elapsed $t))"
    }
}

# ---------------------------------------------------------------------------
Step "file-system audit: anything written outside the kit and the scratch?"
# Only files owned by the running user count (the operating system's services
# write as SYSTEM), newer than the marker, outside the acceptance root. The
# places a Windows program writes to on its own are scanned: the profile,
# ProgramData, the Windows temp directory.
$roots = @($env:USERPROFILE, $env:ProgramData, (Join-Path $env:SystemRoot "Temp"), $env:PUBLIC) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -Unique
$rootPrefix = (Resolve-Path $Root).Path.TrimEnd("\") + "\"
# Subtrees that are never the kit's doing and are large: the OS's own caches
# and application data under the profile, package caches, and the
# development tools' state. Left out of the walk, not of the verdict - a
# file the kit wrote anywhere else is still reported.
$skipSubtrees = @(
    "AppData\Local\Packages", "AppData\Local\Microsoft", "AppData\LocalLow", "AppData\Roaming\Microsoft",
    "AppData\Local\Temp\DiagOutputDir", "AppData\Local\Docker", "AppData\Local\Google", "AppData\Local\Programs",
    ".nuget", ".dotnet", ".vscode", ".claude", ".cache",
    "Microsoft", "Package Cache", "Packages", "regid.1991-06.com.microsoft"
) | ForEach-Object { $_.ToLowerInvariant() }
function Walk-Tree([string] $dir, [string] $base, [System.Collections.Generic.List[string]] $hits) {
    $rel = if ($dir.Length -gt $base.Length) { $dir.Substring($base.Length + 1).ToLowerInvariant() } else { "" }
    if ($rel -ne "" -and ($skipSubtrees -contains $rel)) { return }
    if ($dir.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) { return }
    $entries = $null
    try { $entries = [System.IO.Directory]::EnumerateFileSystemEntries($dir) } catch { return }
    foreach ($entry in $entries) {
        try {
            $info = [System.IO.FileInfo]::new($entry)
            $attributes = $info.Attributes
            if ($attributes -band [System.IO.FileAttributes]::ReparsePoint) { continue }
            if ($attributes -band [System.IO.FileAttributes]::Directory) { Walk-Tree $entry $base $hits; continue }
            if ($info.LastWriteTime -gt $Marker -or $info.CreationTime -gt $Marker) { $hits.Add($entry) }
        } catch { }
    }
}
$candidates = [System.Collections.Generic.List[string]]::new()
foreach ($r in $roots) { Walk-Tree (Resolve-Path $r).Path.TrimEnd("\") (Resolve-Path $r).Path.TrimEnd("\") $candidates }
$offenders = @($candidates | Where-Object { try { (Get-Acl -LiteralPath $_).Owner -eq $Me } catch { $false } })
if ($offenders.Count -gt 0) {
    Warn "files written outside the kit and the scratch: $($offenders.Count)"
    $offenders | Select-Object -First 40 | ForEach-Object { Write-Host "      $_" }
    if (-not $AuditWarnOnly) { Die "containment violated" } else { Warn "(-AuditWarnOnly: not failing the run)" }
}

Unprotect-Kit
Log "verify: OK  ($(Elapsed $TAll) in total, platform windows-x64, kit $(Split-Path -Leaf $Kit), MS-MPI $(if ($HaveMsmpi) { 'verified' } else { 'not on this machine' }))"
exit 0
