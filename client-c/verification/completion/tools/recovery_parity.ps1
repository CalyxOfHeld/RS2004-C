param(
 [Parameter(Mandatory=$true)][string]$Log,
 [Parameter(Mandatory=$true)][string]$OutDir,
 [string]$Replay='',
 [string]$StateControls=''
)
# Controlled-I/O comparison: native loss is an explicit original IOException.
# This does not equate Java available() at TCP EOF with a receive exception.
$ErrorActionPreference='Continue'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(-not $env:RS2_PASSWORD){throw 'Set process-only RS2_PASSWORD for the recorded disposable login.'}
if(Test-Path -LiteralPath $OutDir){throw 'Use a new output directory to preserve earlier evidence.'}
$Log=(Resolve-Path -LiteralPath $Log).Path
if(-not $Replay){$Replay="$root/client-c/bin/rs2replay225.exe"}
$Replay=(Resolve-Path -LiteralPath $Replay).Path
if($StateControls){$StateControls=(Resolve-Path -LiteralPath $StateControls).Path}
New-Item -ItemType Directory -Force "$OutDir/classes","$OutDir/native","$OutDir/original" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
'Contract: unchanged original tryReconnect invoked through a controlled receive IOException at recorded native error boundary. Physical TCP EOF timing is not compared. Intermediate P presentations and final frames are separate checks.' | Set-Content -Encoding UTF8 "$OutDir/comparison-contract.txt"
$jar="$root/javaclient/build/libs/rs2client.jar"
$src="$root/client-c/verification/completion/reference/src"
& javac -nowarn -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$src/RecoveryTransport.java" "$src/RecoveryParityOracle.java" "$src/RecoveryOutcomeControls.java" *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Recovery oracle compilation failed.'}
Push-Location "$root/client-c"
try {
 & $Replay $Log --out "$OutDir/native" *> "$OutDir/native/replay.log"
 $LASTEXITCODE | Set-Content "$OutDir/native/replay.exit"
 if($LASTEXITCODE -ne 0){throw 'Strict native replay failed.'}
 $state=(Select-String "$OutDir/native/replay.log" -Pattern '\[PARITY\] login frame=\d+ random_state=(\d+)' | Select-Object -First 1).Matches.Groups[1].Value
 if(-not $state){throw 'Missing initial-login RNG evidence.'}
 $extras=@();if($StateControls){$extras=@('--state-controls',$StateControls)}
 & java -cp "$jar;$OutDir/classes" RecoveryParityOracle $Log "$root/engine/data/pack/client" "$OutDir/original" $state @extras *> "$OutDir/original/oracle.log"
 $LASTEXITCODE | Set-Content "$OutDir/original/oracle.exit"
 if($LASTEXITCODE -ne 0){throw 'Original recovery protocol/presentation/state comparison failed.'}
 & $Replay $Log --out "$OutDir/native" --regions "$OutDir/original/regions.txt" --region-hashes "$OutDir/native/regions.txt" *> "$OutDir/native/regions.log"
 $LASTEXITCODE | Set-Content "$OutDir/native/regions.exit"
 if($LASTEXITCODE -ne 0){throw 'Strict native regional replay failed.'}
 & python "$PSScriptRoot/parity_compare.py" "$OutDir/original/java-regions.txt" "$OutDir/native/regions.txt" "$OutDir/original/java-report.txt" *> "$OutDir/compare.log"
 $LASTEXITCODE | Set-Content "$OutDir/compare.exit"
 if($LASTEXITCODE -ne 0){throw 'Original final-frame comparison failed.'}
 $nativeStates=@(Select-String "$OutDir/native/replay.log" -Pattern '\[PARITY\] login frame=\d+ random_state=(\d+)' | ForEach-Object {$_.Matches.Groups[1].Value})
 $seen=@{};$originalStates=@(Select-String "$OutDir/original/oracle.log" -Pattern '\[ORIGINAL LOGIN RNG\] frame=(\d+) state=(\d+)' | ForEach-Object { $key=$_.Matches.Groups[1].Value;if(-not $seen.ContainsKey($key)){$seen[$key]=$true;$_.Matches.Groups[2].Value} })
 if($originalStates.Count -lt $nativeStates.Count){throw 'Missing per-login RNG comparison.'}
 for($i=0;$i -lt $nativeStates.Count;$i++){if($nativeStates[$i] -ne $originalStates[$i]){throw "Login RNG differs at connection $i"}}
 "PASS: $($nativeStates.Count) login RNG states; strict native replay; original controlled-I/O protocol/intermediate presentations/final frames." | Set-Content -Encoding UTF8 "$OutDir/result.txt"
} finally {Pop-Location}

exit 0
