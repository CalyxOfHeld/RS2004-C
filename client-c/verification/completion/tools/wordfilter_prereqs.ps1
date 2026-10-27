param([Parameter(Mandatory=$true)][string]$OutDir,[string]$TestExecutable='', [string]$BranchExecutable='')
# Compatibility entry point; prerequisites are included in complete parity.
& "$PSScriptRoot/wordfilter_parity.ps1" -OutDir $OutDir -TestExecutable $TestExecutable -BranchExecutable $BranchExecutable
