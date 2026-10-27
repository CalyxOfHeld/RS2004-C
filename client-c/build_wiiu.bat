@echo off
setlocal
echo =======================================================
echo Building 2004scape client for Nintendo Wii U (.wuhb)
echo =======================================================

set "DEVKITPRO=c:/devkitPro"
set "DEVKITPPC=%DEVKITPRO%/devkitPPC"

"%DEVKITPRO%\msys2\usr\bin\bash.exe" -lc "export DEVKITPRO=/c/devkitPro; export DEVKITPPC=/c/devkitPro/devkitPPC; cd '/c/Users/USER/Documents/2004scape-LostCityRS/2004scape C Rewrite/client-c'; make -f Makefile.wiiu all"

if %ERRORLEVEL% equ 0 (
    echo.
    echo =======================================================
    echo [SUCCESS] Built bin/rs2client.wuhb!
    echo To install on your Wii U:
    echo 1. Copy bin/rs2client.wuhb to sd:/wiiu/apps/rs2client/
    echo 2. Launch via Aroma or Homebrew Launcher
    echo =======================================================
) else (
    echo.
    echo [ERROR] Wii U build failed with error code %ERRORLEVEL%.
)
endlocal
