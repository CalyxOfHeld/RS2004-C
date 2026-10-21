param([int[]] $Ports = @(43595, 8889, 8899))

# Exit 0: free to start; 2: all configured ports already share one server;
# 1: partial/conflicting listeners, or the check could not be completed.
try {
    $listeners = @(Get-NetTCPConnection -State Listen -ErrorAction Stop |
        Where-Object { $Ports -contains $_.LocalPort })
} catch {
    Write-Host 'Could not check local server ports. No additional server was started.'
    Write-Host $_.Exception.Message
    exit 1
}

if ($listeners.Count -eq 0) { exit 0 }

$occupiedPorts = @($listeners.LocalPort | Sort-Object -Unique)
$owners = @($listeners.OwningProcess | Sort-Object -Unique)
foreach ($listener in ($listeners | Sort-Object LocalPort, OwningProcess -Unique)) {
    Write-Host "Port $($listener.LocalPort) is already listening in process $($listener.OwningProcess)."
}

if ($occupiedPorts.Count -eq $Ports.Count -and $owners.Count -eq 1) {
    Write-Host 'All configured server ports are already in use by the same process. No second server was started.'
    Write-Host 'If the existing server window says World ready, use play_client.bat or test_client.bat.'
    exit 2
}

Write-Host 'Server startup stopped because some configured ports are in use. Check the existing server windows.'
exit 1
