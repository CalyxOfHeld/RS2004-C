param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
Copy-Item "$root/client-c/lib/SDL2/x86_64-w64-mingw32/bin/SDL2.dll" "$out/SDL2.dll"
$jar="$root/javaclient/build/libs/rs2client.jar"
$fixture="$root/client-c/tests/fixtures/tile-construction-reference.bin"
$ground="$root/client-c/tests/fixtures/ground-reference.bin"
if((Get-FileHash $ground).Hash -ne '93A5B300BE31C9AAFC57A778496AF20DC20ACD16DACF31A8001AA4413824F90F'){throw 'Unexpected real-map input fixture'}
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0){
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/type/TileUnderlay.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/type/TileOverlay.java",
    "$root/client-c/verification/completion/reference/src/TileConstructionOracle.java") "$out/java-build.log"
for($pass=1;$pass -le 2;$pass++){
    Invoke-Logged java @('-cp',"$out/classes;$jar",'TileConstructionOracle',$ground,"$out/tiles-$pass.bin") "$out/java-$pass.log"
    if((Get-FileHash "$out/tiles-$pass.bin").Hash -ne (Get-FileHash $fixture).Hash){throw 'Tile fixture changed'}
}
Invoke-Logged make @('-C',"$root/client-c",'-s','bin/test_tile_construction_reference.exe') "$out/native-build.log"
Invoke-Logged "$root/client-c/bin/test_tile_construction_reference.exe" @($fixture) "$out/native.log"
$source="$root/client-c/src/dash3d/world3d.c"
$original=[IO.File]::ReadAllText($source).Replace("`r`n","`n")
$controls=@{
    'shape-zero-texture'=$original.Replace('tile->underlay.texture_id = -1;','tile->underlay.texture_id = texture_id;')
    'shape-zero-flat'=$original.Replace('tile->underlay.flat = false;','tile->underlay.flat = (y_sw == y_se && y_sw == y_ne && y_sw == y_nw);')
    'wide-midpoint'=$original.Replace('uint32_t sum = (uint32_t)a + (uint32_t)b;','return (int32_t)(((int64_t)a + b) >> 1);'+[char]10+'    uint32_t sum = (uint32_t)a + (uint32_t)b;')
    'unsigned-midpoint'=$original.Replace('(sum >> 1) | (sum & UINT32_C(0x80000000))','(sum >> 1)')
    'reverse-rotation'=$original.Replace('(va - rotation) & 3','(va + rotation) & 3')
    'clear-retained-layers'=$original.Replace('GroundTile *tile = &w->tiles[level][x][z];','GroundTile *tile = &w->tiles[level][x][z];'+[char]10+'    tile->has_underlay = false; tile->has_overlay = false;')
}
$objects=Get-ChildItem -LiteralPath "$root/client-c/build/src" -Filter '*.o' -Recurse |
    Where-Object {$_.FullName -notmatch '[\\/]client[\\/]' -and $_.Name -ne 'world3d.o'} | ForEach-Object {$_.FullName}
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include", "$root/client-c/tests/test_tile_construction_reference.c")
$libs=@("-L$root/client-c/lib/SDL2/x86_64-w64-mingw32/lib",'-lSDL2','-lws2_32','-lwinmm','-lm','-Wl,--stack,16777216')
foreach($name in $controls.Keys){
    if($controls[$name] -ceq $original){throw "Control $name did not change source"}
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],(New-Object System.Text.UTF8Encoding($false)))
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c")+$objects+$libs+@('-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" @($fixture) "$out/$name.log" 1
}
$bytes=[IO.File]::ReadAllBytes($fixture)
$bytes[100]=$bytes[100] -bxor 1
[IO.File]::WriteAllBytes("$out/wrong-field.bin",$bytes)
Invoke-Logged "$root/client-c/bin/test_tile_construction_reference.exe" @("$out/wrong-field.bin") "$out/wrong-field.log" 1
$bytes=[IO.File]::ReadAllBytes($fixture)
$stream=[IO.File]::Create("$out/truncated.bin");$stream.Write($bytes,0,$bytes.Length-1);$stream.Dispose()
Invoke-Logged "$root/client-c/bin/test_tile_construction_reference.exe" @("$out/truncated.bin") "$out/truncated.log" 2
Get-FileHash $fixture,$ground,$source,"$root/javaclient/src/main/java/jagex2/dash3d/World3D.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/type/TileOverlay.java",
    "$root/javaclient/src/main/java/jagex2/dash3d/type/TileUnderlay.java",$jar |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/native.log"
'PASS: original tile construction reproduces; native exact; eight negative controls rejected'
