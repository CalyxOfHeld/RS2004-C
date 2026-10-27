param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$BranchExecutable='')
# Complete UTF-16 English/root filtering parity; gameplay messaging is separate.
$ErrorActionPreference='Stop'
trap {Write-Error $_;exit 1}
$root=(Resolve-Path "$PSScriptRoot/../../../..").Path
if(Test-Path -LiteralPath $OutDir){throw 'Use a fresh output directory to preserve evidence.'}
if(-not $TestExecutable){$TestExecutable="$root/client-c/bin/test_jagfile.exe"}
$TestExecutable=(Resolve-Path -LiteralPath $TestExecutable).Path
if(-not $BranchExecutable){$BranchExecutable="$root/client-c/bin/wordfilter_branch_control.exe"}
$BranchExecutable=(Resolve-Path -LiteralPath $BranchExecutable).Path
New-Item -ItemType Directory -Force "$OutDir/classes" | Out-Null
$OutDir=(Resolve-Path -LiteralPath $OutDir).Path
$jar="$root/javaclient/build/libs/rs2client.jar"
& javac -encoding UTF-8 -cp $jar -d "$OutDir/classes" "$root/client-c/verification/completion/reference/src/WordFilterOracle.java" "$root/client-c/verification/completion/reference/src/WordFilterCases.java" *> "$OutDir/build.log"
$LASTEXITCODE | Set-Content "$OutDir/build.exit"
if($LASTEXITCODE -ne 0){throw 'Original WordFilter oracle compilation failed.'}
& java -cp "$jar;$OutDir/classes" WordFilterOracle "$root/engine/data/pack/client/wordenc" "$OutDir/tables.bin" "$OutDir/full-filter-reference.bin" "$OutDir/helpers.bin" "$OutDir/branches.bin" *> "$OutDir/oracle.log"
$LASTEXITCODE | Set-Content "$OutDir/oracle.exit"
if($LASTEXITCODE -ne 0){throw 'Original WordFilter expectations failed.'}
foreach($pair in @(@('tables.bin','wordfilter-tables-reference.bin'),@('helpers.bin','wordfilter-helpers-reference.bin'),@('full-filter-reference.bin','wordfilter-reference.bin'))){
 if((Get-FileHash "$OutDir/$($pair[0])").Hash -ne (Get-FileHash "$root/client-c/tests/fixtures/$($pair[1])").Hash){throw 'Regenerated original fixture differs from bundled expectations.'}
}
$savedFull=$env:RS2_WORDFILTER_FULL_ORACLE
$savedTables=$env:RS2_WORDFILTER_TABLES_ORACLE;$savedHelpers=$env:RS2_WORDFILTER_HELPERS_ORACLE
Push-Location "$root/client-c"
try {
 $env:RS2_WORDFILTER_FULL_ORACLE="$OutDir/full-filter-reference.bin"
 $env:RS2_WORDFILTER_TABLES_ORACLE="$OutDir/tables.bin";$env:RS2_WORDFILTER_HELPERS_ORACLE="$OutDir/helpers.bin"
 & $TestExecutable *> "$OutDir/native.log"
 $LASTEXITCODE | Set-Content "$OutDir/native.exit"
 if($LASTEXITCODE -ne 0){throw 'Native WordFilter complete comparison failed.'}
 foreach($marker in @('4311 fragments,336 bad,452 domains,258 tlds','17248 truncated prefixes rejected transactionally','67544 cases/all65536 code units','160976')){
  if(-not (Select-String -LiteralPath "$OutDir/native.log" -SimpleMatch -Pattern $marker)){throw 'Missing complete coverage marker; rebuild current test_jagfile.'}
 }
 & $BranchExecutable "$OutDir/branches.bin" "$OutDir/full-filter-reference.bin" *> "$OutDir/branches.log"
 $LASTEXITCODE | Set-Content "$OutDir/branches.exit"
 if($LASTEXITCODE -ne 0){throw 'Native branch comparison failed.'}
 foreach($marker in @('loader allocation failures=1136','source branch controls total=8540625','forward-backward-in-place alias safety')){
  if(-not (Select-String -LiteralPath "$OutDir/branches.log" -SimpleMatch -Pattern $marker)){throw 'Missing current branch/safety marker.'}
 }
 'PASS complete UTF-16 English/root filter:160976 original outputs,8540625 branch records,67544 helpers,17248 truncations,1136 loader allocation failures. Filtering library only; gameplay messaging separate.' | Set-Content -Encoding UTF8 "$OutDir/result.txt"
 Get-FileHash $TestExecutable,$BranchExecutable,$PSCommandPath,"$root/client-c/src/core/wordfilter.c","$root/client-c/include/core/wordfilter.h","$root/client-c/verification/completion/tools/wordfilter_branch_control.c","$root/client-c/verification/completion/reference/src/WordFilterCases.java","$root/client-c/verification/completion/reference/src/WordFilterOracle.java",$jar,"$root/engine/data/pack/client/wordenc","$OutDir/tables.bin","$OutDir/helpers.bin","$OutDir/full-filter-reference.bin","$OutDir/branches.bin" | Select-Object Path,Hash | ConvertTo-Json | Set-Content -Encoding UTF8 "$OutDir/identities.json"
} finally {$env:RS2_WORDFILTER_FULL_ORACLE=$savedFull;$env:RS2_WORDFILTER_TABLES_ORACLE=$savedTables;$env:RS2_WORDFILTER_HELPERS_ORACLE=$savedHelpers;Pop-Location}
exit 0
