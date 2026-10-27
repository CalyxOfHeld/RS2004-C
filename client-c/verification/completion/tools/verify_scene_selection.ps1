param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
Copy-Item "$root/client-c/lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll" "$out/SDL2.dll"
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/scene-selection-reference.bin"
$cache="$root/engine/data/pack/client";$maps="$root/client-c/tests/fixtures/scene-lighting"
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
    "$root/client-c/verification/completion/reference/src/SceneSelectionOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++){
    Invoke-Logged java @('-cp',"$out/classes;$jar",'SceneSelectionOracle',$cache,$maps,"$out/selection-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/selection-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Scene selection fixture changed'}
}
Invoke-Logged make @('-C',"$root/client-c",'-s','bin/test_scene_selection_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_scene_selection_reference.exe" @($fixture) "$out/native.log"
$source="$root/client-c/src/dash3d/scene_selection225.c"
$original=[IO.File]::ReadAllText($source).Replace("`r`n","`n")
$controls=@{
    'camera-upper-clamp'=$original.Replace('g->max_x*128-1','g->max_x*128-128').Replace('g->max_z*128-1','g->max_z*128-128')
    'cycle-step'=$original.Replace('(uint32_t)v->cycle+1','(uint32_t)v->cycle+2')
    'wrong-yaw-slice'=$original.Replace('v->map_yaw=yaw/64','v->map_yaw=yaw/128')
    'wrong-pitch-slice'=$original.Replace('v->map_pitch=(pitch-128)/32','v->map_pitch=(pitch-128)/64')
    'short-minimum-window'=$original.Replace('v->min_x=v->eye_tile_x-25','v->min_x=v->eye_tile_x-24')
    'long-maximum-window'=$original.Replace('v->max_x=v->eye_tile_x+25','v->max_x=v->eye_tile_x+26')
    'height-exclusive'=$original.Replace('subtract(t->height,eye_y)>=2000','subtract(t->height,eye_y)>2000')
    'wide-height-difference'=$original.Replace('static int32_t subtract(int32_t a,int32_t b){return jint((uint32_t)a-(uint32_t)b);}',
        'static int64_t subtract(int32_t a,int32_t b){return (int64_t)a-b;}')
    'level-exclusive'=$original.Replace('t->draw_level<=top_level','t->draw_level<top_level')
    'clear-rejected-loc-flag'=$original.Replace('t->visible=false;t->update=false;t->check_loc_spans=0;',
        't->visible=false;t->update=false;t->contains_locs=false;t->check_loc_spans=0;')
    'clear-accepted-spans'=$original.Replace('t->visible=true;t->update=true;','t->visible=true;t->update=true;t->check_loc_spans=0;')
    'zero-count-has-locs'=$original.Replace('t->contains_locs=t->loc_count>0','t->contains_locs=t->loc_count>=0')
}
$objects=Get-ChildItem -LiteralPath "$root/client-c/build/src" -Filter '*.o' -Recurse |
    Where-Object {$_.FullName -notmatch '[\\/]client[\\/]' -and $_.Name -ne 'scene_selection225.o'} | ForEach-Object {$_.FullName}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include","$root/client-c/tests/test_scene_selection_reference.c")
$libs=@("-L$root/client-c/lib/SDL2/x86_64-w64-mingw32/lib",'-lSDL2','-lws2_32','-lwinmm','-lm','-Wl,--stack,16777216')
foreach($name in $controls.Keys){
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],(New-Object System.Text.UTF8Encoding($false)))
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c")+$objects+$libs+@('-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" @($fixture) "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
# Four complete matrix profiles follow the five header ints.
$offset=20+4*8*32*51*51
# Walk input tile records to the first camera, then damage expected cycle.
for($i=0;$i -lt 4*104*104;$i++){if($bytes[$offset] -eq 1){$offset+=19};$offset++}
$offset+=4+9*4
$bytes[$offset]=$bytes[$offset] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-state.bin",$bytes)
Invoke-Logged "$root/client-c/bin/test_scene_selection_reference.exe" @("$out/wrong-state.bin") "$out/wrong-state.log" 1
$bytes=[IO.File]::ReadAllBytes($fixture)
$stream=[IO.File]::Create("$out/truncated.bin");$stream.Write($bytes,0,$bytes.Length-1);$stream.Dispose()
Invoke-Logged "$root/client-c/bin/test_scene_selection_reference.exe" @("$out/truncated.bin") "$out/truncated.log" 2
Get-FileHash $fixture,$source,"$root/client-c/include/dash3d/scene_selection225.h",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar,
    "$cache/config","$cache/models","$cache/textures","$maps/m50_50.raw","$maps/l50_50.raw" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/native.log"
'PASS: original scene selection reproduces; native exact; fourteen negative controls rejected'
