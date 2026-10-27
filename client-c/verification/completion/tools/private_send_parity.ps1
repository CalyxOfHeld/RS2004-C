param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='', [string]$FailureExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/private_send_loop_control.exe"}
if(-not $FailureExecutable){$FailureExecutable="$root/client-c/bin/private_send_failure_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path
$FailureExecutable=(Resolve-Path -LiteralPath $FailureExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
foreach($oracle in @(@{class='PrivateSendOracle';fixture='private-send-reference.bin';input='wordenc'},@{class='PrivatePromptOracle';fixture='private-prompt-reference.bin';input=''})) {
 $name=$oracle.class
 & javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/$name.java" *> "$OutDir/$name-build.log"
 $LASTEXITCODE | Set-Content "$OutDir/$name-build.exit"
 if($LASTEXITCODE -ne 0){throw "Original $name compilation failed."}
 & java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" $name "$root/engine/data/pack/client/$($oracle.input)" "$OutDir/$($oracle.fixture)" *> "$OutDir/$name.log"
 $LASTEXITCODE | Set-Content "$OutDir/$name.exit"
 if($LASTEXITCODE -ne 0){throw "Original $name capture failed."}
 if((Get-FileHash "$OutDir/$($oracle.fixture)").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($oracle.fixture)").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
}
$savedSend=$env:RS2_PRIVATE_SEND_ORACLE;$savedPrompt=$env:RS2_PRIVATE_PROMPT_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_PRIVATE_SEND_ORACLE="$OutDir/private-send-reference.bin";$env:RS2_PRIVATE_PROMPT_ORACLE="$OutDir/private-prompt-reference.bin"
 foreach($check in @(@{name='native';exe=$TestExecutable;args=@()},@{name='loop';exe=$LoopExecutable;args=@("$OutDir/private-send-reference.bin")},@{name='failure';exe=$FailureExecutable;args=@()})) {
  $prior=$ErrorActionPreference
  try {$ErrorActionPreference='Continue';$arguments=$check.args;& $check.exe @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE} finally {$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit"
  if($code -ne 0){throw "Private-send $($check.name) control failed."}
 }
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS private send original:1987 menu/input cases,1746 wire-producing cases')){throw 'Missing original sender coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS private UI original:804 dynamic friend rows,416 menu options,24 prompt images/1103616 pixels')){throw 'Missing original UI coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch 'actual friend-tab click/full prompt pixels; incoming243 replaces prompt')){throw 'Missing actual candidate integration coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/failure.log" -SimpleMatch '2 UI allocation failures,94 insufficient capacities,exact94-byte combined148/244 success')){throw 'Missing native failure coverage.'}
 'PASS controlled private sender:1987 actual original menu/input cases,1746 candidate loopback transmissions; complete cipher/history/prompt/privacy;804 friend rows/416 menus/24 prompt images;actual candidate friend click/full prompt pixels/incoming243;allocation/capacity/write failures. No real-account delivery or complete social-action claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$FailureExecutable,$jar,$PSCommandPath,"$OutDir/private-send-reference.bin","$OutDir/private-prompt-reference.bin","$root/engine/data/pack/client/title","$root/engine/data/pack/client/media","$root/engine/data/pack/client/wordenc","$root/client-c/verification/completion/reference/src/PrivateSendOracle.java","$root/client-c/verification/completion/reference/src/PrivatePromptOracle.java","$root/client-c/src/ui/chatbox.c","$root/client-c/src/ui/component.c","$root/client-c/src/ui/client_actions.c","$root/client-c/src/net/protocol.c","$root/client-c/src/client/client.c","$root/client-c/tests/test_chatbox.c" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_PRIVATE_SEND_ORACLE=$savedSend;$env:RS2_PRIVATE_PROMPT_ORACLE=$savedPrompt;Pop-Location}
exit 0
