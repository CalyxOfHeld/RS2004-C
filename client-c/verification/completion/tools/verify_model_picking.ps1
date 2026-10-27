param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/model-picking-reference.bin"
$model="$root/client-c/src/gfx/model.c"
$original=[IO.File]::ReadAllText($model).Replace("`r`n","`n")
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/client-c/verification/completion/reference/src/ModelPickingOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++) {
    Invoke-Logged java @('-cp',"$out/classes;$jar",'ModelPickingOracle',"$root/engine/data/pack/client","$out/picking-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/picking-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Picking fixture changed'}
}
$sources=@('gfx/anim.c','gfx/pix2d.c','gfx/pix3d.c','gfx/pix8.c','gfx/pix3d_flat225.c',
    'gfx/pix3d_gouraud225.c','gfx/pix3d_texture225.c','gfx/pix3d_texture_triangle225.c',
    'gfx/pix3d_texture_cache225.c','core/arena.c','core/packet.c','core/isaac.c','core/jrandom.c',
    'core/jagfile.c','core/bzip2.c') | ForEach-Object {"$root/client-c/src/$_"}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include",
    "$root/client-c/tests/test_model_picking_reference.c")+$sources
$inputs=@("$root/engine/data/pack/client/models",$fixture,"$root/engine/data/pack/client/textures")
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($model,'-lm','-o',"$out/port.exe")) "$out/port-build.log"
Invoke-Logged "$out/port.exe" $inputs "$out/port.log"
$controls=@{
    'coarse-inclusive-left'=$original.Replace('if(mx>left && mx<right && my>top && my<bottom)', 'if(mx>=left && mx<right && my>top && my<bottom)')
    'ignore-hover-gate'=$original.Replace('if(pick && pick->bitset>0 && pick->check_hover)', 'if(pick && pick->bitset>0)')
    'allow-zero-bitset'=$original.Replace('if(pick && pick->bitset>0 && pick->check_hover)', 'if(pick && pick->bitset>=0 && pick->check_hover)')
    'pick-clipped-faces'=$original.Replace('if(!clipped[f] && check_faces && model_point_within_triangle225','if(check_faces && model_point_within_triangle225')
    'strict-face-right'=$original.Replace('return mx<=xa || mx<=xb || mx<=xc;','return mx<xa || mx<xb || mx<xc;')
}
foreach($name in $controls.Keys) {
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],(New-Object System.Text.UTF8Encoding($false)))
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c",'-lm','-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" $inputs "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
$bytes[16+56+20+3]=$bytes[16+56+20+3] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-pick.bin",$bytes)
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/wrong-pick.bin",$inputs[2]) "$out/wrong-pick.log" 1
[IO.File]::WriteAllBytes("$out/truncated.bin",$bytes[0..($bytes.Length-2)])
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/truncated.bin",$inputs[2]) "$out/truncated.log" 2
Get-FileHash $fixture,$model,"$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/port.log"
'PASS: deterministic Java picking fixture; native exact; seven negative controls rejected'
