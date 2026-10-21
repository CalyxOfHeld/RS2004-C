@echo off
setlocal
where /q powershell.exe
if errorlevel 1 (
    echo Windows PowerShell was not found.
    pause
    exit /b 1
)
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0play_client.ps1" %*
set "result=%errorlevel%"
if not "%result%"=="0" pause
exit /b %result%
