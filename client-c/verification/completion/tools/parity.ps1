param(
    [Parameter(Mandatory = $true)][string]$Log,      # session log recorded with rs2client --record --trace
    [Parameter(Mandatory = $true)][string]$OutDir,
    [int[]]$DumpFrame = @(),
    [string]$Replay = ""
)
# Differential parity run: the C client (rs2replay) and the unchanged Java
# client (ParityOracle on javaclient/build/libs/rs2client.jar) consume the same
# recorded session; outgoing bytes, inbound consumption and per-region frame
# hashes are compared. RS2_PASSWORD must be set for sessions with a login.
# Native stderr must not become a terminating error (PowerShell 5.1); exit codes are checked.
$ErrorActionPreference = "Continue"
if ((Select-String -Path $Log -Pattern "^L " -Quiet) -and -not $env:RS2_PASSWORD) { throw "the log has a login: set RS2_PASSWORD (it is not recorded)" }
$root = (Resolve-Path "$PSScriptRoot\..\..\..\..").Path
$jar = "$root\javaclient\build\libs\rs2client.jar"
$classes = "$OutDir\oracle-classes"
if (-not $Replay) { $Replay = "$root\client-c\bin\rs2replay.exe" }
$replay = (Resolve-Path -LiteralPath $Replay).Path
New-Item -ItemType Directory -Force "$OutDir\c", "$OutDir\java", $classes | Out-Null
$Log = (Resolve-Path $Log).Path; $OutDir = (Resolve-Path $OutDir).Path; $classes = "$OutDir\oracle-classes"
# The C client resolves the cache relative to its working directory.
Push-Location "$root\client-c"

& javac -nowarn -encoding UTF-8 -cp $jar -d $classes "$root\client-c\verification\completion\reference\src\ParityOracle.java"
if ($LASTEXITCODE -ne 0) { throw "javac failed" }

# 1. C replay: verifies the log and reports the Math.random state at login.
& $replay $Log --out "$OutDir\c" *> "$OutDir\c\replay-state.log"
if ($LASTEXITCODE -ne 0) { throw "C replay failed; see $OutDir\c\replay-state.log" }
$login = Select-String -Path "$OutDir\c\replay-state.log" -Pattern "\[PARITY\] login" | Select-Object -First 1
if (-not $login) { throw "no [PARITY] login line: record with --trace and a login" }
$state = $login.Line.Split("=")[-1].Trim()

# 2. Java oracle.
$dumps = @(); foreach ($f in $DumpFrame) { $dumps += @("--dump-frame", "$f") }
& java -cp "$jar;$classes" ParityOracle $Log "$root\engine\data\pack\client" "$OutDir\java" $state @dumps *> "$OutDir\java\oracle.log"
$javaExit = $LASTEXITCODE

# 3. C region hashes over the rectangles the Java client drew.
$cdumps = @(); foreach ($f in $DumpFrame) { $cdumps += @("--dump-frame", "$f") }
& $replay $Log --out "$OutDir\c" --regions "$OutDir\java\regions.txt" --region-hashes "$OutDir\c\c-regions.txt" @cdumps *> "$OutDir\c\replay-regions.log"
if ($LASTEXITCODE -ne 0) { throw "C replay with regions failed" }

# 4. Compare.
& python "$root\client-c\verification\completion\tools\parity_compare.py" "$OutDir\java\java-regions.txt" "$OutDir\c\c-regions.txt" "$OutDir\java\java-report.txt" | Tee-Object -FilePath "$OutDir\parity-summary.txt"
$compareExit = $LASTEXITCODE
Pop-Location
if ($javaExit -ne 0 -or $compareExit -ne 0) { exit 1 }
exit 0
