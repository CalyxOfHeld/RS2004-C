param(
    [Parameter(Mandatory = $true)][string]$OutDir,
    [Parameter(Mandatory = $true)][string]$InputScript,
    [int]$Frames = 440,
    [int]$Port = 43595,
    [string]$Client = ""
)
# Records a live session for parity: snapshots every save first, runs
# rs2client --record --trace with autologin from RS2_USERNAME/RS2_PASSWORD
# (never written to disk), then reports which saves changed.
$ErrorActionPreference = "Continue"
$root = (Resolve-Path "$PSScriptRoot\..\..\..\..").Path
if (-not $Client) { $Client = "$root\client-c\bin\rs2client.exe" }
$Client = (Resolve-Path -LiteralPath $Client).Path
if (-not $env:RS2_USERNAME -or -not $env:RS2_PASSWORD) { throw "set RS2_USERNAME and RS2_PASSWORD" }
New-Item -ItemType Directory -Force "$OutDir\saves-before" | Out-Null
$OutDir = (Resolve-Path $OutDir).Path; $InputScript = (Resolve-Path $InputScript).Path
Copy-Item "$root\engine\data\players\main\*.sav" "$OutDir\saves-before\"
Copy-Item $InputScript "$OutDir\input.txt"
$p = Start-Process -FilePath $Client -ArgumentList @(
    "--port", "$Port", "--trace", "--record", "`"$OutDir\session.rec`"", "--input-script", "`"$OutDir\input.txt`"",
    "--capture", "`"$OutDir\live.bmp`"", "--capture-frame", "$Frames") `
    -RedirectStandardOutput "$OutDir\record.log" -RedirectStandardError "$OutDir\record.err" -PassThru -NoNewWindow -WorkingDirectory "$root\client-c"
if (-not $p.WaitForExit(($Frames / 5 + 60) * 1000)) { $p.Kill(); "record: KILLED after timeout" }
Select-String -Path "$OutDir\record.log" -Pattern "\[LOGIN\]|\[PARITY\]|logout ->" | ForEach-Object { "record: " + $_.Line }
Get-Content "$OutDir\record.err" | ForEach-Object { "record stderr: " + $_ }
$changed = @()
foreach ($save in Get-ChildItem "$OutDir\saves-before" -Filter *.sav) {
    $now = "$root\engine\data\players\main\$($save.Name)"
    if (-not (Test-Path $now) -or (Get-FileHash $now).Hash -ne (Get-FileHash $save.FullName).Hash) { $changed += $save.Name }
}
"record: saves changed: " + $(if ($changed.Count) { $changed -join ", " } else { "none" })
