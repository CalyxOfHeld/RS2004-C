param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_entity_sync.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/public_chat_loop_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$source="$root/client-c/verification/completion/reference/src/PublicChatOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original public-chat oracle compilation failed.'}
& java -cp "$jar;$OutDir/classes" PublicChatOracle "$root/engine/data/pack/client/wordenc" "$OutDir/public-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original packet-handler expectations failed.'}
if((Get-FileHash "$OutDir/public-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/public-chat-reference.bin").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
$saved=$env:RS2_PUBLIC_CHAT_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_PUBLIC_CHAT_ORACLE="$OutDir/public-reference.bin"
 & $TestExecutable *> "$OutDir/native.log"
 $LASTEXITCODE | Set-Content "$OutDir/native.exit"
 if($LASTEXITCODE -ne 0){throw 'Native packet-state comparison failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch -Pattern '3237 actor/history/position states,2586 history-changing steps')){throw 'Missing current original packet coverage marker.'}
 # Expected caught-decode diagnostics write stderr. PowerShell5 treats that
 # stream as an error record; retain it and judge process exit plus final marker.
 $priorPreference=$ErrorActionPreference
 try {$ErrorActionPreference='Continue'; & $LoopExecutable *> "$OutDir/loop.log"; $loopCode=$LASTEXITCODE}
 finally {$ErrorActionPreference=$priorPreference}
 $loopCode | Set-Content "$OutDir/loop.exit"
 if($loopCode -ne 0){throw 'Candidate loopback dispatch failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch -Pattern 'PASS candidate public dispatch:9 ISAAC-framed packets')){throw 'Missing candidate dispatch coverage marker.'}
 'PASS incoming revision225 public/forced chat:3237 actual original masks,2586 history-changing steps;155 expiry ticks/four actors;9 candidate ISAAC-framed packets plus149 expiry updates. No outgoing/player-overhead rendering/full messaging claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$jar,$source,$PSCommandPath,"$root/engine/data/pack/client/wordenc","$OutDir/public-reference.bin","$root/client-c/src/ui/chatbox.c","$root/client-c/include/ui/chatbox.h","$root/client-c/src/client/client.c","$root/client-c/src/net/entity_sync.c","$root/client-c/include/net/entity_sync.h","$root/client-c/tests/test_entity_sync.c","$root/client-c/verification/completion/tools/public_chat_loop_control.c" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_PUBLIC_CHAT_ORACLE=$saved;Pop-Location}
exit 0
