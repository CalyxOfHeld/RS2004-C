@echo off
setlocal
rem Builds the Wii U client: client-c\bin\wiiu\rs2004c.wuhb
cd /d "%~dp0"
"C:\devkitPro\msys2\usr\bin\bash.exe" -lc "export DEVKITPRO=/c/devkitPro DEVKITPPC=/c/devkitPro/devkitPPC; cd \"$(cygpath -u '%~dp0')\" && make -f Makefile.wiiu all"
if errorlevel 1 (
    echo Wii U build failed.
    exit /b 1
)
echo.
echo Built bin\wiiu\rs2004c.wuhb
echo Cemu: File ^> Load, choose the .wuhb
echo Wii U: copy it to sd:\wiiu\apps\rs2004c\ and put the server address in
echo        sd:\wiiu\apps\rs2004c\server.txt, e.g. "192.168.1.20 43595"
endlocal
