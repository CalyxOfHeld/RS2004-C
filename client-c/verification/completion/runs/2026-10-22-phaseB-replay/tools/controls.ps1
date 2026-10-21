param([string]$Log, [string]$OutDir, [string]$Password)
# Positive replay of a session log plus negative controls that must fail.
$bin = "C:\Users\USER\Documents\2004scape-LostCityRS\2004scape C Rewrite\client-c\bin"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
New-Item -ItemType Directory -Force $OutDir | Out-Null
python "$here\tamper.py" $Log "$OutDir\neg"
$wrong = $Password.Substring(0, $Password.Length - 1) + $(if ($Password.EndsWith("x")) { "y" } else { "x" })
$cases = @(
    @("positive", $Log, $Password, 0),
    @("flipped-inbound", "$OutDir\neg\flipped-inbound.rec", $Password, 1),
    @("truncated", "$OutDir\neg\truncated.rec", $Password, 1),
    @("wrong-password-same-length", $Log, $wrong, 1)
)
$allOk = $true
foreach ($c in $cases) {
    $name = $c[0]; $dir = "$OutDir\$name"
    New-Item -ItemType Directory -Force $dir | Out-Null
    $env:RS2_PASSWORD = $c[2]
    $p = Start-Process -FilePath "$bin\rs2replay.exe" -ArgumentList @("`"$($c[1])`"", "--out", "`"$dir`"") -RedirectStandardOutput "$dir\replay.log" -RedirectStandardError "$dir\replay.err" -PassThru -Wait -NoNewWindow
    $verdict = (Select-String -Path "$dir\replay.log" -Pattern "PASS REPLAY|FAIL REPLAY" | Select-Object -Last 1).Line
    $why = (Get-Content "$dir\replay.err" | Where-Object { $_ -match "desync" } | Select-Object -First 1)
    $expected = $p.ExitCode -eq $c[3]
    if (-not $expected) { $allOk = $false }
    "{0,-28} exit={1} expected={2} {3}" -f $name, $p.ExitCode, $c[3], $(if ($expected) { "OK" } else { "UNEXPECTED" })
    "    $verdict"
    if ($why) { "    $why" }
}
Remove-Item Env:RS2_PASSWORD
"controls " + $(if ($allOk) { "ALL AS EXPECTED" } else { "FAILED" })
