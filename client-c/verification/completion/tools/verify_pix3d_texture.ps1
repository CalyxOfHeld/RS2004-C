param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir) { throw 'Choose a new evidence output directory' }
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$archive="$root/engine/data/pack/client/textures"
$fixtures="$root/client-c/tests/fixtures"
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected) { throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)" }
}
Invoke-Logged python @("$PSScriptRoot/port_pix3d_texture_triangle.py",'--check') "$out/generated.log"
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",
    "$root/client-c/verification/completion/reference/src/Pix3DTextureSpanOracle.java",
    "$root/client-c/verification/completion/reference/src/Pix3DTextureTriangleOracle.java",
    "$root/client-c/verification/completion/reference/src/Pix3DBrightnessOracle.java") "$out/java-build.log"
$oracles=@(
    @('Pix3DTextureSpanOracle','spans.bin','pix3d-texture-span-reference.bin'),
    @('Pix3DTextureTriangleOracle','triangles.bin','pix3d-texture-triangle-reference.bin'),
    @('Pix3DBrightnessOracle','brightness.bin','pix3d-brightness-reference.bin')
)
foreach($spec in $oracles) {
    Invoke-Logged java @('-cp',"$out/classes;$jar",$spec[0],$archive,"$out/$($spec[1])") "$out/$($spec[0]).log"
    if((Get-FileHash "$out/$($spec[1])").Hash -ne (Get-FileHash "$fixtures/$($spec[2])").Hash) {
        throw "Oracle fixture changed: $($spec[2])"
    }
}
Invoke-Logged make @('-C',"$root/client-c",'-s',
    'bin/test_pix3d_texture_span_reference.exe',
    'bin/test_pix3d_texture_triangle_reference.exe',
    'bin/test_pix3d_texture_cache_reference.exe',
    'bin/test_pix3d_brightness_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_pix3d_texture_span_reference.exe" @("$out/spans.bin") "$out/spans-direct.log"
Invoke-Logged "$root/client-c/bin/test_pix3d_texture_triangle_reference.exe" @("$out/triangles.bin") "$out/triangles-direct.log"
Invoke-Logged "$root/client-c/bin/test_pix3d_texture_triangle_reference.exe" @("$out/triangles.bin",$archive) "$out/triangles-cached.log"
Invoke-Logged "$root/client-c/bin/test_pix3d_texture_cache_reference.exe" @($archive,"$out/spans.bin") "$out/cache.log"
Invoke-Logged "$root/client-c/bin/test_pix3d_brightness_reference.exe" @($archive,"$out/brightness.bin") "$out/brightness.log"
Get-FileHash $archive,$jar,"$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",
    "$fixtures/pix3d-texture-span-reference.bin",
    "$fixtures/pix3d-texture-triangle-reference.bin",
    "$fixtures/pix3d-brightness-reference.bin" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/spans-direct.log","$out/triangles-direct.log","$out/triangles-cached.log","$out/cache.log","$out/brightness.log"
'PASS: Java real-cache span, triangle, texel and brightness oracles match'
