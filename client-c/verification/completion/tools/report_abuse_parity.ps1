param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$LoopExecutable='', [string]$FailureExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $LoopExecutable){$LoopExecutable="$root/client-c/bin/report_abuse_loop_control.exe"}
if(-not $FailureExecutable){$FailureExecutable="$root/client-c/bin/report_abuse_failure_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path;$LoopExecutable=(Resolve-Path -LiteralPath $LoopExecutable).Path;$FailureExecutable=(Resolve-Path -LiteralPath $FailureExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/ReportAbuseOracle.java" *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit"
if($LASTEXITCODE -ne 0){throw 'Original report oracle compilation failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" ReportAbuseOracle "$root/engine/data/pack/client" "$OutDir/report-reference.bin" "$OutDir/report-ui-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original report capture failed.'}
foreach($fixture in @('report-reference.bin','report-ui-reference.bin')){if((Get-FileHash "$OutDir/$fixture").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$fixture").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}}
$saved=$env:RS2_REPORT_ORACLE;$savedUI=$env:RS2_REPORT_UI_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_REPORT_ORACLE="$OutDir/report-reference.bin";$env:RS2_REPORT_UI_ORACLE="$OutDir/report-ui-reference.bin"
 foreach($check in @(@{name='native';exe=$TestExecutable;args=@()},@{name='loop';exe=$LoopExecutable;args=@("$OutDir/report-reference.bin","$OutDir/report-ui-reference.bin")},@{name='failure';exe=$FailureExecutable;args=@()})) {
  $prior=$ErrorActionPreference
  try {$ErrorActionPreference='Continue';$arguments=$check.args;& $check.exe @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE} finally {$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit"
  if($code -ne 0){throw "Report $($check.name) comparison failed."}
 }
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS report original:2182 footer/menu/editor/category cases,1720 category packets')){throw 'Missing original action coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch 'PASS report UI original:24 complete report panels,4104192 pixels')){throw 'Missing original UI coverage.'}
 if(-not (Select-String -LiteralPath "$OutDir/loop.log" -SimpleMatch 'cached mute/category/cancel clicks and292800 original panel foreground pixels')){throw 'Missing actual candidate integration.'}
 if(-not (Select-String -LiteralPath "$OutDir/failure.log" -SimpleMatch '15 capacity rejections,exact12-byte close/report and1-byte open successes')){throw 'Missing native fault controls.'}
 'PASS controlled revision225 report workflow:2182 original cases,2121 candidate transmissions,1720 category reports; full cipher/interfaces/editor/mute/input;24 panels/4104192 original pixels;actual footer and cached mute/category/cancel clicks,292800 source panel foreground pixels;failure-safe queue/state. No actual report to production/people or full historical screen-corpus claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$LoopExecutable,$FailureExecutable,$jar,$PSCommandPath,"$OutDir/report-reference.bin","$OutDir/report-ui-reference.bin","$root/engine/data/pack/client/title","$root/engine/data/pack/client/media","$root/client-c/src/ui/chatbox.c","$root/client-c/src/ui/context_menu.c","$root/client-c/src/ui/client_actions.c","$root/client-c/src/net/protocol.c","$root/client-c/src/client/client.c","$root/client-c/tests/test_chatbox.c","$root/client-c/verification/completion/reference/src/ReportAbuseOracle.java","$root/engine/data/pack/client/interface" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_REPORT_ORACLE=$saved;$env:RS2_REPORT_UI_ORACLE=$savedUI;Pop-Location}
exit 0
