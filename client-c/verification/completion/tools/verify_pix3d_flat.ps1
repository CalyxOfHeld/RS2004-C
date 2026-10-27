param([Parameter(Mandatory=$true)][string]$OutDir)
# Isolated oracle/port/flag checks. Never builds or replaces the installed client.
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/../../../..").Path
if (Test-Path -LiteralPath $OutDir) { throw 'Choose a new evidence output directory' }
New-Item -ItemType Directory -Path "$OutDir/classes" | Out-Null
$out = (Resolve-Path -LiteralPath $OutDir).Path
$jar = "$root/javaclient/build/libs/rs2client.jar"
$include = "-I$root/client-c/include"
$test = "$root/client-c/tests/test_pix3d_flat_reference.c"
$pix2d = "$root/client-c/src/gfx/pix2d.c"
$pix3d = "$root/client-c/src/gfx/pix3d.c"
$flat = "$root/client-c/src/gfx/pix3d_flat225.c"
function Invoke-Logged($Program, $Arguments, $Log, [int]$Expected=0) {
    # PowerShell 5.1 converts redirected native stderr into error records.
    # Mismatch diagnostics are expected for the baseline/negative controls.
    $ErrorActionPreference = 'Continue'
    & $Program @Arguments *> $Log
    if ($LASTEXITCODE -ne $Expected) { throw "$Program exit $LASTEXITCODE; expected $Expected (see $Log)" }
}
Invoke-Logged python @("$PSScriptRoot/port_pix3d_flat.py",'--check') "$out/generated.log"
Invoke-Logged javac @('-encoding','UTF-8','-cp',$jar,'-d',"$out/classes",
    "$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",
    "$root/client-c/verification/completion/reference/src/Pix3DFlatOracle.java") "$out/java-build.log"
# Source class first, unchanged jar only supplies dependencies.
Invoke-Logged java @('-cp',"$out/classes;$jar",'Pix3DFlatOracle',"$out/flat.bin") "$out/java.log"
$fixture = "$root/client-c/tests/fixtures/pix3d-flat-reference.bin"
if ((Get-FileHash $fixture).Hash -ne (Get-FileHash "$out/flat.bin").Hash) { throw 'Oracle fixture changed' }
$common = @('-std=c99','-Wall','-Wextra','-Werror','-O2',$include,$test,$pix2d,$pix3d)
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@('-DPIX3D225_BASELINE','-lm','-o',"$out/legacy.exe")) "$out/legacy-build.log"
Invoke-Logged "$out/legacy.exe" @("$out/flat.bin") "$out/legacy.log" 1
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($flat,'-lm','-o',"$out/port.exe")) "$out/port-build.log"
Invoke-Logged "$out/port.exe" @("$out/flat.bin") "$out/port.log"
Invoke-Logged x86_64-w64-mingw32-gcc ($common+@($flat,'-DRS2_PIX3D_REV225_FLAT=1','-DPIX3D225_VIA_PUBLIC','-lm','-o',"$out/flag.exe")) "$out/flag-build.log"
Invoke-Logged "$out/flag.exe" @("$out/flat.bin") "$out/flag.log"
$utf8 = New-Object System.Text.UTF8Encoding($false)
$original = [IO.File]::ReadAllText($flat)
$controls = @{
    'alpha-current-word' = $original.Replace('next=dst[offset+1]','next=dst[offset]')
    'clip-right-edge' = $original.Replace('right > pix2d_safe_width) right=pix2d_safe_width',
                                         'right > pix2d_clip_right) right=pix2d_clip_right')
}
foreach ($name in $controls.Keys) {
    if ($controls[$name] -ceq $original) { throw "Control $name failed to alter the source" }
    [IO.File]::WriteAllText("$out/$name.c",$controls[$name],$utf8)
    Invoke-Logged x86_64-w64-mingw32-gcc ($common+@("$out/$name.c",'-lm','-o',"$out/$name.exe")) "$out/$name-build.log"
    Invoke-Logged "$out/$name.exe" @("$out/flat.bin") "$out/$name.log" 1
}
Get-FileHash $fixture,$flat,"$root/javaclient/src/main/java/jagex2/graphics/Pix3D.java",$jar |
    Select-Object Path,Hash | ConvertTo-Json | Set-Content "$out/hashes.json"
Get-Content "$out/java.log","$out/port.log","$out/flag.log"
'PASS: original mismatch reproduced; port and flag exact; both negative controls rejected'
