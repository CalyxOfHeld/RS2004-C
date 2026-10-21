param([string]$Exe, [string]$OutDir)
# Deterministic offline captures used to prove the Phase B loop extraction is behavior-preserving.
$bin = "C:\Users\USER\Documents\2004scape-LostCityRS\2004scape C Rewrite\client-c\bin"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
New-Item -ItemType Directory -Force $OutDir | Out-Null
$common = @("--offline", "--seed", "225", "--fixed-step")
$runs = @{
    "scene"   = @("--capture", "`"$OutDir\scene.bmp`"", "--capture-frame", "60", "--capture-interval", "20")
    "rebuild" = @("--rebuild", "--capture", "`"$OutDir\rebuild.bmp`"", "--capture-frame", "40")
    "walk"    = @("--walk-to", "36", "29", "--capture", "`"$OutDir\walk.bmp`"", "--capture-frame", "160", "--capture-interval", "20")
    "ui"      = @("--input-script", "`"$here\ui.input`"", "--capture", "`"$OutDir\ui.bmp`"", "--capture-frame", "150", "--capture-interval", "10")
}
foreach ($name in $runs.Keys) {
    $args = $common + $runs[$name]
    $p = Start-Process -FilePath (Join-Path $bin $Exe) -ArgumentList $args -RedirectStandardOutput "$OutDir\$name.log" -RedirectStandardError "$OutDir\$name.err" -PassThru -Wait -NoNewWindow
    "$name exit=$($p.ExitCode)"
}
