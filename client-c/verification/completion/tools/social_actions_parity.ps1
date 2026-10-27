param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='', [string]$FailureExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/social_actions_loop_control.exe"}
if(-not $FailureExecutable){$FailureExecutable="$root/client-c/bin/social_actions_failure_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path;$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path;$FailureExecutable=(Resolve-Path -LiteralPath $FailureExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar"
foreach($name in @('SocialActionsOracle','SocialPromptOracle')) {
 & javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/$name.java" *> "$OutDir/$name-build.log"
 $LASTEXITCODE | Set-Content "$OutDir/$name-build.exit"
 if($LASTEXITCODE -ne 0){throw "Original $name compilation failed."}
}
& java -cp "$jar;$OutDir/classes" SocialActionsOracle "$OutDir/social-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original social action capture failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" SocialPromptOracle "$root/engine/data/pack/client" "$OutDir/social-ui-reference.bin" *> "$OutDir/ui.log"
$LASTEXITCODE | Set-Content "$OutDir/ui.exit"
if($LASTEXITCODE -ne 0){throw 'Original social UI capture failed.'}
foreach($fixture in @('social-reference.bin','social-ui-reference.bin')){if((Get-FileHash "$OutDir/$fixture").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$fixture").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}}
$saved=$env:RS2_SOCIAL_ORACLE;$savedUI=$env:RS2_SOCIAL_UI_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_SOCIAL_ORACLE="$OutDir/social-reference.bin";$env:RS2_SOCIAL_UI_ORACLE="$OutDir/social-ui-reference.bin"
 foreach($check in @(@{name='native';exe=$TestExecutable;args=@()},@{name='loop';exe=$LoopExecutable;args=@("$OutDir/social-reference.bin")},@{name='failure';exe=$FailureExecutable;args=@()})) {
  $prior=$ErrorActionPreference
  try {$ErrorActionPreference='Continue';$arguments=$check.args;& $check.exe @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE} finally {$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit"
  if($code -ne 0){throw "Social $($check.name) comparison failed."}
 }
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS social original:3266 direct/menu/prompt cases,758 wire cases,532 exact error messages')){throw 'Missing original action coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS social UI original:404 dynamic ignore rows,104 menu options,24 prompt images/1103616 pixels')){throw 'Missing original UI coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch '4 actual friend/ignore tab button clicks/full prompt images')){throw 'Missing actual candidate integration.'}
 if(-not (Select-String -LiteralPath "$OutDir/failure.log" -SimpleMatch '36 capacity rejections,4 exact9-byte successes,4 error-UI allocation faults')){throw 'Missing native fault controls.'}
 'PASS controlled revision225 friend/ignore actions:3266 original cases,758 candidate transmissions,532 errors;all100 tails/history/full cipher/prompt/input;404 ignore rows/104 menus/24 prompt images;4 actual cached tab button clicks/full prompt pixels;failure-safe list/history/cipher/output. No production persistence/conversation/report-abuse claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$FailureExecutable,$jar,$PSCommandPath,"$OutDir/social-reference.bin","$OutDir/social-ui-reference.bin","$root/engine/data/pack/client/title","$root/engine/data/pack/client/media","$root/client-c/src/ui/chatbox.c","$root/client-c/src/ui/context_menu.c","$root/client-c/src/ui/client_actions.c","$root/client-c/src/net/protocol.c","$root/client-c/src/client/client.c","$root/client-c/tests/test_chatbox.c","$root/client-c/verification/completion/reference/src/SocialActionsOracle.java","$root/client-c/verification/completion/reference/src/SocialPromptOracle.java" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_SOCIAL_ORACLE=$saved;$env:RS2_SOCIAL_UI_ORACLE=$savedUI;Pop-Location}
exit 0
