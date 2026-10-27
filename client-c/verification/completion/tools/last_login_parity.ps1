param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain evidence.'}
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path;$jar="$root/javaclient/build/libs/rs2client.jar";$cache="$root/engine/data/pack/client"
$source="$root/client-c/verification/completion/reference/src/LastLoginOracle.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" $source *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit"
if($LASTEXITCODE -ne 0){throw 'Original last-login oracle build failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" LastLoginOracle $cache "$OutDir/reference.bin" "$OutDir/ui.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original last-login capture failed.'}
foreach($fixture in @(@{new='reference.bin';saved='last-login-reference.bin'},@{new='ui.bin';saved='last-login-ui.bin'})){if((Get-FileHash "$OutDir/$($fixture.new)").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($fixture.saved)").Hash){throw "Regenerated original $($fixture.new) differs from bundled expectations."}}
$dnsSource="$OutDir/reference-source/sign/signlink.java"
New-Item -ItemType Directory -Force "$OutDir/reference-source/sign","$OutDir/dns-classes" | Out-Null
$originalSignlink=[IO.File]::ReadAllText("$root/javaclient/src/main/java/sign/signlink.java")
$instrumented=$originalSignlink
foreach($replacement in @(@{old='String var1 = findcachedir();';new='String var1 = "";'},@{old='uid = getuid(var1);';new='uid = 0;'},@{old='dns = InetAddress.getByName(dnsreq).getHostName();';new='dns = SignlinkDnsOracle.resolve(dnsreq);'})){
 if(([regex]::Matches($instrumented,[regex]::Escape($replacement.old))).Count -ne 1){throw 'Original signlink boundary changed;review instrumentation.'}
 $instrumented=$instrumented.Replace($replacement.old,$replacement.new)
}
[IO.File]::WriteAllText($dnsSource,$instrumented,[Text.UTF8Encoding]::new($false))
& javac -encoding UTF-8 -cp $jar -d "$OutDir/dns-classes" $dnsSource "$root/client-c/verification/completion/reference/src/SignlinkDnsOracle.java" *> "$OutDir/dns-build.log"
$LASTEXITCODE | Set-Content "$OutDir/dns-build.exit";if($LASTEXITCODE -ne 0){throw 'Source DNS worker build failed.'}
& java -cp "$OutDir/dns-classes;$jar" sign.SignlinkDnsOracle "$OutDir/dns-reference.bin" *> "$OutDir/dns-oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/dns-oracle.exit";if($LASTEXITCODE -ne 0){throw 'Source DNS worker capture failed.'}
if((Get-FileHash "$OutDir/dns-reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/signlink-dns-reference.bin").Hash){throw 'Regenerated source DNS fixture differs.'}
$names=@('RS2_LAST_LOGIN_ORACLE','RS2_LAST_LOGIN_UI_ORACLE');$saved=@{};foreach($name in $names){$saved[$name]=[Environment]::GetEnvironmentVariable($name)}
Push-Location "$root/client-c"
try {
 $env:RS2_LAST_LOGIN_ORACLE="$OutDir/reference.bin";$env:RS2_LAST_LOGIN_UI_ORACLE="$OutDir/ui.bin"
 foreach($check in @(@{name='native';exe='test_component';args=@()},@{name='candidate';exe='last_login_candidate_control';args=@("$OutDir/reference.bin","$OutDir/ui.bin","$OutDir/dns-reference.bin")})){
  $prior=$ErrorActionPreference;try{$ErrorActionPreference='Continue';$arguments=$check.args;& "$root/client-c/bin/$($check.exe).exe" @arguments *> "$OutDir/$($check.name).log";$code=$LASTEXITCODE}finally{$ErrorActionPreference=$prior}
  $code | Set-Content "$OutDir/$($check.name).exit";if($code -ne 0){throw "Last-login $($check.name) check failed."}
 }
 foreach($marker in @(@{file='oracle';text='2406 read140 states,678 close231 observations'},@{file='native';text='24 complete512x334 panels/4104192 pixels and guards'},@{file='dns-oracle';text='Original signlink DNS:5 states,3 resolver calls'},@{file='native';text='26 malformed/null/metadata guards'},@{file='candidate';text='2406 ISAAC-framed packet-phase states,678 close231s,24 fragmented payloads,24 actual viewport panels/2478029 original foreground pixels'},@{file='candidate';text='bounded asynchronous DNS/5 original worker states/old-result-and-request-loss quirk/success/failure/allocation/thread-create/shutdown controls;no external DNS query'})){if(-not (Select-String -LiteralPath "$OutDir/$($marker.file).log" -SimpleMatch $marker.text)){throw "Missing coverage marker $($marker.file)."}}
 'PASS revision225 LAST_LOGIN_INFO140:2406 actual original/native/framed candidate states,678 close231s,full cipher/interfaces/report/input/DNS/text/colours;24 full native panels/4104192 pixels,24 candidate viewport panels/2478029 foreground pixels;26 malformed guards,output failure/recovery and bounded asynchronous DNS/source5-state worker controls (only startup/resolver boundaries replaced). Three recovery clientcodes use direct original-method probes because absent from cache. Original redraw/continue metadata is captured but unqualified. Isolated transport/controlled resolver,no production connection or external DNS lookup.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 $paths=@($source,$dnsSource,"$root/client-c/verification/completion/reference/src/SignlinkDnsOracle.java","$root/javaclient/src/main/java/sign/signlink.java","$OutDir/dns-reference.bin",$jar,$PSCommandPath,"$OutDir/reference.bin","$OutDir/ui.bin","$root/client-c/tests/test_component.c","$root/client-c/verification/completion/tools/last_login_reference_control.h","$root/client-c/verification/completion/tools/last_login_candidate_control.c","$root/client-c/src/client/client.c","$root/client-c/src/ui/component_packets.c","$root/client-c/include/ui/component_packets.h","$root/client-c/Makefile","$root/client-c/bin/test_component.exe","$root/client-c/bin/last_login_candidate_control.exe","$cache/interface","$cache/media","$cache/title")
 Get-FileHash -LiteralPath $paths | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {foreach($name in $names){[Environment]::SetEnvironmentVariable($name,$saved[$name])};Pop-Location}
exit 0
