param([Parameter(Mandatory=$true)][string]$OutDir)
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Choose a new evidence directory'}
New-Item -ItemType Directory -Path $OutDir | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$graph="$root/client-c/tests/fixtures/scene-graph-reference.bin"
$heights="$root/client-c/tests/fixtures/scene-model-heights-reference.bin"
$trace="$root/client-c/tests/fixtures/painter-trace-reference.bin"
$src="$root/client-c/src/dash3d/scene_traversal225.c"
$occlusion="$root/client-c/src/dash3d/occlusion225.c"
$include="-I$root/client-c/include"
function Invoke-Checked($Program,$Arguments,$Log,[int]$Expected=0){
    $ErrorActionPreference='Continue'
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected){throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)"}
}
Invoke-Checked python @("$PSScriptRoot/inspect_scene_graph225.py",$graph,$trace,$heights) "$out/fixture-check.log"
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',$include,$src,$occlusion)
Invoke-Checked x86_64-w64-mingw32-gcc (@('-shared')+$common+@('-o',"$out/traversal.dll")) "$out/build.log"
Invoke-Checked python @("$PSScriptRoot/run_scene_traversal225.py",$graph,$heights,$trace,"$out/traversal.dll") "$out/generated-order.log"
$lines=Get-Content "$out/generated-order.log"
if($lines.Count -ne 12){throw 'Wrong generated-order frame count'}
foreach($line in $lines){
    if($line -notmatch '^frame=(\d+) traversal=True calls=(\d+)/(\d+) remaining=0 first_mismatch=None$'){
        throw "Unexpected generated-order line: $line"
    }
    if($Matches[2] -ne $Matches[3]){throw "Callback count mismatch: $line"}
}
Invoke-Checked x86_64-w64-mingw32-gcc (@('-shared','-DSCENE225_FAULT_NO_MOVE')+$common+@('-o',"$out/no-move.dll")) "$out/no-move-build.log"
Invoke-Checked python @("$PSScriptRoot/run_scene_traversal225.py",$graph,$heights,$trace,"$out/no-move.dll") "$out/no-move-control.log" 1
if(-not (Select-String -Path "$out/no-move-control.log" -Pattern 'first_mismatch=\d+')){throw 'Queue fault was not detected'}
Invoke-Checked x86_64-w64-mingw32-gcc (@('-std=c99','-Wall','-Wextra','-Werror','-O2',$include,
    "$root/client-c/tests/test_scene_traversal225.c",$src,$occlusion,'-o',"$out/smoke.exe")) "$out/smoke-build.log"
Invoke-Checked "$out/smoke.exe" @() "$out/smoke.log"
Invoke-Checked x86_64-w64-mingw32-gcc (@('-DSCENE225_FAULT_TIE_LAST','-std=c99','-Wall','-Wextra','-Werror','-O2',$include,
    "$root/client-c/tests/test_scene_traversal225.c",$src,$occlusion,'-o',"$out/tie-last.exe")) "$out/tie-last-build.log"
Invoke-Checked "$out/tie-last.exe" @() "$out/tie-last-control.log" 6
Get-FileHash $graph,$heights,$trace,$src,"$root/client-c/include/dash3d/scene_traversal225.h" |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/generated-order.log"
'PASS: 12 original generated-order frames exact; queue and location-tie faults rejected; focused smoke passed'
