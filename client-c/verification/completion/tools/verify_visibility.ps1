param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
Copy-Item "$root/client-c/lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll" "$out/SDL2.dll"
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/visibility-reference.bin"
$ground="$root/client-c/tests/fixtures/ground-reference.bin"
if((Get-FileHash $ground).Hash -ne '93A5B300BE31C9AAFC57A778496AF20DC20ACD16DACF31A8001AA4413824F90F'){throw 'Unexpected real-map input fixture'}
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0){
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/client-c/verification/completion/reference/src/TileConstructionOracle.java",
    "$root/client-c/verification/completion/reference/src/VisibilityOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++){
    Invoke-Logged java @('-cp',"$out/classes;$jar",'VisibilityOracle',$ground,"$out/visibility-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/visibility-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Visibility fixture changed'}
}
Invoke-Logged make @('-C',"$root/client-c",'-s','bin/test_visibility_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_visibility_reference.exe" @($fixture) "$out/native.log"
$source="$root/client-c/src/dash3d/visibility225.c"
$original=[IO.File]::ReadAllText($source).Replace("`r`n","`n")
$controls=@{
    'yaw-wrap-32'=$original.Replace('(yi+1)%31','(yi+1)%32')
    'clear-retained-edges'=$original.Replace('VisibilityProjection225 *p=&state->projection;',
        'for(int a=0;a<8;a++)for(int b=0;b<32;b++)for(int c=0;c<51;c++)for(int d=0;d<51;d++)state->matrix[a][b][c][d]=false;'+[char]10+'    VisibilityProjection225 *p=&state->projection;')
    'near-exclusive'=$original.Replace('depth<50','depth<=50')
    'far-exclusive'=$original.Replace('depth>3500','depth>=3500')
    'strict-viewport'=$original.Replace('sx<=p->right','sx<p->right').Replace('sy<=p->bottom','sy<p->bottom')
    'omit-next-pitch'=$original.Replace('samples[pi+1][yi][xx][zz] || samples[pi+1][next][xx][zz]','false || false')
    'omit-neighborhood'=$original.Replace('int dx=-1;dx<=1','int dx=0;dx<=0').Replace('int dz=-1;dz<=1','int dz=0;dz<=0')
    'wide-yaw-products'=$original.Replace('shr16(add(mul(z,p->sin_yaw),mul(x,p->cos_yaw)))','(int32_t)(((int64_t)z*p->sin_yaw+(int64_t)x*p->cos_yaw)>>16)')
}
$objects=Get-ChildItem -LiteralPath "$root/client-c/build/src" -Filter '*.o' -Recurse |
    Where-Object {$_.FullName -notmatch '[\\/]client[\\/]' -and $_.Name -ne 'visibility225.o'} | ForEach-Object {$_.FullName}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include","$root/client-c/tests/test_visibility_reference.c")
$libs=@("-L$root/client-c/lib/SDL2/x86_64-w64-mingw32/lib",'-lSDL2','-lws2_32','-lwinmm','-lm','-Wl,--stack,16777216')
foreach($name in $controls.Keys){
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],(New-Object System.Text.UTF8Encoding($false)))
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c")+$objects+$libs+@('-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" @($fixture) "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
# First matrix byte follows magic/count, 14 input ints, ten final-state ints.
$bytes[104]=$bytes[104] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-matrix.bin",$bytes)
Invoke-Logged "$root/client-c/bin/test_visibility_reference.exe" @("$out/wrong-matrix.bin") "$out/wrong-matrix.log" 1
$bytes=[IO.File]::ReadAllBytes($fixture)
$stream=[IO.File]::Create("$out/truncated.bin");$stream.Write($bytes,0,$bytes.Length-1);$stream.Dispose()
Invoke-Logged "$root/client-c/bin/test_visibility_reference.exe" @("$out/truncated.bin") "$out/truncated.log" 2
Get-FileHash $fixture,$ground,$source,"$root/client-c/include/dash3d/visibility225.h",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/native.log"
'PASS: original visibility reproduces; native exact; ten negative controls rejected'
