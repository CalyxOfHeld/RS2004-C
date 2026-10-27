param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain evidence.'}
New-Item -ItemType Directory -Force "$OutDir/classes","$OutDir/reference-source/jagex2/client","$OutDir/reference-source/deob" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar"
$tracking=[IO.File]::ReadAllText("$root/javaclient/src/main/java/jagex2/client/InputTracking.java").Replace("`r`n","`n")
if(([regex]::Matches($tracking,[regex]::Escape('System.currentTimeMillis()'))).Count -ne 10){throw 'Original tracking clock boundaries changed;review capture.'}
$tracking=$tracking.Replace('System.currentTimeMillis()','TrackingClock.now()')
[IO.File]::WriteAllText("$OutDir/reference-source/jagex2/client/InputTracking.java",$tracking,[Text.UTF8Encoding]::new($false))
$client=[IO.File]::ReadAllText("$root/javaclient/src/main/java/deob/client.java").Replace("`r`n","`n")
$boundary="`t`tthis.updatePlayers(true);"
if(([regex]::Matches($client,[regex]::Escape($boundary))).Count -ne 1){throw 'Original updateGame observer boundary changed;review capture.'}
$client=$client.Replace($boundary,"`t`tif (jagex2.client.TrackingClock.prefixOnly) throw new jagex2.client.TrackingClock.PrefixDone();`n$boundary")
[IO.File]::WriteAllText("$OutDir/reference-source/deob/client.java",$client,[Text.UTF8Encoding]::new($false))
$sources=@("$OutDir/reference-source/jagex2/client/InputTracking.java","$OutDir/reference-source/deob/client.java","$root/client-c/verification/completion/reference/src/TrackingClock.java","$root/client-c/verification/completion/reference/src/InputTrackingOracle.java")
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" @sources *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit";if($LASTEXITCODE -ne 0){throw 'Original tracking source build failed.'}
& java '-Djava.awt.headless=false' -cp "$OutDir/classes;$jar" InputTrackingOracle "$OutDir/reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit";if($LASTEXITCODE -ne 0){throw 'Original tracking capture failed.'}
if((Get-FileHash "$OutDir/reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/input-tracking-reference.bin").Hash){throw 'Regenerated original tracking fixture differs from bundled expectations.'}
$prior=$ErrorActionPreference;try{$ErrorActionPreference='Continue';& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" InputTrackingOracle "$OutDir/forbidden.bin" *> "$OutDir/class-guard.log";$code=$LASTEXITCODE}finally{$ErrorActionPreference=$prior}
$code | Set-Content "$OutDir/class-guard.exit";if($code -ne 1 -or (Test-Path "$OutDir/forbidden.bin") -or -not (Select-String -LiteralPath "$OutDir/class-guard.log" -SimpleMatch 'Use verifier instrumented source classes')){throw 'Wrong-class capture guard failed.'}
$saved=[Environment]::GetEnvironmentVariable('RS2_TRACKING_ORACLE')
Push-Location "$root/client-c"
try {
 $env:RS2_TRACKING_ORACLE="$OutDir/reference.bin"
 foreach($check in @(@{name='native';exe='test_protocol_transport';args=@()},@{name='candidate';exe='input_tracking_candidate_control';args=@("$OutDir/reference.bin","$OutDir/event-roundtrip.log")})){
  $prior=$ErrorActionPreference;try{$ErrorActionPreference='Continue';$arguments=$check.args;& "$root/client-c/bin/$($check.exe).exe" @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE}finally{$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit";if($code -ne 0){throw "Tracking $($check.name) check failed."}
 }
 foreach($marker in @(@{file='oracle';text='Original event encoding counts=[0, 92, 69, 365, 359, 13, 47, 309, 544, 358, 14, 17, 18, 19]'},@{file='oracle';text='5339 states,336 read133/226 packets,163 actual updateGame prefixes'},@{file='native';text='760 SDL key translations vs original bytes'},@{file='native';text='23 invalid-kind/null/metadata/length guards'},@{file='candidate';text='336 ISAAC-framed read controls,499 actual client_update calls,4665 synthetic queue events'},@{file='candidate';text='480 per-event states from new key/focus/window events and481 clock reads'},@{file='candidate';text='produceexactly2 records/2 clock reads'},@{file='candidate';text='actual native normal2/mod18 disable,reconnect15 retention andserver142 logout'})){if(-not (Select-String -LiteralPath "$OutDir/$($marker.file).log" -SimpleMatch $marker.text)){throw "Missing tracking coverage marker $($marker.file)."}}
 'PASS revision225 tracking:5339 source/native/candidate states,336 original controls,163 original updateGame prefixes,499 native update calls/4665 synthetic queue events;all13 encodings/full buffers/time/count/positions/wire/ISAAC/source clock calls.760 SDL key translations,4 focus/window routes,23 guards,staged failure/recovery,480 per-event/481-clock log replay,duplicate input guard andinjected-login/native-logout lifecycle. Original copies change10 clock boundaries/1 before-actors observer only. UI consumption held;no physical user input,production connection,full original tick orwhole active-session CLI replay qualification.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 $paths=@($sources)+@($jar,$PSCommandPath,"$root/javaclient/src/main/java/jagex2/client/InputTracking.java","$root/javaclient/src/main/java/jagex2/client/GameShell.java","$root/javaclient/src/main/java/deob/client.java","$root/client-c/include/core/input_tracking.h","$root/client-c/src/core/input_tracking.c","$root/client-c/include/client/input_tracking_sdl.h","$root/client-c/include/client/client.h","$root/client-c/src/client/client.c","$root/client-c/include/net/protocol.h","$root/client-c/src/net/protocol.c","$root/client-c/platform/sdl2_main.c","$root/client-c/platform/replay_main.c","$root/client-c/verification/completion/tools/input_tracking_reference_control.h","$root/client-c/verification/completion/tools/input_tracking_candidate_control.c","$root/client-c/tests/test_protocol_transport.c","$root/client-c/Makefile","$root/client-c/bin/test_protocol_transport.exe","$root/client-c/bin/input_tracking_candidate_control.exe","$OutDir/reference.bin","$OutDir/event-roundtrip.log","$root/engine/data/pack/client/config","$root/engine/data/pack/client/interface","$root/engine/data/pack/client/models","$root/engine/data/pack/client/title","$root/engine/data/pack/client/media")
 Get-FileHash -LiteralPath $paths | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {[Environment]::SetEnvironmentVariable('RS2_TRACKING_ORACLE',$saved);Pop-Location}
exit 0
