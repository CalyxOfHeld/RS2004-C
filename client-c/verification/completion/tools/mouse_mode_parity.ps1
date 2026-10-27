param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain evidence.'}
New-Item -ItemType Directory -Force "$OutDir/classes","$OutDir/reference-source/deob" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$original=[IO.File]::ReadAllText("$root/javaclient/src/main/java/deob/client.java").Replace("`r`n","`n")
$start=$original.IndexOf("`tpublic final void updateGame(boolean arg0) {")
$end=$original.IndexOf("`n`t@ObfuscatedName",$start)
$a=$original.IndexOf("`t`tint var6;`n`t`tif (this.objDragArea != 0)",$start)
$b=$original.IndexOf("`t`tcyclelogic3++;",$a)
if($start -lt 0 -or $end -lt $start -or $a -lt $start -or $b -lt $a -or $b -gt $end){throw 'Original release boundaries changed.'}
$slice=$original.Substring($a,$b-$a)
if(([regex]::Matches($slice,[regex]::Escape('this.showContextMenu(-386);'))).Count -ne 1 -or ([regex]::Matches($slice,[regex]::Escape('this.useMenuOption(6412, this.menuSize - 1);'))).Count -ne 1){throw 'Original release slice changed.'}
$source=$original.Substring(0,$start)+"`tpublic final void updateGame(boolean arg0) {`n`t`tMouseModeBoundary.calls++;`n"+$slice+"`t}`n"+$original.Substring($end)
[IO.File]::WriteAllText("$OutDir/reference-source/deob/client.java",$source,[Text.UTF8Encoding]::new($false))
$jar="$root/javaclient/build/libs/rs2client.jar"
$oracle="$root/client-c/verification/completion/reference/src/MouseModeOracle.java"
$boundary="$root/client-c/verification/completion/reference/src/MouseModeBoundary.java"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$OutDir/reference-source/deob/client.java" $boundary $oracle *> "$OutDir/oracle-build.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle-build.exit";if($LASTEXITCODE -ne 0){throw 'Original source compile failed.'}
& java '-Djava.awt.headless=false' -cp "$OutDir/classes;$jar" MouseModeOracle "$root/engine/data/pack/client" "$OutDir/reference.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit";if($LASTEXITCODE -ne 0){throw 'Original mouse-mode capture failed.'}
if((Get-FileHash "$OutDir/reference.bin").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/mouse-mode-reference.bin").Hash){throw 'Regenerated source fixture differs from pinned expectations.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" MouseModeOracle "$root/engine/data/pack/client" "$OutDir/forbidden.bin" *> "$OutDir/class-guard.log"
$code=$LASTEXITCODE;$code | Set-Content "$OutDir/class-guard.exit"
if($code -ne 1 -or (Test-Path "$OutDir/forbidden.bin") -or -not (Select-String -LiteralPath "$OutDir/class-guard.log" -SimpleMatch 'Wrong client class:release slice not loaded')){throw 'Wrong-class guard failed.'}
$saved=[Environment]::GetEnvironmentVariable('RS2_MOUSE_MODE_ORACLE')
Push-Location "$root/client-c"
try {
 $env:RS2_MOUSE_MODE_ORACLE="$OutDir/reference.bin"
 foreach($check in @(@{name='native';exe='test_context_menu';args=@()},@{name='candidate';exe='mouse_mode_candidate_control';args=@("$OutDir/reference.bin")})){
  $arguments=$check.args;& "$root/client-c/bin/$($check.exe).exe" @arguments *> "$OutDir/$($check.name).log"
  $LASTEXITCODE | Set-Content "$OutDir/$($check.name).exit";if($LASTEXITCODE -ne 0){throw "Mouse-mode $($check.name) check failed."}
 }
 foreach($marker in @(@{file='oracle';text='1744 setting states,5120 input states including320'},@{file='native';text='PASS original mouse mode:1744 setting states'},@{file='candidate';text='864 encrypted varp packets/24 fragments'},@{file='candidate';text='16 actual cached setting clicks'})){if(-not (Select-String -LiteralPath "$OutDir/$($marker.file).log" -SimpleMatch $marker.text)){throw "Missing $($marker.file) coverage marker."}}
 'PASS revision225 mouse mode:1744 original setting states,5120 original/native/candidate input states including320 original release slices;864 encrypted varp packets/24 fragments,16 actual cached widget clicks,all source menu rectangles/local-server-option/waiting/drag/wire/ISAAC. Original updateGame is explicitly restricted to its contiguous drag-release slice;other original methods unchanged. No full original tick,successful drag swap,pixel,physical-input,account orcombined gameplay claim.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash -LiteralPath @($jar,$oracle,$boundary,$PSCommandPath,"$OutDir/reference-source/deob/client.java","$OutDir/reference.bin","$root/client-c/src/client/client.c","$root/client-c/src/ui/varp.c","$root/client-c/src/ui/context_menu.c","$root/client-c/src/ui/component_packets.c","$root/client-c/src/ui/client_actions.c","$root/client-c/src/net/protocol.c","$root/client-c/bin/test_context_menu.exe","$root/client-c/bin/mouse_mode_candidate_control.exe","$root/engine/data/pack/client/config","$root/engine/data/pack/client/interface","$root/engine/data/pack/client/title") | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {[Environment]::SetEnvironmentVariable('RS2_MOUSE_MODE_ORACLE',$saved);Pop-Location}
exit 0
