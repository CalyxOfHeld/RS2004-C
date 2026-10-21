param(
    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]] $ClientArgs
)

$clientRoot = Join-Path $PSScriptRoot 'client-c'
$clientPath = Join-Path $clientRoot 'bin/rs2client.exe'
$bridgeUpdate = Join-Path $clientRoot 'bin/rs2client_bridge.exe'
if ((Test-Path -LiteralPath $bridgeUpdate -PathType Leaf) -and
    (-not (Test-Path -LiteralPath $clientPath -PathType Leaf) -or
     (Get-Item -LiteralPath $bridgeUpdate).LastWriteTimeUtc -gt (Get-Item -LiteralPath $clientPath).LastWriteTimeUtc)) {
    try {
        Copy-Item -LiteralPath $bridgeUpdate -Destination $clientPath -Force -ErrorAction Stop
    } catch {
        # An already-open client can lock the installed executable.
        $clientPath = $bridgeUpdate
        Write-Host 'Starting the updated bridge build. Close the older client before signing in to the same account.'
    }
}
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

# Account saves are persistent. Use a new disposable username for fresh tests.

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
