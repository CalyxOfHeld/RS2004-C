param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain evidence.'}
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar";$cache="$root/engine/data/pack/client"
$sources=@('CameraPacketsOracle','InterfaceModelsOracle','ResetMultiwayOracle') | ForEach-Object {"$root/client-c/verification/completion/reference/src/$_.java"}
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" @sources *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit"
if($LASTEXITCODE -ne 0){throw 'Original batch oracle build failed.'}
foreach($oracle in @(@{name='camera';class='CameraPacketsOracle';args=@("$OutDir/camera-reference.bin")},@{name='interface';class='InterfaceModelsOracle';args=@($cache,"$OutDir/interface-reference.bin")},@{name='reset-multiway';class='ResetMultiwayOracle';args=@($cache,"$OutDir/reset-reference.bin","$OutDir/multiway-reference.bin")})) {
 $arguments=$oracle.args;& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" $oracle.class @arguments *> "$OutDir/$($oracle.name)-oracle.log"
 $LASTEXITCODE | Set-Content "$OutDir/$($oracle.name)-oracle.exit"
 if($LASTEXITCODE -ne 0){throw "Original $($oracle.name) capture failed."}
}
$fixtures=@{'camera-reference.bin'='camera-packets-reference.bin';'interface-reference.bin'='interface-models-reference.bin';'reset-reference.bin'='reset-anims-reference.bin';'multiway-reference.bin'='multiway-reference.bin'}
foreach($fixture in $fixtures.GetEnumerator()){if((Get-FileHash "$OutDir/$($fixture.Key)").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($fixture.Value)").Hash){throw "Regenerated original $($fixture.Key) differs from bundled expectations."}}
$names=@('RS2_CAMERA_ORACLE','RS2_INTERFACE_MODEL_ORACLE','RS2_RESET_ORACLE','RS2_MULTIWAY_ORACLE');$saved=@{}
foreach($name in $names){$saved[$name]=[Environment]::GetEnvironmentVariable($name)}
Push-Location "$root/client-c"
try {
 $env:RS2_CAMERA_ORACLE="$OutDir/camera-reference.bin";$env:RS2_INTERFACE_MODEL_ORACLE="$OutDir/interface-reference.bin";$env:RS2_RESET_ORACLE="$OutDir/reset-reference.bin";$env:RS2_MULTIWAY_ORACLE="$OutDir/multiway-reference.bin"
 $checks=@(@{name='camera-native';exe='test_camera';args=@()},@{name='camera-candidate';exe='camera_candidate_control';args=@("$OutDir/camera-reference.bin")},@{name='interface-native';exe='test_component';args=@()},@{name='interface-candidate';exe='interface_models_candidate_control';args=@("$OutDir/interface-reference.bin")},@{name='interface-failure';exe='interface_models_failure_control';args=@()},@{name='reset-native';exe='test_entity_sync';args=@()},@{name='reset-candidate';exe='reset_multiway_candidate_control';args=@("$OutDir/reset-reference.bin","$OutDir/multiway-reference.bin")})
 foreach($check in $checks){$prior=$ErrorActionPreference;try{$ErrorActionPreference='Continue';$arguments=$check.args;& "$root/client-c/bin/$($check.exe).exe" @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE}finally{$ErrorActionPreference=$prior};$code | Set-Content "$OutDir/$($check.name).exit";if($code -ne 0){throw "Batch $($check.name) check failed."}}
 foreach($marker in @(@{file='camera-native';text='3307 states,825 packets,1552 cutscene ticks,900 actual original draw'},@{file='camera-candidate';text='825 ISAAC-framed packet-phase observations'},@{file='interface-native';text='37 complete512x334 previews,6327296 pixels'},@{file='interface-candidate';text='37 actual viewport model preview foreground comparisons'},@{file='interface-failure';text='4 allocation failures,21 wrong lengths'},@{file='reset-native';text='5696 icon regions,7290880 pixels'},@{file='reset-candidate';text='184 actual client_draw icon regions,235520 pixels'})){if(-not (Select-String -LiteralPath "$OutDir/$($marker.file).log" -SimpleMatch $marker.text)){throw "Missing coverage marker $($marker.file)."}}
 'PASS revision225 receive batch:camera3307 states/825 packets/900 draw-RNG views/1552 ticks;interface378 states/full meshes/aliases/LRU50/37 previews/6327296 pixels;reset128 full actor arrays/multiway5696 regions/7290880 pixels. Actual candidate825+378+5824 framed packets,50 camera draws,37 preview foreground checks,184 icon regions;safe rejection/cleanup. Controlled fixtures,not arbitrary populated renderer or full gameplay qualification.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 $binaries=$checks | ForEach-Object {"$root/client-c/bin/$($_.exe).exe"}
 $identityPaths=@($binaries)+@($sources)+@($jar,$PSCommandPath,"$OutDir/camera-reference.bin","$OutDir/interface-reference.bin","$OutDir/reset-reference.bin","$OutDir/multiway-reference.bin","$cache/title","$cache/media","$cache/interface","$cache/models","$cache/config")
 Get-FileHash -LiteralPath $identityPaths | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {foreach($name in $names){[Environment]::SetEnvironmentVariable($name,$saved[$name])};Pop-Location}
exit 0
