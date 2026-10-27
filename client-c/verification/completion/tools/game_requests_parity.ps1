param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='', [string]$FailureExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/game_requests_loop_control.exe"}
if(-not $FailureExecutable){$FailureExecutable="$root/client-c/bin/game_requests_failure_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path;$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path;$FailureExecutable=(Resolve-Path -LiteralPath $FailureExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar";$source="$root/client-c/verification/completion/reference/src/GameRequestsOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit"
if($LASTEXITCODE -ne 0){throw 'Original game-request oracle compilation failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" GameRequestsOracle "$OutDir/game-requests-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original packet4 capture failed.'}
if((Get-FileHash "$OutDir/game-requests-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/game-requests-reference.bin").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
$saved=$env:RS2_GAME_REQUEST_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_GAME_REQUEST_ORACLE="$OutDir/game-requests-reference.bin"
 foreach($check in @(@{name='native';exe=$TestExecutable;args=@()},@{name='loop';exe=$LoopExecutable;args=@("$OutDir/game-requests-reference.bin")},@{name='failure';exe=$FailureExecutable;args=@()})) {
  $prior=$ErrorActionPreference
  try {$ErrorActionPreference='Continue';$arguments=$check.args;& $check.exe @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE} finally {$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit"
  if($code -ne 0){throw "Game-request $($check.name) comparison failed."}
 }
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS game requests original:2073 packet4 cases,1385 additions,294 trades,286 duels')){throw 'Missing original history coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch '21 fragmented headers;draw-phase region override,malformed rejection/recovery')){throw 'Missing actual candidate integration.'}
 if(-not (Select-String -LiteralPath "$OutDir/failure.log" -SimpleMatch '6 allocation failures,7 malformed/metadata guards')){throw 'Missing native fault controls.'}
 'PASS controlled revision225 game-request admission:2073 original/candidate framed states,1385 additions,294 trades,286 duels;all100 history slots,ignore/override/raw sender/text/type and modal state;21 fragmented headers,region draw phase,malformed recovery/allocation safety. No production delivery or trade/duel execution claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$FailureExecutable,$jar,$source,$PSCommandPath,"$OutDir/game-requests-reference.bin","$root/engine/data/pack/client/title","$root/engine/data/pack/client/media","$root/engine/data/pack/client/interface","$root/client-c/src/ui/chatbox.c","$root/client-c/include/ui/chatbox.h","$root/client-c/src/client/client.c","$root/client-c/tests/test_chatbox.c" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_GAME_REQUEST_ORACLE=$saved;Pop-Location}
exit 0
