@echo off
setlocal
cd /d "%~dp0engine"
set "NODE_PORT=43595"
set "WEB_PORT=8889"
set "WEB_MANAGEMENT_PORT=8899"
rem Normal play honors the client's idle logout request.
set "NODE_DEBUG=false"
where /q powershell.exe
if errorlevel 1 (
    echo Windows PowerShell is required to check the local server ports.
    pause
    exit /b 1
)
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0check_local_server.ps1"
set "portCheck=%errorlevel%"
if "%portCheck%"=="2" exit /b 0
if not "%portCheck%"=="0" (
    pause
    exit /b 1
)
where /q bun
if errorlevel 1 (
    echo Bun is required to run the existing local server. Install/configure Bun, then retry.
    pause
    exit /b 1
)
echo Starting this project's revision-225 server on port 43595.
echo Automatic idle logout is enabled. Combat can delay logout.
echo Keep this window open. Wait for World ready before launching the client.
echo Use Ctrl+C here to request a normal server shutdown.
bun run src/app.ts
set "result=%errorlevel%"
if not "%result%"=="0" (
    echo Server exited with error code %result%. Review the error above.
    pause
)
exit /b %result%
