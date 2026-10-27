param([Parameter(Mandatory=$true)][string]$OutDir,[string]$Python='C:/Python314/python.exe')
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh evidence directory.'}
New-Item -ItemType Directory -Force "$OutDir/classes","$OutDir/worker-classes","$OutDir/source/deob","$OutDir/worker-source/sign" | Out-Null
$OutDir=(Resolve-Path $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar";$ref="$root/client-c/verification/completion/reference/src"
function Run-Check([string]$name,[scriptblock]$action,[int]$expected=0){& $action *> "$OutDir/$name.log";$code=$LASTEXITCODE;$code | Set-Content "$OutDir/$name.exit";if($code -ne $expected){throw "$name exited $code;expected $expected"}}
function Replace-Once([string]$text,[string]$from,[string]$to){if(([regex]::Matches($text,[regex]::Escape($from))).Count -ne 1){throw "Source boundary changed:$from"};return $text.Replace($from,$to)}
$source=[IO.File]::ReadAllText("$root/javaclient/src/main/java/deob/client.java").Replace("`r`n","`n")
$start=$source.IndexOf("`tpublic final void updateGame(boolean arg0) {");$end=$source.IndexOf("`n`t@ObfuscatedName",$start)
$a=$source.IndexOf("`t`tif (this.nextMusicDelay > 0)",$start);$b=$source.IndexOf("`t`tPacket var11 = InputTracking.flush",$a)
if($start -lt 0 -or $end -lt $start -or $a -lt $start -or $b -gt $end){throw 'Original music tick slice changed'}
$isolated=$source.Substring(0,$start)+"`tpublic int musicSlice225;`n`tpublic final void updateGame(boolean arg0) {`n"+$source.Substring($a,$b-$a)+"`t}`n"+$source.Substring($end)
[IO.File]::WriteAllText("$OutDir/source/deob/client.java",$isolated)
$worker=[IO.File]::ReadAllText("$root/javaclient/src/main/java/sign/signlink.java")
$worker=Replace-Once $worker 'String var1 = findcachedir();' 'String var1 = "";'
$worker=Replace-Once $worker 'uid = getuid(var1);' 'uid = 0;'
$worker=Replace-Once $worker 'new FileOutputStream(var1 + savereq)' 'MixedAudioBoundary.open(var1 + savereq)'
$worker=Replace-Once $worker 'Thread.sleep((long) looprate);' 'MixedAudioBoundary.step();'
[IO.File]::WriteAllText("$OutDir/worker-source/sign/signlink.java",$worker)
Run-Check 'oracle-build' {javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$OutDir/source/deob/client.java" "$ref/MusicStateOracle.java" "$ref/MusicSettingOracle.java" "$ref/MidiOracle.java"}
Run-Check 'worker-build' {javac -encoding UTF-8 -cp $jar -d "$OutDir/worker-classes" "$OutDir/worker-source/sign/signlink.java" "$ref/MixedAudioBoundary.java" "$ref/MixedAudioOracle.java"}
Run-Check 'state' {java '-Djava.awt.headless=false' -cp "$OutDir/classes;$jar" MusicStateOracle "$root/engine/data/pack/client" "$OutDir/state.bin"}
Run-Check 'settings' {java '-Djava.awt.headless=false' -cp "$OutDir/classes;$jar" MusicSettingOracle "$root/engine/data/pack/client" "$OutDir/settings.bin"}
Run-Check 'mixed' {java '-Djava.awt.headless=false' -cp "$OutDir/worker-classes;$jar" MixedAudioOracle "$OutDir/mixed.bin"}
Run-Check 'decode' {java -cp "$jar;$OutDir/classes" MidiOracle "$root/engine/data/pack/client" "$OutDir/decode.bin"}
Run-Check 'wrong-state-class' {java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" MusicStateOracle "$root/engine/data/pack/client" "$OutDir/forbidden-state.bin"} 1
Run-Check 'wrong-worker-class' {java '-Djava.awt.headless=false' -cp "$jar;$OutDir/worker-classes" MixedAudioOracle "$OutDir/forbidden-worker.bin"} 1
if((Test-Path "$OutDir/forbidden-state.bin") -or (Test-Path "$OutDir/forbidden-worker.bin")){throw 'Wrong-class guard wrote a fixture'}
foreach($pair in @(@('state.bin','music-state-reference.bin'),@('settings.bin','music-setting-reference.bin'),@('mixed.bin','mixed-audio-reference.bin'),@('decode.bin','midi-reference.bin'))){if((Get-FileHash "$OutDir/$($pair[0])").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($pair[1])").Hash){throw "Source fixture differs:$($pair[0])"}}
$oldState=[Environment]::GetEnvironmentVariable('RS2_MUSIC_STATE_ORACLE');$oldSetting=[Environment]::GetEnvironmentVariable('RS2_MUSIC_SETTING_ORACLE')
Push-Location "$root/client-c"
try {
 $env:RS2_MUSIC_STATE_ORACLE="$OutDir/state.bin";$env:RS2_MUSIC_SETTING_ORACLE="$OutDir/settings.bin"
 Run-Check 'native' {& "$root/client-c/bin/test_audio.exe"}
 Run-Check 'candidate' {& "$root/client-c/bin/music_candidate_control.exe" "$OutDir/state.bin" "$OutDir/mixed.bin" "$OutDir/settings.bin" "$OutDir/music.log"}
 Run-Check 'gain-build' {& x86_64-w64-mingw32-gcc -std=c99 -Wall -Wextra -Werror -O2 -Iinclude -Ilib/SDL2/x86_64-w64-mingw32/include -shared '-Wl,--export-all-symbols' verification/completion/tools/midi_gain_library.c -o "$OutDir/midi_gain.dll"}
 Run-Check 'gain-policy' {& $Python verification/completion/tools/midi_gain_policy.py --library "$OutDir/midi_gain.dll" --fixture "$OutDir/decode.bin" --outdir "$OutDir/gain-policy"}
 foreach($marker in @('840 framed song/jingle packets','960 original direct/150/175/193','68 original actual-worker shared','PASS actual mixed MIDI worker','PASS integrated audio','PASS mixed audio log')){if(-not (Select-String -LiteralPath "$OutDir/candidate.log" -SimpleMatch $marker)){throw "Missing coverage:$marker"}}
 'PASS four source fixtures SHA-exact;1800 original operations/next ticks,960 settings/wire/cipher,840 framed candidate packets,68 shared worker states,unheld integrated flow,actual worker join,MQ/AQ/MP replay,230 cached MIDI/RMID files at5 native gain-policy levels. Captured output;physical audibility andcontinuous PCM gain/fade equivalence are not claimed.' | Set-Content -Encoding utf8 "$OutDir/result.txt"
}finally{[Environment]::SetEnvironmentVariable('RS2_MUSIC_STATE_ORACLE',$oldState);[Environment]::SetEnvironmentVariable('RS2_MUSIC_SETTING_ORACLE',$oldSetting);Pop-Location}
exit 0
