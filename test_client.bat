@echo off
setlocal
cd /d "%~dp0client-c"
set "PATH=C:\devkitPro\msys2\opt\bin;C:\devkitPro\msys2\usr\bin;C:\devkitPro\msys2\mingw64\bin;%PATH%"
set "TMP=%CD%\verification\build-temp"
set "TEMP=%TMP%"
if not exist "%TMP%" mkdir "%TMP%"
echo Compiling the native revision-225 client...
make bin/rs2client.exe
if errorlevel 1 (
    echo Compilation failed. Review the error above.
    pause
    exit /b 1
)
call "%~dp0play_client.bat" %*
exit /b %errorlevel%
