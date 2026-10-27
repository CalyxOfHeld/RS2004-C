param([Parameter(Mandatory=$true)][string]$OutDir)
# Isolated source oracle, legacy baseline, direct/flag paths and fault controls.
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir) { throw 'Choose a new evidence output directory' }
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
$source="$root/client-c/src/gfx/pix3d_gouraud225.c"
$fixture="$root/client-c/tests/fixtures/pix3d-gouraud-reference.bin"
function Invoke-Logged($Program,$Arguments,$Log,[int]$Expected=0) {
    $ErrorActionPreference='Continue' # Native stderr is diagnostic, exit code is authoritative.
    & $Program @Arguments *> $Log
    if($LASTEXITCODE -ne $Expected) { throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)" }
}
Invoke-Logged python @("$PSScriptRoot/port_pix3d_gouraud.py",'--check') "$out/generated.log"
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",
    "$root/client-c/verification/completion/reference/src/Pix3DGouraudOracle.java") "$out/java-build.log"
Invoke-Logged java @('-cp',"$out/classes;$jar",'Pix3DGouraudOracle',"$out/gouraud.bin") "$out/java.log"
if((Get-FileHash $fixture).Hash -ne (Get-FileHash "$out/gouraud.bin").Hash) { throw 'Oracle fixture changed' }
$common=@('-std=c99','-Wall','-Wextra','-Werror','-O2',"-I$root/client-c/include",
    "$root/client-c/tests/test_pix3d_gouraud_reference.c","$root/client-c/src/gfx/pix2d.c",
    "$root/client-c/src/gfx/pix3d.c","$root/client-c/src/gfx/pix3d_flat225.c")
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@('-DPIX3D225_BASELINE','-lm','-o',"$out/legacy.exe")) "$out/legacy-build.log"
Invoke-Logged "$out/legacy.exe" @("$out/gouraud.bin") "$out/legacy.log" 1
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($source,'-lm','-o',"$out/port.exe")) "$out/port-build.log"
Invoke-Logged "$out/port.exe" @("$out/gouraud.bin") "$out/port.log"
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($source,'-DRS2_PIX3D_REV225=1','-DPIX3D225_VIA_PUBLIC','-lm','-o',"$out/flag.exe")) "$out/flag-build.log"
Invoke-Logged "$out/flag.exe" @("$out/gouraud.bin") "$out/flag.log"
$utf8=New-Object Text.UTF8Encoding($false)
$original=[IO.File]::ReadAllText($source)
$controls=@{
    'alpha-current-word'=$original.Replace('next=dst[offset+1]','next=dst[offset]')
    'clip-right-edge'=$original.Replace('right>pix2d_safe_width) right=pix2d_safe_width',
                                       'right>pix2d_clip_right) right=pix2d_clip_right')
    'jagged-disabled'=$original.Replace('if(pix3d_jagged)','if(false)')
    'reciprocal-shift'=$original.Replace('pix3d_div_table[groups]),15)','pix3d_div_table[groups]),16)')
}
foreach($name in $controls.Keys) {
    if($controls[$name] -ceq $original) { throw "Control $name did not alter source" }
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],$utf8)
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c",'-lm','-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" @("$out/gouraud.bin") "$out/$name.log" 1
}
Get-FileHash $fixture,$source,"$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",$jar |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/java.log","$out/port.log","$out/flag.log"
'PASS: legacy mismatch; exact direct/flag paths; all four fault controls rejected'
