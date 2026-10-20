@echo off
setlocal
cd /d "%~dp0client-c"
bin\rs2client.exe %*
if errorlevel 1 pause
