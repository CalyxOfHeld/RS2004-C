param([string]$OutputDirectory = (Join-Path $PSScriptRoot '../verification'))
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$clientRoot = Split-Path $PSScriptRoot -Parent
$binary = Join-Path $clientRoot 'bin/rs2client.exe'
$output = [IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $output | Out-Null
$views = @(
    @{ Name='after-default'; Args=@() },
    @{ Name='after-east'; Args=@('--camera-yaw','512','--camera-pitch','256') },
    @{ Name='after-south'; Args=@('--camera-yaw','1024','--camera-pitch','383') },
    @{ Name='after-west'; Args=@('--camera-yaw','1536') },
    @{ Name='after-rebuild'; Args=@('--rebuild') },
    @{ Name='after-walk'; Args=@('--walk-to','33','32','--capture-frame','35') }
)
$results = @()
$sceneHashes = @{}
foreach ($view in $views) {
    $path = Join-Path $output ($view.Name + '.bmp')
    $log = Join-Path $output ($view.Name + '.log')
    $arguments = @('--offline','--capture',$path) + $view.Args
    & $binary @arguments *> $log
    if ($LASTEXITCODE -ne 0) { throw "Client failed for $($view.Name); see $log" }
    $terrain = [Drawing.Bitmap]::new($path + '.terrain.bmp')
    $client = [Drawing.Bitmap]::new($path)
    try {
        $grass = 0; $road = 0
        for ($y = 11; $y -lt 345; $y++) {
            for ($x = 8; $x -lt 520; $x++) {
                $c = $terrain.GetPixel($x,$y)
                if ($c.G -gt $c.R + 10 -and $c.G -gt $c.B + 10) { $grass++ }
                if ($c.R -gt 40 -and [Math]::Abs([int]$c.R - $c.G) -lt 15 -and [Math]::Abs([int]$c.G - $c.B) -lt 15) { $road++ }
            }
        }
        if ($grass -lt 1000 -or $road -lt 100) { throw "Grass/paths missing for $($view.Name): $grass/$road" }
        for ($y = 0; $y -lt $terrain.Height; $y++) {
            for ($x = 0; $x -lt $terrain.Width; $x++) {
                if ($x -ge 8 -and $x -lt 520 -and $y -ge 11 -and $y -lt 345) { continue }
                $c = $terrain.GetPixel($x,$y)
                if ($c.R -ne 0 -or $c.G -ne 0 -or $c.B -ne 0) { throw "Terrain escaped viewport at $x,$y" }
            }
        }
        if ($client.GetPixel(100,400).ToArgb() -eq [Drawing.Color]::Black.ToArgb()) { throw 'Chatbox missing' }
        if ($client.GetPixel(600,300).ToArgb() -eq [Drawing.Color]::Black.ToArgb()) { throw 'Sidebar missing' }
        $client.Save((Join-Path $output ($view.Name + '.png')), [Drawing.Imaging.ImageFormat]::Png)
        $terrain.Save((Join-Path $output ($view.Name + '.terrain.png')), [Drawing.Imaging.ImageFormat]::Png)
        $sceneHashes[$view.Name] = (Get-FileHash ($path + '.terrain.bmp')).Hash
        $results += [pscustomobject]@{ View=$view.Name; Grass=$grass; Road=$road; Exit=0 }
    } finally { $terrain.Dispose(); $client.Dispose() }
    # Retain lossless PNG evidence; discard this run's large temporary BMPs.
    Remove-Item -LiteralPath $path, ($path + '.terrain.bmp')
}
$initial = $sceneHashes['after-default']
$rebuilt = $sceneHashes['after-rebuild']
if ($initial -ne $rebuilt) { throw 'Rebuilding the same scene changed its rendered terrain' }
if (!(Select-String -Path (Join-Path $output 'after-walk.log') -SimpleMatch 'player tile=(33,32)')) { throw 'Player did not walk to target' }
$results | Format-Table | Out-String | Set-Content (Join-Path $output 'client-verification.txt')
$results | Format-Table
Write-Output 'PASS: SDL client frames, four camera views, movement, UI, viewport clipping, and identical scene rebuild.'
