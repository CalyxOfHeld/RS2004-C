param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain evidence.'}
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$source="$root/client-c/verification/completion/reference/src/ContinueOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit"
if($LASTEXITCODE -ne 0){throw 'Original continue oracle build failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" ContinueOracle "$root/engine/data/pack/client" "$OutDir/reference.bin" "$OutDir/ui.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original continue capture failed.'}
foreach($fixture in @(@{new='reference.bin';saved='continue-reference.bin'},@{new='ui.bin';saved='continue-ui.bin'})){
 if((Get-FileHash "$OutDir/$($fixture.new)").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($fixture.saved)").Hash){throw "Regenerated original $($fixture.new) differs from pinned expectations."}
}
$names=@('RS2_CONTINUE_ORACLE','RS2_CONTINUE_UI_ORACLE');$saved=@{}
foreach($name in $names){$saved[$name]=[Environment]::GetEnvironmentVariable($name)}
Push-Location "$root/client-c"
try {
 $env:RS2_CONTINUE_ORACLE="$OutDir/reference.bin";$env:RS2_CONTINUE_UI_ORACLE="$OutDir/ui.bin"
 foreach($check in @(@{name='native';exe='test_component';args=@()},@{name='candidate';exe='continue_candidate_control';args=@("$OutDir/reference.bin","$OutDir/ui.bin")})){
  $arguments=$check.args
  & "$root/client-c/bin/$($check.exe).exe" @arguments *> "$OutDir/$($check.name).log"
  $LASTEXITCODE | Set-Content "$OutDir/$($check.name).exit"
  if($LASTEXITCODE -ne 0){throw "Continue $($check.name) check failed."}
 }
 foreach($marker in @(@{file='oracle';text='Original continue:1134'},@{file='native';text='48 complete512x334 panels/8208384 pixels'},@{file='candidate';text='432 encrypted receive packets,18 fragmented payloads,48 actual panels/10112 source foreground pixels'},@{file='candidate';text='three queued primary clicks emit one235'},@{file='candidate';text='injected normal2/mod18 clear,reconnect15 retains'})){
  if(-not (Select-String -LiteralPath "$OutDir/$($marker.file).log" -SimpleMatch $marker.text)){throw "Missing $($marker.file) coverage marker."}
 }
 'PASS revision225 continue:1134 actual original/native/candidate action/read/close states;full waiting/interface/menu/wire/ISAAC;48 full source/native panels and48 actual candidate panels;432 encrypted receive packets,18 fragmented payloads,queued UI click and safety/login controls. Uses unchanged original jar,synthetic components with real cache fonts;not full gameplay/authentication or all interface animation/redraw parity.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 $paths=@($source,$jar,$PSCommandPath,"$OutDir/reference.bin","$OutDir/ui.bin","$root/client-c/src/client/client.c","$root/client-c/src/ui/component.c","$root/client-c/src/ui/varp.c","$root/client-c/src/ui/client_actions.c","$root/client-c/src/ui/component_packets.c","$root/client-c/src/net/protocol.c","$root/client-c/include/ui/component.h","$root/client-c/verification/completion/tools/continue_reference_control.h","$root/client-c/verification/completion/tools/continue_candidate_control.c","$root/client-c/bin/test_component.exe","$root/client-c/bin/continue_candidate_control.exe","$root/engine/data/pack/client/title","$root/engine/data/pack/client/interface")
 Get-FileHash -LiteralPath $paths | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {foreach($name in $names){[Environment]::SetEnvironmentVariable($name,$saved[$name])};Pop-Location}
exit 0
