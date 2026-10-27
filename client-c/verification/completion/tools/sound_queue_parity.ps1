param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh evidence directory.'}
New-Item -ItemType Directory -Force "$OutDir/classes","$OutDir/backend-classes","$OutDir/source/deob","$OutDir/source/sign" | Out-Null
$OutDir=(Resolve-Path $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar";$ref="$root/client-c/verification/completion/reference/src"
function Replace-Once([string]$text,[string]$from,[string]$to){if(([regex]::Matches($text,[regex]::Escape($from))).Count -ne 1){throw "Source boundary changed: $from"};return $text.Replace($from,$to)}
function Run-Check([string]$name,[scriptblock]$action,[int]$expected=0){& $action *> "$OutDir/$name.log";$code=$LASTEXITCODE;$code | Set-Content "$OutDir/$name.exit";if($code -ne $expected){throw "$name exited $code; expected $expected"}}
$source=[IO.File]::ReadAllText("$root/javaclient/src/main/java/deob/client.java").Replace("`r`n","`n")
$start=$source.IndexOf("`tpublic final void updateGame(boolean arg0) {");if($start -lt 0){throw 'Missing original updateGame'}
$end=$source.IndexOf("`n`t@ObfuscatedName",$start);if($end -lt 0){throw 'Missing method end'}
$loopStart=$source.IndexOf("`t`tint var12;",$start);$loopEnd=$source.IndexOf("`t`tif (this.nextMusicDelay > 0)",$loopStart)
if($loopStart -lt $start -or $loopEnd -gt $end){throw 'Missing original queue slice'}
$loop=$source.Substring($loopStart,$loopEnd-$loopStart)
$loop=Replace-Once $loop 'Wave.generate((byte) -16, this.waveLoops[var3], this.waveIds[var3])' 'SoundQueueBoundary.generate(this.waveIds[var3],this.waveLoops[var3])'
if(([regex]::Matches($loop,'System.currentTimeMillis\(\)')).Count -ne 2){throw 'Expected two original clock reads'}
$loop=$loop.Replace('System.currentTimeMillis()','SoundQueueBoundary.now()')
$loop=Replace-Once $loop 'this.saveWave(var5.data, var5.pos, 0)' 'SoundQueueBoundary.save(this,var5.data,var5.pos)'
$loop=Replace-Once $loop 'this.replayWave(this.field1143)' 'SoundQueueBoundary.replay(this)'
$isolated=$source.Substring(0,$start)+"`tpublic final void updateGame(boolean arg0) {`n"+$loop+"`t}`n"+$source.Substring($end)
[IO.File]::WriteAllText("$OutDir/source/deob/client.java",$isolated)
$worker=[IO.File]::ReadAllText("$root/javaclient/src/main/java/sign/signlink.java")
$worker=Replace-Once $worker 'String var1 = findcachedir();' 'String var1 = "";'
$worker=Replace-Once $worker 'uid = getuid(var1);' 'uid = 0;'
$worker=Replace-Once $worker 'new FileOutputStream(var1 + savereq)' 'SoundBackendBoundary.open(var1 + savereq)'
$worker=Replace-Once $worker 'Thread.sleep((long) looprate);' 'SoundBackendBoundary.step();'
[IO.File]::WriteAllText("$OutDir/source/sign/signlink.java",$worker)
Run-Check 'oracle-build' {javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$OutDir/source/deob/client.java" "$ref/SoundQueueBoundary.java" "$ref/SoundQueueOracle.java"}
Run-Check 'backend-build' {javac -encoding UTF-8 -cp $jar -d "$OutDir/backend-classes" "$OutDir/source/sign/signlink.java" "$ref/SoundBackendBoundary.java" "$ref/SoundBackendOracle.java"}
Run-Check 'oracle' {java '-Djava.awt.headless=false' -cp "$OutDir/classes;$jar" SoundQueueOracle "$root/engine/data/pack/client" "$OutDir/reference.bin"}
Run-Check 'backend' {java '-Djava.awt.headless=false' -cp "$OutDir/backend-classes;$jar" SoundBackendOracle "$OutDir/backend.bin"}
Run-Check 'wrong-queue-class' {java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" SoundQueueOracle "$root/engine/data/pack/client" "$OutDir/forbidden-queue.bin"} 1
Run-Check 'wrong-worker-class' {java '-Djava.awt.headless=false' -cp "$jar;$OutDir/backend-classes" SoundBackendOracle "$OutDir/forbidden-worker.bin"} 1
if((Test-Path "$OutDir/forbidden-queue.bin") -or (Test-Path "$OutDir/forbidden-worker.bin")){throw 'Wrong-class guard wrote a fixture'}
foreach($pair in @(@('reference.bin','sound-queue-reference.bin'),@('backend.bin','sound-backend-reference.bin'))){if((Get-FileHash "$OutDir/$($pair[0])").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($pair[1])").Hash){throw 'Regenerated source fixture differs'}}
$old=[Environment]::GetEnvironmentVariable('RS2_SOUND_QUEUE_ORACLE')
Push-Location "$root/client-c"
try {
 $env:RS2_SOUND_QUEUE_ORACLE="$OutDir/reference.bin"
 Run-Check 'native' {& "$root/client-c/bin/test_audio.exe"}
 Run-Check 'candidate' {& "$root/client-c/bin/sound_queue_candidate_control.exe" "$OutDir/reference.bin" "$OutDir/backend.bin" "$OutDir/audio.log" "$OutDir/audio.states"}
 foreach($marker in @('2326 full50-slot states','1740 actual cache generations','67 original actual-worker states','3 actual candidate ticks,2 recorded wall-clock reads,3 request outcomes','framed142 logout retention')){if(-not (Select-String -LiteralPath "$OutDir/candidate.log" -SimpleMatch $marker)){throw "Missing coverage: $marker"}}
 'PASS source fixtures SHA-exact;2326 queue states/ordered traces,1740 actual-cache generations,67 worker states,actual worker join,logged clock/outcome replay,framed sound/settings/login/logout lifecycle. Captured output;no physical audibility or shared MIDI-worker scheduling claim.' | Set-Content -Encoding utf8 "$OutDir/result.txt"
}finally{[Environment]::SetEnvironmentVariable('RS2_SOUND_QUEUE_ORACLE',$old);Pop-Location}
exit 0
