param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh evidence directory.'}
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/WaveSettingOracle.java" *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit";if($LASTEXITCODE -ne 0){throw 'Original oracle build failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" WaveSettingOracle "$root/engine/data/pack/client" "$OutDir/reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit";if($LASTEXITCODE -ne 0){throw 'Original oracle failed.'}
& node "$root/client-c/verification/completion/reference/src/wave_gain_oracle.mjs" "$root/webclient/src/3rdparty/audio.js" "$OutDir/gain.bin" *> "$OutDir/web-gain.log"
$LASTEXITCODE | Set-Content "$OutDir/web-gain.exit";if($LASTEXITCODE -ne 0){throw 'Original web gain failed.'}
foreach($pair in @(@('reference.bin','wave-setting-reference.bin'),@('gain.bin','wave-gain-reference.bin'))){if((Get-FileHash "$OutDir/$($pair[0])").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($pair[1])").Hash){throw 'Regenerated source fixture differs.'}}
$old=[Environment]::GetEnvironmentVariable('RS2_WAVE_SETTING_ORACLE');$oldGain=[Environment]::GetEnvironmentVariable('RS2_WAVE_GAIN_ORACLE')
Push-Location "$root/client-c"
try {$env:RS2_WAVE_SETTING_ORACLE="$OutDir/reference.bin";$env:RS2_WAVE_GAIN_ORACLE="$OutDir/gain.bin"
 & "$root/client-c/bin/test_audio.exe" *> "$OutDir/native.log";$LASTEXITCODE | Set-Content "$OutDir/native.exit";if($LASTEXITCODE -ne 0){throw 'Native setting check failed.'}
 & "$root/client-c/bin/wave_setting_candidate_control.exe" "$OutDir/reference.bin" "$OutDir/gain.bin" *> "$OutDir/candidate.log";$LASTEXITCODE | Set-Content "$OutDir/candidate.exit";if($LASTEXITCODE -ne 0){throw 'Candidate setting check failed.'}
 foreach($marker in @('552 encrypted packets','22048 cache sample bytes at4 source gains')){if(-not (Select-String -LiteralPath "$OutDir/candidate.log" -SimpleMatch $marker)){throw 'Missing candidate coverage marker.'}}
 'PASS wave setting:960 original/native/candidate states,72 read12 admissions,552 framed candidate packets,4 original web gains,1024 U8 policy probes,22048 actual backend output bytes;audio tick held at packet observer andSDL output/device calls captured. No original full scheduler,live-buffer attenuation,audibility orphysical-device claim.' | Set-Content -Encoding utf8 "$OutDir/result.txt"
}finally{[Environment]::SetEnvironmentVariable('RS2_WAVE_SETTING_ORACLE',$old);[Environment]::SetEnvironmentVariable('RS2_WAVE_GAIN_ORACLE',$oldGain);Pop-Location}
exit 0
