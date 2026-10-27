param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
Copy-Item "$root/client-c/lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll" "$out/SDL2.dll"
$jar="$root/javaclient/build/libs/rs2client.jar";$cache="$root/engine/data/pack/client";$maps="$root/client-c/tests/fixtures/scene-lighting"
$fixture="$root/client-c/tests/fixtures/painter-trace-reference.bin";$scene="$root/client-c/tests/fixtures/static-scene-reference.bin"
$graphFixture="$root/client-c/tests/fixtures/scene-graph-reference.bin"
$heightsFixture="$root/client-c/tests/fixtures/scene-model-heights-reference.bin"
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0){
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-XDignore.symbol.file','-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",
    "$root/client-c/verification/completion/reference/src/StaticSceneOracle.java",
    "$root/client-c/verification/completion/reference/src/GroundDrawOracle.java",
    "$root/client-c/verification/completion/reference/src/PainterTraceAgent.java",
    "$root/client-c/verification/completion/reference/src/PainterTraceHooks.java",
    "$root/client-c/verification/completion/reference/src/SceneGraphOracle.java") "$out/java-build.log"
Invoke-Logged python @("$PSScriptRoot/painter_trace_controls.py",'agent',"$out/classes","$out/agent.jar") "$out/agent-build.log"
$commonJava=@('-Djava.awt.headless=true','-cp',"$out/classes;$jar",'StaticSceneOracle',$cache,$maps)
Invoke-Logged java ($commonJava+@("$out/untraced-scene.bin")) "$out/untraced.log"
if((Get-FileHash "$out/untraced-scene.bin").Hash -ne (Get-FileHash $scene).Hash){throw 'Unchanged source frame fixture changed'}
for($pass=1;$pass -le 2;$pass++){
    Invoke-Logged java (@("-javaagent:$out/agent.jar","-Drs2.painter.trace=$out/painter-$pass.bin","-Drs2.scene.graph=$out/scene-graph-$pass.bin")+$commonJava+@("$out/traced-scene-$pass.bin")) "$out/traced-$pass.log"
    if((Get-FileHash "$out/traced-scene-$pass.bin").Hash -ne (Get-FileHash $scene).Hash){throw 'Observation changed original scene pixels/state'}
    if((Get-FileHash "$out/painter-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Painter command trace changed'}
    $hooks=Select-String -Path "$out/traced-$pass.log" -Pattern 'PainterTraceAgent: .+ observation hooks='
    if($hooks.Count -ne 2){throw 'Observation hooks did not load in both original classes'}
}
if((Get-FileHash "$out/scene-graph-1.bin").Hash -ne (Get-FileHash "$out/scene-graph-2.bin").Hash){throw 'Original scene graph capture is nondeterministic'}
if((Get-FileHash "$out/scene-graph-1.bin").Hash -ne (Get-FileHash $graphFixture).Hash){throw 'Original scene graph changed from the accepted fixture'}
if((Get-FileHash "$out/scene-graph-1.bin.maxy").Hash -ne (Get-FileHash "$out/scene-graph-2.bin.maxy").Hash){throw 'Original scene model heights are nondeterministic'}
if((Get-FileHash "$out/scene-graph-1.bin.maxy").Hash -ne (Get-FileHash $heightsFixture).Hash){throw 'Original scene model heights changed from the accepted fixture'}
Invoke-Logged python @("$PSScriptRoot/inspect_scene_graph225.py",$graphFixture,$fixture,$heightsFixture) "$out/scene-model-heights-check.log"
Invoke-Logged python @("$PSScriptRoot/painter_trace_controls.py",'check',$fixture,$scene,$out) "$out/trace-check.log"
Invoke-Logged make @('-C',"$root/client-c",'-s','bin/test_painter_trace_reference.exe') "$out/native-build.log"
$exe="$root/client-c/bin/test_painter_trace_reference.exe";$inputs=@("$cache/textures",$fixture)
Invoke-Logged $exe $inputs "$out/native.log"
$objects=Get-ChildItem -LiteralPath "$root/client-c/build/src" -Filter '*.o' -Recurse |
    Where-Object {$_.FullName -notmatch '[\\/]client[\\/]'} | ForEach-Object {$_.FullName}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include","$root/client-c/tests/test_painter_trace_reference.c")
$libs=@("-L$root/client-c/lib/SDL2/x86_64-w64-mingw32/lib",'-lSDL2','-lws2_32','-lwinmm','-lm','-Wl,--stack,16777216')
foreach($entry in @(@('skip-models','PAINTER_TRACE_SKIP_MODELS'),@('skip-ground','PAINTER_TRACE_SKIP_GROUND'))){
    $name=$entry[0];Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("-D$($entry[1])=1")+$objects+$libs+@('-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" $inputs "$out/$name.log" 1
}
# A measured scope limit: ordinary recomputed bounds ALSO match this corpus.
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@('-DPAINTER_TRACE_RECOMPUTE_BOUNDS=1')+$objects+$libs+@('-o',"$out/recompute-bounds.exe")) "$out/recompute-bounds-build.log"
Invoke-Logged "$out/recompute-bounds.exe" $inputs "$out/recompute-bounds.log"
$model="$root/client-c/src/gfx/model.c";$original=[IO.File]::ReadAllText($model).Replace("`r`n","`n")
$fault=$original.Replace('radius=bounds->radius;','radius=0;')
if($fault -ceq $original){throw 'Bounds fault did not change source'}
[IO.File]::WriteAllText("$out/zero-radius.c",$fault,(New-Object System.Text.UTF8Encoding($false)))
$withoutModel=$objects | Where-Object {$_ -notmatch '[\\/]gfx[\\/]model.o$'}
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/zero-radius.c")+$withoutModel+$libs+@('-o',"$out/zero-radius.exe")) "$out/zero-radius-build.log"
Invoke-Logged "$out/zero-radius.exe" $inputs "$out/zero-radius.log" 1
Invoke-Logged $exe @($inputs[0],"$out/reverse-order.bin") "$out/reverse-order.log" 1
Invoke-Logged $exe @($inputs[0],"$out/wrong-pixel.bin") "$out/wrong-pixel.log" 1
Invoke-Logged $exe @($inputs[0],"$out/truncated.bin") "$out/truncated.log" 2
Get-FileHash $fixture,$scene,$graphFixture,$heightsFixture,$model,"$root/client-c/include/gfx/model.h",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/World.java",
    "$root/javaclient/src/main/java/jagex2/graphics/Model.java",$jar,
    "$cache/config","$cache/models","$cache/textures","$maps/m50_50.raw","$maps/l50_50.raw" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/native.log"
'PASS: observations preserve original frames; trace reproduces; C command replay exact; six faults rejected'
