param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $ClientArgs
)

$clientRoot = Join-Path $PSScriptRoot 'client-c'
$clientPath = Join-Path $clientRoot 'bin/rs2client.exe'
if (-not (Test-Path -LiteralPath $clientPath -PathType Leaf)) {
    Write-Error 'The native client executable is missing. Run test_client.bat to build it.'
    exit 1
}

$offlinePreview = $ClientArgs -contains '--offline'
$defaultCredentialsApplied = $false
if (-not $offlinePreview -and -not $env:RS2_USERNAME -and -not $env:RS2_PASSWORD) {
    # Existing local test account. Supply both environment variables to use another account.
    $env:RS2_USERNAME = 'antigravity'
    $env:RS2_PASSWORD = 'antigravity'
    $defaultCredentialsApplied = $true
}

# Automatically reset player save before launch so each session starts fresh on Tutorial Island
if (-not $offlinePreview -and $env:RS2_USERNAME) {
    $saveDir = Join-Path $PSScriptRoot 'engine/data/players'
    if (Test-Path -LiteralPath $saveDir) {
        Get-ChildItem -Path $saveDir -Recurse -Filter "$($env:RS2_USERNAME).sav" -ErrorAction SilentlyContinue |
            Remove-Item -Force -ErrorAction SilentlyContinue
        Write-Host "Reset player save file for fresh session ($($env:RS2_USERNAME))."
    }
}

$exitCode = 1
try {
    Set-Location -LiteralPath $clientRoot
    if ($offlinePreview) {
        Write-Host 'Starting the explicit offline scene preview.'
    } else {
        Write-Host 'Connecting to the local revision-225 server on port 43595.'
    }
    & $clientPath '--port' '43595' @ClientArgs
    $exitCode = $LASTEXITCODE
} finally {
    if ($defaultCredentialsApplied) {
        Remove-Item Env:RS2_USERNAME, Env:RS2_PASSWORD -ErrorAction SilentlyContinue
    }
}
exit $exitCode
