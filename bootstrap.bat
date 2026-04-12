@echo off
setlocal enabledelayedexpansion

echo Bootstrapping loom on Windows...

if not exist build\cli mkdir build\cli
if not exist build\lib mkdir build\lib
set CC=
where clang >nul 2>&1
if %ERRORLEVEL% equ 0 set CC=clang

if "%CC%"=="" (
    where gcc >nul 2>&1
    if %ERRORLEVEL% equ 0 set CC=gcc
)

if "%CC%"=="" (
    where cl >nul 2>&1
    if %ERRORLEVEL% equ 0 set CC=cl
)

if "%CC%"=="" (
    echo Error: No C compiler found in PATH ^(gcc, clang, cl^).
    exit /b 1
)

echo Using compiler: %CC%

set CFLAGS=-std=c11 -Wall -Wextra -O2 -Iinclude

if "%CC%"=="cl" (
    set CFLAGS=/std:c11 /W3 /O2 /Iinclude /D_CRT_SECURE_NO_WARNINGS
    
    echo   CC   src\main.c src\init.c src\path.c src\exec.c
    %CC% %CFLAGS% /c src\main.c /Fobuild\cli\main.o
    %CC% %CFLAGS% /c src\init.c /Fobuild\cli\init.o
    %CC% %CFLAGS% /c src\path.c /Fobuild\cli\path.o
    %CC% %CFLAGS% /c src\exec.c /Fobuild\cli\exec.o
    
    echo   LINK build\loom.exe
    %CC% %CFLAGS% /Febuild\loom.exe build\cli\main.o build\cli\init.o build\cli\path.o build\cli\exec.o

    echo   CC   src\loom.c src\runner.c src\toolchain.c src\hash.c src\cache.c src\exec.c src\path.c
    %CC% %CFLAGS% /c src\loom.c /Fobuild\lib\loom.o
    %CC% %CFLAGS% /c src\runner.c /Fobuild\lib\runner.o
    %CC% %CFLAGS% /c src\toolchain.c /Fobuild\lib\toolchain.o
    %CC% %CFLAGS% /c src\hash.c /Fobuild\lib\hash.o
    %CC% %CFLAGS% /c src\cache.c /Fobuild\lib\cache.o
    %CC% %CFLAGS% /c src\exec.c /Fobuild\lib\exec.o
    %CC% %CFLAGS% /c src\path.c /Fobuild\lib\path.o

    echo   AR   build\libloom.lib
    lib /OUT:build\libloom.lib build\lib\loom.o build\lib\runner.o build\lib\toolchain.o build\lib\hash.o build\lib\cache.o build\lib\exec.o build\lib\path.o
) else (
    echo   CC   src\main.c src\init.c src\path.c src\exec.c
    %CC% %CFLAGS% -c src\main.c -o build\cli\main.o
    %CC% %CFLAGS% -c src\init.c -o build\cli\init.o
    %CC% %CFLAGS% -c src\path.c -o build\cli\path.o
    %CC% %CFLAGS% -c src\exec.c -o build\cli\exec.o
    
    echo   LINK build\loom.exe
    %CC% -o build\loom.exe build\cli\main.o build\cli\init.o build\cli\path.o build\cli\exec.o

    echo   CC   src\loom.c src\runner.c src\toolchain.c src\hash.c src\cache.c src\exec.c src\path.c
    %CC% %CFLAGS% -c src\loom.c -o build\lib\loom.o
    %CC% %CFLAGS% -c src\runner.c -o build\lib\runner.o
    %CC% %CFLAGS% -c src\toolchain.c -o build\lib\toolchain.o
    %CC% %CFLAGS% -c src\hash.c -o build\lib\hash.o
    %CC% %CFLAGS% -c src\cache.c -o build\lib\cache.o
    %CC% %CFLAGS% -c src\exec.c -o build\lib\exec.o
    %CC% %CFLAGS% -c src\path.c -o build\lib\path.o

    echo   AR   build\libloom.a
    ar rcs build\libloom.a build\lib\loom.o build\lib\runner.o build\lib\toolchain.o build\lib\hash.o build\lib\cache.o build\lib\exec.o build\lib\path.o
)

echo.
echo Build complete.
echo   loom binary  : build\loom.exe
echo   loom library : build\libloom.lib or build\libloom.a
echo.
echo After installation loom can rebuild itself:
echo   build\loom.exe build
