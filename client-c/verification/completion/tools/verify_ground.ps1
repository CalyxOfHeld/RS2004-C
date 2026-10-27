param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/ground-reference.bin"
$ground="$root/client-c/src/dash3d/ground225.c"
$original=[IO.File]::ReadAllText($ground).Replace("`r`n","`n")
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0){
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/client-c/verification/completion/reference/src/GroundDrawOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++){
    Invoke-Logged java @('-cp',"$out/classes;$jar",'GroundDrawOracle',"$root/engine/data/pack/client",
        "$root/client-c/tests/fixtures/scene-lighting","$out/ground-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/ground-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Ground oracle fixture changed'}
}
$sources=@('gfx/pix2d.c','gfx/pix3d.c','gfx/pix8.c','gfx/pix3d_flat225.c','gfx/pix3d_gouraud225.c',
    'gfx/pix3d_texture225.c','gfx/pix3d_texture_triangle225.c','gfx/pix3d_texture_cache225.c',
    'core/arena.c','core/packet.c','core/isaac.c','core/jagfile.c','core/bzip2.c','core/jrandom.c') |
    ForEach-Object {"$root/client-c/src/$_"}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include",
    "$root/client-c/tests/test_ground_reference.c")+$sources
$inputs=@("$root/engine/data/pack/client/textures",$fixture)
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($ground,'-lm','-o',"$out/port.exe")) "$out/port-build.log"
Invoke-Logged "$out/port.exe" $inputs "$out/port.log"
$controls=@{
    'near-plane-51'=$original.Replace('if(v[i].z<50)','if(v[i].z<51)')
    'early-alpha-reset'=$original.Replace('static GroundVertex225 transform(int32_t x,int32_t y,int32_t z,const Ground225 *view){',
        'static GroundVertex225 transform(int32_t x,int32_t y,int32_t z,const Ground225 *view){'+[char]10+'    pix3d_set_trans(0);')
    'force-full-textures'=$original.Replace('if(view->low_memory){','if(view->low_memory && false){')
    'underlay-flat-axis'=$original.Replace('tile->flat?0:2,tile->flat?1:3,tile->flat?3:1','2,3,1')
    'overlay-flat-axis'=$original.Replace('tile->flat?0:a,tile->flat?1:b,tile->flat?3:c','a,b,c')
    'ignore-overlay-draw-gate'=$original.Replace('pix3d_set_trans(0);if(!draw)return true;','pix3d_set_trans(0);(void)draw;')
    'wide-pick-products'=$original.Replace('return mul(a,c)>0 && mul(c,b)>0;','return (int64_t)a*c>0 && (int64_t)c*b>0;')
    'inclusive-pick-edges'=$original.Replace('return mul(a,c)>0 && mul(c,b)>0;','return mul(a,c)>=0 && mul(c,b)>=0;')
    'skip-hidden-picking'=$original.Replace('if(view->taking_input && world3d_point_inside_triangle225','if(ca!=12345678 && view->taking_input && world3d_point_inside_triangle225')
}
foreach($name in $controls.Keys){
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],(New-Object System.Text.UTF8Encoding($false)))
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c",'-lm','-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" $inputs "$out/$name.log" 1
}
Invoke-Logged python @("$PSScriptRoot/ground_fixture_controls.py",$fixture,$out) "$out/fixture-controls.log"
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/wrong-pixel.bin") "$out/wrong-pixel.log" 1
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/truncated.bin") "$out/truncated.log" 2
Get-FileHash $fixture,$ground,"$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",$jar,"$root/engine/data/pack/client/textures" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/port.log"
'PASS: original ground fixture reproduces; native exact; eleven negative controls rejected'
