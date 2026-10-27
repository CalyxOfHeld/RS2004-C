param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='', [string]$FailureExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/public_send_loop_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path
if(-not $FailureExecutable){$FailureExecutable="$root/client-c/bin/public_send_failure_control.exe"}
$FailureExecutable=(Resolve-Path -LiteralPath $FailureExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$source="$root/client-c/verification/completion/reference/src/PublicSendOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original public-chat oracle compilation failed.'}
& java -cp "$jar;$OutDir/classes" PublicSendOracle "$root/engine/data/pack/client/wordenc" "$OutDir/send-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original packet-handler expectations failed.'}
if((Get-FileHash "$OutDir/send-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/public-send-reference.bin").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
$saved=$env:RS2_PUBLIC_SEND_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_PUBLIC_SEND_ORACLE="$OutDir/send-reference.bin"
 & $TestExecutable *> "$OutDir/native.log"
 $LASTEXITCODE | Set-Content "$OutDir/native.exit"
 if($LASTEXITCODE -ne 0){throw 'Native packet-state comparison failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch -Pattern '3521 key cases,3295 wire-producing cases')){throw 'Missing current original packet coverage marker.'}
 # Preserve native stderr. PowerShell5 treats that
 # stream as an error record; retain it and judge process exit plus final marker.
 $priorPreference=$ErrorActionPreference
 try {$ErrorActionPreference='Continue'; & $LoopExecutable "$OutDir/send-reference.bin" *> "$OutDir/loop.log"; $loopCode=$LASTEXITCODE}
 finally {$ErrorActionPreference=$priorPreference}
 $loopCode | Set-Content "$OutDir/loop.exit"
 if($loopCode -ne 0){throw 'Candidate loopback dispatch failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch -Pattern 'PASS candidate public send:3521 deferred event-phase cases')){throw 'Missing candidate dispatch coverage marker.'}
 & $FailureExecutable *> "$OutDir/failure.log"
 $LASTEXITCODE | Set-Content "$OutDir/failure.exit"
 if($LASTEXITCODE -ne 0){throw 'Native public-send failure control failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/failure.log" -SimpleMatch -Pattern '2 UI allocation failures,88 insufficient capacities')){throw 'Missing native failure coverage marker.'}
 'PASS outgoing revision225 public chat:3521 actual original key cases,3295 wire-producing cases; exact encrypted bytes/cipher/history/actor/input/modes;3295 candidate loopback transmissions,last through full update; native allocation/capacity/failure safety. Controlled sender acceptance; no production conversation/player-overhead/private-send claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$FailureExecutable,$jar,$source,$PSCommandPath,"$root/engine/data/pack/client/wordenc","$OutDir/send-reference.bin","$root/client-c/src/ui/chatbox.c","$root/client-c/include/ui/chatbox.h","$root/client-c/src/client/client.c","$root/client-c/src/net/protocol.c","$root/client-c/include/net/protocol.h","$root/client-c/tests/test_chatbox.c","$root/client-c/verification/completion/tools/public_send_loop_control.c","$root/client-c/verification/completion/tools/public_send_failure_control.c" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_PUBLIC_SEND_ORACLE=$saved;Pop-Location}
exit 0
