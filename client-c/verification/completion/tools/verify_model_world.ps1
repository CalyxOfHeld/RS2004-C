param([Parameter(Mandatory=$true)][string]$OutDir)
# Original world draw vs separate C path. No installed build or server use.
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/model-world-reference.bin"
$model="$root/client-c/src/gfx/model.c"
$original=[IO.File]::ReadAllText($model).Replace("`r`n","`n")
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/client-c/verification/completion/reference/src/ModelWorldOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++) {
    Invoke-Logged java @('-cp',"$out/classes;$jar",'ModelWorldOracle',"$root/engine/data/pack/client","$out/world-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/world-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'World fixture changed'}
}
$sources=@('gfx/anim.c','gfx/pix2d.c','gfx/pix3d.c','gfx/pix8.c','gfx/pix3d_flat225.c',
    'gfx/pix3d_gouraud225.c','gfx/pix3d_texture225.c','gfx/pix3d_texture_triangle225.c',
    'gfx/pix3d_texture_cache225.c','core/arena.c','core/packet.c','core/isaac.c','core/jrandom.c',
    'core/jagfile.c','core/bzip2.c') | ForEach-Object {"$root/client-c/src/$_"}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include",
    "$root/client-c/tests/test_model_world_reference.c")+$sources
$inputs=@("$root/engine/data/pack/client/models",$fixture,"$root/engine/data/pack/client/textures")
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($model,'-lm','-o',"$out/port.exe")) "$out/port-build.log"
Invoke-Logged "$out/port.exe" $inputs "$out/port.log"
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($model,'-DMODEL_WORLD225_BASELINE','-lm','-o',"$out/baseline.exe")) "$out/baseline-build.log"
Invoke-Logged "$out/baseline.exe" $inputs "$out/baseline.log" 1
$controls=@{
    'replace-inherited-alpha'=$original.Replace('int count=0;'+[char]10+'    for(int k=0;k<3;k++) {',
        'int count=0;'+[char]10+'    pix3d_set_trans(m->face_alpha?m->face_alpha[f]:0);'+[char]10+'    for(int k=0;k<3;k++) {')
    'omit-quad-triangle'=$original.Replace('m225_emit(m,f,x,y,c,texels,opaque);','(void)x;(void)y;(void)c;')
    'reciprocal-plus-one'=$original.Replace('pix3d_div_table2[distance]);','m225_add(pix3d_div_table2[distance],1));')
    'near-plane-51'=$original.Replace('if(z>=50) {g225.sx[v]','if(z>=51) {g225.sx[v]')
    'far-plane-3501'=$original.Replace('origin>=3500','origin>3500')
}
$utf8=New-Object System.Text.UTF8Encoding($false)
foreach($name in $controls.Keys) {
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],$utf8)
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c",'-lm','-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" $inputs "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
$bytes[16+44+20+3]=$bytes[16+44+20+3] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-pixel.bin",$bytes)
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/wrong-pixel.bin",$inputs[2]) "$out/wrong-pixel.log" 1
[IO.File]::WriteAllBytes("$out/truncated.bin",$bytes[0..($bytes.Length-2)])
Invoke-Logged "$out/port.exe" @($inputs[0],"$out/truncated.bin",$inputs[2]) "$out/truncated.log" 2
Get-FileHash $fixture,$model,"$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar,
    "$root/engine/data/pack/client/models","$root/engine/data/pack/client/textures" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/port.log"
'PASS: deterministic Java world fixture; native exact; baseline and seven negative controls rejected'
