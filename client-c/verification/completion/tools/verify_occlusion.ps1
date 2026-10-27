param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
Copy-Item "$root/client-c/lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll" "$out/SDL2.dll"
$jar="$root/javaclient/build/libs/rs2client.jar";$cache="$root/engine/data/pack/client";$maps="$root/client-c/tests/fixtures/scene-lighting"
$fixture="$root/client-c/tests/fixtures/occlusion-reference.bin";$ground="$root/client-c/tests/fixtures/ground-reference.bin"
if((Get-FileHash $ground).Hash -ne '93A5B300BE31C9AAFC57A778496AF20DC20ACD16DACF31A8001AA4413824F90F'){throw 'Unexpected real-map inputs'}
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0){
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/client-c/verification/completion/reference/src/StaticSceneOracle.java",
    "$root/client-c/verification/completion/reference/src/TileConstructionOracle.java",
    "$root/client-c/verification/completion/reference/src/OcclusionOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++){
    Invoke-Logged java @('-cp',"$out/classes;$jar",'OcclusionOracle',$cache,$maps,$ground,"$out/occlusion-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/occlusion-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Occlusion fixture changed'}
}
Invoke-Logged make @('-C',"$root/client-c",'-s','bin/test_occlusion_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_occlusion_reference.exe" @($fixture) "$out/native.log"
$source="$root/client-c/src/dash3d/occlusion225.c"
$original=[IO.File]::ReadAllText($source).Replace("`r`n","`n")
$controls=@{
    'near-32-inclusive'=$original.Replace('if(d>32)','if(d>=32)')
    'roof-128-inclusive'=$original.Replace('if(d<=128)continue','if(d<128)continue')
    'ignore-visibility'=$original.Replace('if(!visible)continue;','if(!visible && false)continue;')
    'reverse-active-order'=$original.Replace('Occluder225 *o=&s->pool[i];','size_t index=s->count-1-i;Occluder225 *o=&s->pool[index];').Replace('s->active[s->active_count++]=(uint16_t)i;','s->active[s->active_count++]=(uint16_t)index;')
    'wide-slopes'=$original.Replace('return jint((uint32_t)sub(a,eye)<<8)/distance;','return (int32_t)(((int64_t)a-eye)*256/distance);')
    'wide-point-products'=$original.Replace('return add(base,shr8(mul(delta,distance)));','(void)shr8(mul(delta,distance));return add(base,jint((uint32_t)(((int64_t)delta*distance)>>8)));')
    'strict-point-bounds'=$original.Replace('z<=b','z<b').Replace('x<=b','x<b').Replace('y<=e','y<e').Replace('z<=e','z<e')
    'include-occluder-plane'=$original.Replace('if(d<=0)continue;','if(d<0)continue;')
    'positive-cache-first'=$original.Replace('if(*cache==neg(s->cycle))return false;'+[char]10+'    if(*cache==s->cycle)return true;',
        'if(*cache==s->cycle)return true;'+[char]10+'    if(*cache==neg(s->cycle))return false;')
    'change-positive-cache-tag'=$original.Replace('if(*cache==s->cycle)return true;','if(*cache==s->cycle){*cache=neg(s->cycle);return true;}')
    'expanded-loc-far-edge'=$original.Replace('c=x1*128-1,d=z1*128-1','c=(x1+1)*128-1,d=(z1+1)*128-1')
    'invert-wall-tile-gate'=$original.Replace('if(!world3d_tile_occluded225(s,l,x,z))return false;','if(!world3d_tile_occluded225(s,l,x,z))return true;')
}
$objects=Get-ChildItem -LiteralPath "$root/client-c/build/src" -Filter '*.o' -Recurse |
    Where-Object {$_.FullName -notmatch '[\\/]client[\\/]' -and $_.Name -ne 'occlusion225.o'} | ForEach-Object {$_.FullName}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include","$root/client-c/tests/test_occlusion_reference.c")
$libs=@("-L$root/client-c/lib/SDL2/x86_64-w64-mingw32/lib",'-lSDL2','-lws2_32','-lwinmm','-lm','-Wl,--stack,16777216')
foreach($name in $controls.Keys){
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],(New-Object System.Text.UTF8Encoding($false)))
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c")+$objects+$libs+@('-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" @($fixture) "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
$offset=20+4*51*51+4*105*105*4+4
# Skip first group inputs/pool, count/active indices, then flip one expected mode.
$poolCount=[int]([uint32]$bytes[$offset+28] -shl 24 -bor [uint32]$bytes[$offset+29] -shl 16 -bor [uint32]$bytes[$offset+30] -shl 8 -bor [uint32]$bytes[$offset+31])
$offset+=32+$poolCount*72
$activeCount=[int]([uint32]$bytes[$offset] -shl 24 -bor [uint32]$bytes[$offset+1] -shl 16 -bor [uint32]$bytes[$offset+2] -shl 8 -bor [uint32]$bytes[$offset+3])
$offset+=4+$activeCount*4+11*4
$bytes[$offset]=$bytes[$offset] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-mode.bin",$bytes)
Invoke-Logged "$root/client-c/bin/test_occlusion_reference.exe" @("$out/wrong-mode.bin") "$out/wrong-mode.log" 1
$bytes=[IO.File]::ReadAllBytes($fixture)
$stream=[IO.File]::Create("$out/truncated.bin");$stream.Write($bytes,0,$bytes.Length-1);$stream.Dispose()
Invoke-Logged "$root/client-c/bin/test_occlusion_reference.exe" @("$out/truncated.bin") "$out/truncated.log" 2
Get-FileHash $fixture,$source,"$root/client-c/include/dash3d/occlusion225.h",$ground,
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar,
    "$cache/config","$cache/models","$cache/textures","$maps/m50_50.raw","$maps/l50_50.raw" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/native.log"
'PASS: original occlusion reproduces; native exact; fourteen negative controls rejected'
