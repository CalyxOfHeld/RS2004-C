param(
 [Parameter(Mandatory=$true)][string]$OutDir,
 [string]$TestExecutable=''
)
# Invoke the unchanged revision225 original renderer; run the existing native
# chatbox controls against all pixels and hover entries. Build native first.
$ErrorActionPreference='Continue'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to retain earlier evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_chatbox.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/ChatDisplayOracle.java" *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original chat oracle compilation failed.'}
& java '-Djava.awt.headless=false' -cp "$jar;$OutDir/classes" ChatDisplayOracle "$root/engine/data/pack/client" "$OutDir/original.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original chat rendering/hover capture failed.'}
$savedOracle=$env:RS2_CHAT_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_CHAT_ORACLE="$OutDir/original.bin"
 & $TestExecutable *> "$OutDir/native.log"
 $LASTEXITCODE | Set-Content "$OutDir/native.exit"
 if($LASTEXITCODE -ne 0){throw 'Native chat pixels or hover entries differ from original.'}
 if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch -Pattern '144 complete chatback/split images,31246848 pixels,21456 hover queries exact')){throw 'Missing full original comparison evidence; rebuild the current test_chatbox.'}
 Get-FileHash -Algorithm SHA256 $TestExecutable,$jar,"$OutDir/original.bin" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
 'PASS:144 complete original chatback/split images;31246848 pixels;21456 hover queries. Controlled display fixtures, not full messaging transport or social-action execution.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
} finally {
 $env:RS2_CHAT_ORACLE=$savedOracle
 Pop-Location
}
exit 0
