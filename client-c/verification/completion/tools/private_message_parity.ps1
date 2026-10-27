param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/private_message_loop_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$source="$root/client-c/verification/completion/reference/src/PrivateMessageOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original private-message oracle compilation failed.'}
& java -cp "$jar;$OutDir/classes" PrivateMessageOracle "$root/engine/data/pack/client/wordenc" "$OutDir/private-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original packet-handler expectations failed.'}
if((Get-FileHash "$OutDir/private-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/private-message-reference.bin").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
$saved=$env:RS2_PRIVATE_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_PRIVATE_ORACLE="$OutDir/private-reference.bin"
 & $TestExecutable *> "$OutDir/native.log"
 $LASTEXITCODE | Set-Content "$OutDir/native.exit"
 if($LASTEXITCODE -ne 0){throw 'Native packet-state comparison failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch -Pattern '2634 packet states,2520 history additions,1 caught decode failure')){throw 'Missing current original packet coverage marker.'}
 & $LoopExecutable *> "$OutDir/loop.log"
 $LASTEXITCODE | Set-Content "$OutDir/loop.exit"
 if($LASTEXITCODE -ne 0){throw 'Candidate loopback dispatch failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch -Pattern 'PASS candidate private/ignore dispatch:8 ISAAC-framed loopback packets')){throw 'Missing candidate dispatch coverage marker.'}
 'PASS incoming revision225 private messages/ignore updates:2634 actual original packet states,2520 history additions,1 caught decode failure;8 candidate ISAAC-framed isolated loopback packets. No outgoing/full messaging claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$jar,$source,$PSCommandPath,"$root/engine/data/pack/client/wordenc","$OutDir/private-reference.bin","$root/client-c/src/ui/chatbox.c","$root/client-c/include/ui/chatbox.h","$root/client-c/src/client/client.c","$root/client-c/tests/test_chatbox.c","$root/client-c/verification/completion/tools/private_message_loop_control.c" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_PRIVATE_ORACLE=$saved;Pop-Location}
exit 0
