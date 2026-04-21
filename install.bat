@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "PREFIX=%~1"
if not defined PREFIX if defined NOEMA_PREFIX set "PREFIX=%NOEMA_PREFIX%"
if not defined PREFIX set "PREFIX=%LOCALAPPDATA%\Programs\Noema"

set "BIN_DIR=%PREFIX%\bin"
set "SHARE_DIR=%PREFIX%\share\noema"
set "STDLIB_DIR=%SHARE_DIR%\stdlib"

where mingw32-make >nul 2>nul
if not errorlevel 1 (
    set "MAKE=mingw32-make"
    goto :build
)

where make >nul 2>nul
if not errorlevel 1 (
    set "MAKE=make"
    goto :build
)

echo error: make tool not found.
echo Install MinGW and ensure mingw32-make.exe or make.exe is in PATH.
exit /b 1

:build
echo ==^> building Noema with %MAKE%
%MAKE%
if errorlevel 1 exit /b 1

if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"
if not exist "%SHARE_DIR%" mkdir "%SHARE_DIR%"
if exist "%STDLIB_DIR%" rmdir /S /Q "%STDLIB_DIR%"
mkdir "%STDLIB_DIR%"

echo ==^> installing compiler to %BIN_DIR%
copy /Y "build\noema.exe" "%BIN_DIR%\noema.exe" >nul
if errorlevel 1 exit /b 1

echo ==^> installing stdlib to %STDLIB_DIR%
xcopy /E /I /Y "stdlib\*" "%STDLIB_DIR%\" >nul
if errorlevel 1 exit /b 1

echo ==^> done
echo compiler: %BIN_DIR%\noema.exe
echo stdlib:   %STDLIB_DIR%
echo plugins are not installed; they remain user-managed.
echo.
echo If you want to use noema from any terminal, add this to PATH:
echo   %BIN_DIR%
exit /b 0
