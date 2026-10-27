param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$CandidateExecutable='')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
if(-not $CandidateExecutable){$CandidateExecutable="$root/client-c/bin/overhead_chat_candidate_control.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
$CandidateExecutable=(Resolve-Path -LiteralPath $CandidateExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$source="$root/client-c/verification/completion/reference/src/OverheadChatOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original overhead diagnostic compilation failed.'}
& java -cp "$jar;$OutDir/classes" OverheadChatOracle "$root/engine/data/pack/client/title" "$OutDir/overhead-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original overhead capture failed.'}
if((Get-FileHash "$OutDir/overhead-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/overhead-chat-reference.bin").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
$saved=$env:RS2_OVERHEAD_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_OVERHEAD_ORACLE="$OutDir/overhead-reference.bin"
 foreach($check in @(@{name='native';exe=$TestExecutable},@{name='candidate';exe=$CandidateExecutable})) {
  $log="$OutDir/$($check.name).log"
  $prior=$ErrorActionPreference
  try {$ErrorActionPreference='Continue'; & $check.exe *> $log; $code=$LASTEXITCODE} finally {$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit"
  if($code -ne 0){throw "Overhead $($check.name) comparison failed."}
  if(-not (Select-String -LiteralPath $log -SimpleMatch 'PASS overhead original:1426 frames,243857408 viewport pixels,all50 persistent slots; 6 safe styled50th rejections')){throw 'Missing original comparison coverage marker.'}
  if(-not (Select-String -LiteralPath $log -SimpleMatch 'PASS overhead native safety:10 malformed')){throw 'Missing native safety marker.'}
 }
 if(-not (Select-String -LiteralPath "$OutDir/candidate.log" -SimpleMatch 'PASS candidate overhead:actual projection/full original pixels at8,11; setting6 real draw 252 yellow glyph pixels')){throw 'Missing candidate integration marker.'}
 'PASS controlled revision225 overhead:1426 original frames,complete50-slot persistent state,243857408 pixels; actual candidate projection at8,11;6 safe original overflow rejections;10 native malformed controls;actual setting6/draw-cycle integration. No production conversation or full historical screen-corpus claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$CandidateExecutable,$jar,$source,$PSCommandPath,"$root/engine/data/pack/client/title","$OutDir/overhead-reference.bin","$root/client-c/src/ui/overhead_chat.c","$root/client-c/include/ui/overhead_chat.h","$root/client-c/src/gfx/pixfont.c","$root/client-c/src/ui/chatbox.c","$root/client-c/src/client/client.c","$root/client-c/tests/test_chatbox.c","$root/client-c/verification/completion/tools/overhead_chat_candidate_control.c" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_OVERHEAD_ORACLE=$saved;Pop-Location}
exit 0
