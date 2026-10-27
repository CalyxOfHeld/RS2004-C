param(
 [Parameter(Mandatory=$true)][string]$OutDir,
 [string]$TestExecutable=''
)
# Build the current native test_packet first. This wrapper regenerates original
# expectations from the unchanged revision225 jar in a fresh evidence directory.
$ErrorActionPreference='Continue'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain prior evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_packet.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/WordPackOracle.java" *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original WordPack oracle compilation failed.'}
& java -cp "$jar;$OutDir/classes" WordPackOracle "$OutDir/reference.bin" "$root/engine/data/pack/client/wordenc" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original WordPack fixture generation failed.'}
$generated=(Get-FileHash -Algorithm SHA256 "$OutDir/reference.bin").Hash
$bundled=(Get-FileHash -Algorithm SHA256 "$root/client-c/tests/fixtures/wordpack-reference.bin").Hash
if($generated -ne $bundled){throw 'Regenerated original corpus differs from the bundled expected fixture.'}
$savedOracle=$env:RS2_WORDPACK_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_WORDPACK_ORACLE="$OutDir/reference.bin"
 & $TestExecutable *> "$OutDir/native.log"
 $LASTEXITCODE | Set-Content "$OutDir/native.exit"
 if($LASTEXITCODE -ne 0){throw 'Native WordPack comparison failed.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch -Pattern 'PASS WordPack original: 140722 pack and 71405 raw decode cases; 2178 original overflows safely rejected')){throw 'Missing full original comparison marker; rebuild current test_packet.'}
 Get-FileHash -Algorithm SHA256 $TestExecutable,$jar,"$OutDir/reference.bin","$root/engine/data/pack/client/wordenc" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
 'PASS:140722 original pack cases;71405 raw decode cases;2178 original overflows safely rejected;English Latin-1 contract;regenerated fixture identity exact.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
} finally {
 $env:RS2_WORDPACK_ORACLE=$savedOracle
 Pop-Location
}
exit 0
