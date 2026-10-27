param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir) { throw 'Choose a new evidence output directory' }
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$archive="$root/engine/data/pack/client/textures"
$fixture="$root/client-c/tests/fixtures/pix3d-palette-reference.bin"
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected) { throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)" }
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",
    "$root/client-c/verification/completion/reference/src/Pix3DPaletteOracle.java") "$out/java-build.log"
Invoke-Logged java @('-cp',"$out/classes;$jar",'Pix3DPaletteOracle',$archive,"$out/palette.bin") "$out/java.log"
if((Get-FileHash $fixture).Hash -ne (Get-FileHash "$out/palette.bin").Hash) { throw 'Oracle output differs from reviewed fixture' }
Invoke-Logged make @('-C',"$root/client-c",'bin/test_pix3d_palette_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_pix3d_palette_reference.exe" @($archive,$fixture) "$out/compare.log"
Get-FileHash $archive,$fixture,"$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",$jar |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/java.log","$out/compare.log"
'PASS: deterministic Java palette and real-cache Pix8 preparation match'
