param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain evidence.'}
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar";$cache="$root/engine/data/pack/client"
$source="$root/client-c/verification/completion/reference/src/MapDownloadOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit"
if($LASTEXITCODE -ne 0){throw 'Original map oracle build failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" MapDownloadOracle $cache "$OutDir/map-reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original map capture failed.'}
if((Get-FileHash "$OutDir/map-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/map-download-reference.bin").Hash){throw 'Regenerated original map fixture differs from bundled expectations.'}
$saved=[Environment]::GetEnvironmentVariable('RS2_MAP_ORACLE')
Push-Location "$root/client-c"
try {
 $env:RS2_MAP_ORACLE="$OutDir/map-reference.bin"
 foreach($check in @(@{name='native';exe='test_map_streaming';args=@()},@{name='candidate';exe='map_download_candidate_control';args=@("$OutDir/map-reference.bin")},@{name='failure';exe='map_download_failure_control';args=@()})){
  $prior=$ErrorActionPreference;try{$ErrorActionPreference='Continue';$arguments=$check.args;& "$root/client-c/bin/$($check.exe).exe" @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE}finally{$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit";if($code -ne 0){throw "Map $($check.name) check failed."}
 }
 foreach($marker in @(@{file='oracle';text='374 states,7 rebuild requests,24 done observations,7 completed actual buildScene'},@{file='native';text='374 original states,367 packets,7 actual scene rebuilds'},@{file='candidate';text='374 original states,367 ISAAC-framed packets,7 actual client_update rebuilds,12 fragmented headers'},@{file='candidate';text='13 actual malformed receive/recovery guards;corrupt rebuild retention and real-map recovery'},@{file='failure';text='4 allocation failures,5 atomic cache I/O failures,189 malformed lengths/ranges/null guards,8 corrupt decode rejections'})){if(-not (Select-String -LiteralPath "$OutDir/$($marker.file).log" -SimpleMatch $marker.text)){throw "Missing coverage marker $($marker.file)."}}
 'PASS revision225 map download lifecycle:374 actual original states/367 receive packets/7 complete buildScene snapshots;native+actual candidate full buffers/cache/wire/ciphers/height/flags/collision;12 fragmented headers;4 allocation/5 atomic cache I/O failures/189 malformed guards/8 corrupt decodes. Isolated cache writes only;original logical cache keys compared to native named files. Controlled50_50 transport and rebuild qualification,not arbitrary populated renderer/full gameplay.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 $paths=@($source,$jar,$PSCommandPath,"$OutDir/map-reference.bin","$root/client-c/tests/test_map_streaming.c","$root/client-c/verification/completion/tools/map_download_reference.h","$root/client-c/verification/completion/tools/map_download_candidate_control.c","$root/client-c/verification/completion/tools/map_download_failure_control.c","$root/client-c/src/dash3d/map_streamer.c","$root/client-c/include/dash3d/map_streamer.h","$root/client-c/src/client/client.c","$root/client-c/src/net/protocol.c","$root/client-c/Makefile","$root/client-c/bin/test_map_streaming.exe","$root/client-c/bin/map_download_candidate_control.exe","$root/client-c/bin/map_download_failure_control.exe","$cache/config","$cache/models","$cache/media","$cache/title","$cache/textures","$cache/maps/m50_50","$cache/maps/l50_50")
 Get-FileHash -LiteralPath $paths | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {[Environment]::SetEnvironmentVariable('RS2_MAP_ORACLE',$saved);Pop-Location}
exit 0
