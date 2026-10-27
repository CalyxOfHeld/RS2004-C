param(
    [Parameter(Mandatory=$true)][string]$OutDir,
    [Parameter(Mandatory=$true)][string]$Account,
    [Parameter(Mandatory=$true)][string]$Client,
    [Parameter(Mandatory=$true)][string]$Replay,
    [Parameter(Mandatory=$true)][string]$OptInReplay,
    [Parameter(Mandatory=$true)][string]$InputScript,
    [int]$Frames=440
)
# Fresh recording and immediate comparisons, with credentials only in memory.
# Use an unused disposable account. The existing server must already be ready.
# Overall exit remains the full parity result; open pixel gates return 1.
$ErrorActionPreference='Stop'
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if($Account -notmatch '^[a-z0-9]{1,12}$') { throw 'Use a simple unused disposable account name' }
if(Test-Path -LiteralPath $OutDir) { throw 'Choose a new evidence directory' }
$players="$root/engine/data/players"
if(Get-ChildItem $players -Recurse -Filter "$Account.sav") { throw 'Account already has a save; choose an unused name' }
$Client=(Resolve-Path -LiteralPath $Client).Path
$Replay=(Resolve-Path -LiteralPath $Replay).Path
$OptInReplay=(Resolve-Path -LiteralPath $OptInReplay).Path
$InputScript=(Resolve-Path -LiteralPath $InputScript).Path
New-Item -ItemType Directory -Path $OutDir | Out-Null
$out=(Resolve-Path -LiteralPath $OutDir).Path
$before=@(Get-ChildItem $players -Recurse -Filter *.sav | ForEach-Object {
    [pscustomobject]@{Path=$_.FullName;Hash=(Get-FileHash -LiteralPath $_.FullName).Hash}
})
$before | ConvertTo-Json | Set-Content "$out/saves-before.json"
$oldUser=$env:RS2_USERNAME; $oldPassword=$env:RS2_PASSWORD
$rng=[Security.Cryptography.RandomNumberGenerator]::Create()
$bytes=New-Object byte[] 12
$rng.GetBytes($bytes); $rng.Dispose()
$parityExit=2
try {
    $env:RS2_USERNAME=$Account
    $env:RS2_PASSWORD=[Convert]::ToBase64String($bytes).Replace('+','a').Replace('/','b')
    $ErrorActionPreference='Continue' # Redirect native stderr and check results explicitly.
    & powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot/record_live.ps1" `
        -OutDir "$out/record" -InputScript $InputScript -Client $Client -Frames $Frames *> "$out/record-driver.log"
    if($LASTEXITCODE -ne 0) { throw 'Recording driver failed' }
    & powershell -NoProfile -ExecutionPolicy Bypass -File "$PSScriptRoot/parity.ps1" `
        -Log "$out/record/session.rec" -OutDir "$out/parity" -Replay $Replay *> "$out/parity-driver.log"
    $parityExit=$LASTEXITCODE
    Set-Content "$out/parity-driver.exit" $parityExit
    if(-not (Select-String -LiteralPath "$out/parity/c/replay-state.log" -Pattern '^PASS REPLAY:' -Quiet)) {
        throw 'Default C replay did not pass'
    }
    if(-not (Select-String -LiteralPath "$out/parity/java/java-report.txt" -Pattern '^RESULT MATCH$' -Quiet)) {
        throw 'Java protocol comparison did not match'
    }
    New-Item -ItemType Directory -Path "$out/opt-in-replay" | Out-Null
    & $OptInReplay "$out/record/session.rec" --out "$out/opt-in-replay" --allow-frame-differences `
        --regions "$out/parity/java/regions.txt" --region-hashes "$out/opt-in-replay/c-regions.txt" *> "$out/opt-in-replay/replay.log"
    $optInExit=$LASTEXITCODE
    Set-Content "$out/opt-in-replay/replay.exit" $optInExit
    if($optInExit -ne 0) { throw 'Opt-in stream replay failed' }
    & python "$PSScriptRoot/parity_compare.py" "$out/parity/java/java-regions.txt" `
        "$out/opt-in-replay/c-regions.txt" "$out/parity/java/java-report.txt" *> "$out/opt-in-parity-summary.txt"
    Set-Content "$out/opt-in-parity.exit" $LASTEXITCODE
    # Controls must still fail when framebuffer differences are permitted.
    New-Item -ItemType Directory -Path "$out/controls" | Out-Null
    $utf8=New-Object Text.UTF8Encoding($false)
    $lines=[IO.File]::ReadAllLines("$out/record/session.rec")
    $changed=[string[]]$lines.Clone()
    $writeIndex=-1
    for($i=0;$i -lt $changed.Length;$i++) { if($changed[$i].StartsWith('NX ')) { $writeIndex=$i; break } }
    if($writeIndex -lt 0) { throw 'No outgoing write for control' }
    $parts=$changed[$writeIndex].Split(' ')
    $hex=$parts[$parts.Length-1]
    $flipped=([Convert]::ToInt32($hex.Substring(0,2),16) -bxor 1).ToString('x2')+$hex.Substring(2)
    $parts[$parts.Length-1]=$flipped
    $changed[$writeIndex]=$parts -join ' '
    [IO.File]::WriteAllLines("$out/controls/outgoing-byte.rec",$changed,$utf8)
    $cut=[int]($lines.Length*0.6)
    [IO.File]::WriteAllLines("$out/controls/truncated.rec",[string[]]$lines[0..$cut],$utf8)
    $correctPassword=$env:RS2_PASSWORD
    foreach($control in @('outgoing-byte','truncated','wrong-password')) {
        $log="$out/controls/$control.rec"
        $env:RS2_PASSWORD=$correctPassword
        if($control -eq 'wrong-password') {
            $log="$out/record/session.rec"
            $last=$correctPassword.Substring($correctPassword.Length-1)
            $suffix=if($last -eq 'x') { 'y' } else { 'x' }
            $env:RS2_PASSWORD=$correctPassword.Substring(0,$correctPassword.Length-1)+$suffix
        }
        New-Item -ItemType Directory -Path "$out/controls/$control" | Out-Null
        & $OptInReplay $log --allow-frame-differences --out "$out/controls/$control" *> "$out/controls/$control/replay.log"
        $controlExit=$LASTEXITCODE
        Set-Content "$out/controls/$control/replay.exit" $controlExit
        if($controlExit -ne 1) { throw "Control $control failed to reject bad input" }
    }
    $env:RS2_PASSWORD=$correctPassword
    Get-Content "$out/record-driver.log","$out/parity/java/java-report.txt"
    Get-Content "$out/parity/c/replay-state.log" -Tail 1
    Get-Content "$out/opt-in-replay/replay.log" -Tail 2
    "Full Java frame comparison exit: $parityExit"
} finally {
    $env:RS2_USERNAME=$oldUser; $env:RS2_PASSWORD=$oldPassword
    [Array]::Clear($bytes,0,$bytes.Length)
    $changes=@()
    foreach($save in $before) {
        if(-not (Test-Path -LiteralPath $save.Path) -or (Get-FileHash -LiteralPath $save.Path).Hash -ne $save.Hash) {
            $changes+=$save.Path
        }
    }
    [pscustomobject]@{OriginalSaves=$before.Count;ChangedOriginalSaves=$changes;
        NewSaveExists=(Test-Path -LiteralPath "$players/main/$Account.sav");PasswordWritten=$false} |
        ConvertTo-Json | Set-Content "$out/saves-result.json"
}
exit $parityExit
