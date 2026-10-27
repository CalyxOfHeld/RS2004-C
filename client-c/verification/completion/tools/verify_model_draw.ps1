param([Parameter(Mandatory=$true)][string]$OutDir)
# Isolated drawSimple comparison; does not install a client or use a server.
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir) { throw 'Choose a new evidence directory' }
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/model-draw-reference.bin"
$model="$root/client-c/src/gfx/model.c"
$original=[IO.File]::ReadAllText($model)
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected) { throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)" }
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/client-c/verification/completion/reference/src/ModelDrawOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++) {
    Invoke-Logged java @('-cp',"$out/classes;$jar",'ModelDrawOracle',"$root/engine/data/pack/client",
        "$out/model-$pass.bin",'--extended') "$out/java-$pass.log"
    if((Get-FileHash "$out/model-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash) { throw 'Model oracle fixture changed' }
}
$sources=@('gfx/anim.c','gfx/pix2d.c','gfx/pix3d.c','gfx/pix8.c','gfx/pix3d_flat225.c',
    'gfx/pix3d_gouraud225.c','gfx/pix3d_texture225.c','gfx/pix3d_texture_triangle225.c',
    'gfx/pix3d_texture_cache225.c','core/arena.c','core/packet.c','core/isaac.c','core/jrandom.c',
    'core/jagfile.c','core/bzip2.c') | ForEach-Object { "$root/client-c/src/$_" }
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include",
    "$root/client-c/tests/test_model_draw_reference.c")+$sources
$inputs=@("$root/engine/data/pack/client/models",$fixture,"$root/engine/data/pack/client/textures")
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($model,'-lm','-o',"$out/port.exe")) "$out/port-build.log"
Invoke-Logged "$out/port.exe" $inputs "$out/port.log"
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($model,'-DMODEL_DRAW225_BASELINE','-lm','-o',"$out/baseline.exe")) "$out/baseline-build.log"
Invoke-Logged "$out/baseline.exe" $inputs "$out/baseline.log" 1
$controls=@{
    'ignore-alpha'=$original.Replace('pix3d_set_trans(m->face_alpha?m->face_alpha[f]:0);','pix3d_set_trans(0);')
    'ignore-priorities'=$original.Replace('model_order_faces(m,order,face_depth,count);','/* deliberate priority omission */')
    'ignore-bounds'=$original.Replace('if(bucket>=max_depth) continue;','if(bucket>=max_depth) { /* deliberate out-of-range draw */ }')
    'flat-texture-shade'=$original.Replace('if(type&1) cb=cc=ca;','if(type&1) cb=cc=ca^64;')
}
$utf8=New-Object System.Text.UTF8Encoding($false)
foreach($name in $controls.Keys) {
    if($controls[$name] -ceq $original) { throw "Control $name did not change the source" }
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],$utf8)
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c",'-lm','-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" $inputs "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
$bytes[16+40+12+34*4+3]=$bytes[16+40+12+34*4+3] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-pixel.bin",$bytes)
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/wrong-pixel.bin",$inputs[2]) "$out/wrong-pixel.log" 1
[IO.File]::WriteAllBytes("$out/truncated.bin",$bytes[0..($bytes.Length-2)])
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/truncated.bin",$inputs[2]) "$out/truncated.log" 2
Get-FileHash $fixture,$model,"$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar,
    "$root/engine/data/pack/client/models","$root/engine/data/pack/client/textures" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/port.log" | Select-Object -Last 1
'PASS: deterministic Java fixture; native exact; baseline and six negative controls rejected'
