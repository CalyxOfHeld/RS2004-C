param([Parameter(Mandatory=$true)][string]$OutDir,[ValidateSet(0,1)][int]$ExpectedNativeExit=1)
# Current native scene parity FAILS. This reproduces and measures that gap;
# a successful measurement is never reported as a scene-parity pass.
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes","$OutDir/java-captures","$OutDir/native-captures" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/static-scene-reference.bin"
$cache="$root/engine/data/pack/client"
$maps="$root/client-c/tests/fixtures/scene-lighting"
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/client-c/verification/completion/reference/src/StaticSceneOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++) {
    Invoke-Logged java @('-Djava.awt.headless=true','-cp',"$out/classes;$jar",'StaticSceneOracle',
        $cache,$maps,"$out/scene-$pass.bin","$out/java-captures") "$out/java-$pass.log"
    if((Get-FileHash "$out/scene-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Static scene oracle changed'}
}
Invoke-Logged make @('-C',"$root/client-c",'-s','bin/test_scene_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_scene_reference.exe" @($cache,$maps,$fixture,"$out/native-captures") "$out/native.log" $ExpectedNativeExit
Get-FileHash $fixture,$jar,"$maps/m50_50.raw","$maps/l50_50.raw",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$cache/config","$cache/models","$cache/textures" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/native.log" | Select-Object -Last 1
if($ExpectedNativeExit -eq 1){'MEASURED: Java fixture reproduces; native scene parity DIFFERS (exit 1)'}
else {'Native scene comparison returned equality (exit 0); inspect scope in the logs'}
