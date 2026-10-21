@echo off
setlocal
cd /d "%~dp0client-c"
set "PATH=C:\devkitPro\msys2\opt\bin;C:\devkitPro\msys2\usr\bin;C:\devkitPro\msys2\mingw64\bin;%PATH%"

echo ========================================================
echo   Running 2004scape C Rewrite Automated Test Suite
echo ========================================================
make test

pause
