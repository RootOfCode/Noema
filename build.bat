@echo off
setlocal EnableExtensions EnableDelayedExpansion

if /I "%~1"=="clean" goto :clean

set "SOURCES=src\main.c src\lexico.c src\preprocessador.c src\parser.c src\runtime.c src\builtins.c src\ffi.c"

if not exist obj mkdir obj
if not exist build mkdir build

where cl >nul 2>nul
if not errorlevel 1 goto :build_msvc

where gcc >nul 2>nul
if not errorlevel 1 (
    set "CC=gcc"
    goto :build_gnu
)

where clang >nul 2>nul
if not errorlevel 1 (
    set "CC=clang"
    goto :build_gnu
)

echo error: no supported compiler found in PATH.
echo Install Visual Studio Build Tools, MinGW-w64 or LLVM/Clang for Windows.
exit /b 1

:build_msvc
echo ==^> building Noema with MSVC
set "CFLAGS=/nologo /std:c11 /W4 /O2 /D_CRT_SECURE_NO_WARNINGS"
if defined LIBFFI_INCLUDE set CFLAGS=!CFLAGS! /I"%LIBFFI_INCLUDE%"
set "OBJECTS="

for %%F in (%SOURCES%) do (
    set "OBJ=obj\%%~nF.obj"
    cl !CFLAGS! /c %%F /Fo!OBJ!
    if errorlevel 1 exit /b 1
    set "OBJECTS=!OBJECTS! !OBJ!"
)

set "FFI_LIB=ffi.lib"
if defined LIBFFI_LIB set "FFI_LIB=%LIBFFI_LIB%\ffi.lib"

link /nologo /out:build\noema.exe !OBJECTS! kernel32.lib "!FFI_LIB!"
if errorlevel 1 exit /b 1
goto :success

:build_gnu
echo ==^> building Noema with %CC%
set "CFLAGS=-std=c11 -Wall -Wextra -Wpedantic -O2"
if defined LIBFFI_INCLUDE set CFLAGS=!CFLAGS! -I"%LIBFFI_INCLUDE%"
set "LIBS=-lffi -lkernel32"
if defined LIBFFI_LIB set LIBS=-L"%LIBFFI_LIB%" !LIBS!
set "OBJECTS="

for %%F in (%SOURCES%) do (
    set "OBJ=obj\%%~nF.o"
    "%CC%" !CFLAGS! -c %%F -o !OBJ!
    if errorlevel 1 exit /b 1
    set "OBJECTS=!OBJECTS! !OBJ!"
)

"%CC%" !CFLAGS! -o build\noema.exe !OBJECTS! !LIBS!
if errorlevel 1 exit /b 1
goto :success

:clean
if exist obj rmdir /S /Q obj
if exist build rmdir /S /Q build
echo cleaned
exit /b 0

:success
echo ==^> build complete
echo binary: build\noema.exe
echo.
echo Optional variables:
echo   LIBFFI_INCLUDE=C:\path\to\libffi\include
echo   LIBFFI_LIB=C:\path\to\libffi\lib
exit /b 0
