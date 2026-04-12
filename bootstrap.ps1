param (
    [Parameter(Position=0)]
    [string]$Command = "build",
    [string]$Prefix = "$env:ProgramFiles\loom"
)

$targetPath = $Prefix
$safePrefix = $Prefix -replace "\\", "/"
$BinDir = Join-Path $targetPath "bin"
$IncludeDir = Join-Path $targetPath "include"
$LibDir = Join-Path $targetPath "lib"

if ($Command -eq "build") {
    Write-Host "Bootstrapping loom on Windows..."

    if (-not (Test-Path "build\cli")) { New-Item -ItemType Directory -Force -Path "build\cli" | Out-Null }
    if (-not (Test-Path "build\lib")) { New-Item -ItemType Directory -Force -Path "build\lib" | Out-Null }

    $CC = $null
    if (Get-Command clang -ErrorAction SilentlyContinue) {
        $CC = "clang"
    } elseif (Get-Command gcc -ErrorAction SilentlyContinue) {
        $CC = "gcc"
    } elseif (Get-Command cl -ErrorAction SilentlyContinue) {
        $CC = "cl"
    }

    if (-not $CC) {
        Write-Host "Error: No C compiler found in PATH (gcc, clang, cl)."
        exit 1
    }

    Write-Host "Using compiler: $CC"

    if ($CC -eq "cl") {
        $CLFlags = @("/std:c11", "/W3", "/O2", "/Iinclude", "/D_CRT_SECURE_NO_WARNINGS", "/DLOOM_PREFIX=`"$safePrefix`"")
        
        Write-Host "  CC   src\main.c src\init.c src\path.c src\exec.c"
        & cl $CLFlags /c src\main.c /Fobuild\cli\main.o
        & cl $CLFlags /c src\init.c /Fobuild\cli\init.o
        & cl $CLFlags /c src\path.c /Fobuild\cli\path.o
        & cl $CLFlags /c src\exec.c /Fobuild\cli\exec.o
        
        Write-Host "  LINK build\loom.exe"
        & cl $CLFlags /Febuild\loom.exe build\cli\main.o build\cli\init.o build\cli\path.o build\cli\exec.o

        Write-Host "  CC   src\loom.c src\runner.c src\toolchain.c src\hash.c src\cache.c src\exec.c src\path.c"
        & cl $CLFlags /c src\loom.c /Fobuild\lib\loom.o
        & cl $CLFlags /c src\runner.c /Fobuild\lib\runner.o
        & cl $CLFlags /c src\toolchain.c /Fobuild\lib\toolchain.o
        & cl $CLFlags /c src\hash.c /Fobuild\lib\hash.o
        & cl $CLFlags /c src\cache.c /Fobuild\lib\cache.o
        & cl $CLFlags /c src\exec.c /Fobuild\lib\exec.o
        & cl $CLFlags /c src\path.c /Fobuild\lib\path.o

        Write-Host "  AR   build\libloom.lib"
        & lib /OUT:build\libloom.lib build\lib\loom.o build\lib\runner.o build\lib\toolchain.o build\lib\hash.o build\lib\cache.o build\lib\exec.o build\lib\path.o
    } else {
        $CFLAGS = @("-std=c11", "-Wall", "-Wextra", "-O2", "-Iinclude", "-DLOOM_PREFIX=`"$safePrefix`"")

        Write-Host "  CC   src\main.c src\init.c src\path.c src\exec.c"
        & $CC $CFLAGS -c src\main.c -o build\cli\main.o
        & $CC $CFLAGS -c src\init.c -o build\cli\init.o
        & $CC $CFLAGS -c src\path.c -o build\cli\path.o
        & $CC $CFLAGS -c src\exec.c -o build\cli\exec.o
        
        Write-Host "  LINK build\loom.exe"
        & $CC -o build\loom.exe build\cli\main.o build\cli\init.o build\cli\path.o build\cli\exec.o

        Write-Host "  CC   src\loom.c src\runner.c src\toolchain.c src\hash.c src\cache.c src\exec.c src\path.c"
        & $CC $CFLAGS -c src\loom.c -o build\lib\loom.o
        & $CC $CFLAGS -c src\runner.c -o build\lib\runner.o
        & $CC $CFLAGS -c src\toolchain.c -o build\lib\toolchain.o
        & $CC $CFLAGS -c src\hash.c -o build\lib\hash.o
        & $CC $CFLAGS -c src\cache.c -o build\lib\cache.o
        & $CC $CFLAGS -c src\exec.c -o build\lib\exec.o
        & $CC $CFLAGS -c src\path.c -o build\lib\path.o

        Write-Host "  AR   build\libloom.a"
        & ar rcs build\libloom.a build\lib\loom.o build\lib\runner.o build\lib\toolchain.o build\lib\hash.o build\lib\cache.o build\lib\exec.o build\lib\path.o
    }

    Write-Host "`nBuild complete."
    Write-Host "  loom binary  : build\loom.exe"
    Write-Host "  loom library : build\libloom.lib or build\libloom.a`n"
    Write-Host "To install loom system-wide, run:"
    Write-Host "  .\bootstrap.ps1 install"
} elseif ($Command -eq "install") {
    if (-not (Test-Path "build\loom.exe")) {
        Write-Host "Binaries not found. Running build first..."
        & .\bootstrap.ps1 build -Prefix $Prefix
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }

    Write-Host "Installing to $targetPath ..."

    try {
        if (-not (Test-Path $BinDir)) { New-Item -ItemType Directory -Force -Path $BinDir -ErrorAction Stop | Out-Null }
        if (-not (Test-Path $IncludeDir)) { New-Item -ItemType Directory -Force -Path $IncludeDir -ErrorAction Stop | Out-Null }
        if (-not (Test-Path $LibDir)) { New-Item -ItemType Directory -Force -Path $LibDir -ErrorAction Stop | Out-Null }

        Write-Host "  INSTALL $BinDir\loom.exe"
        Copy-Item -Path "build\loom.exe" -Destination (Join-Path $BinDir "loom.exe") -Force -ErrorAction Stop

        Write-Host "  INSTALL $IncludeDir\loom.h"
        Copy-Item -Path "include\loom.h" -Destination (Join-Path $IncludeDir "loom.h") -Force -ErrorAction Stop

        if (Test-Path "build\libloom.a") {
            Write-Host "  INSTALL $LibDir\libloom.a"
            Copy-Item -Path "build\libloom.a" -Destination (Join-Path $LibDir "libloom.a") -Force -ErrorAction Stop
        } elseif (Test-Path "build\libloom.lib") {
            Write-Host "  INSTALL $LibDir\libloom.lib"
            Copy-Item -Path "build\libloom.lib" -Destination (Join-Path $LibDir "libloom.lib") -Force -ErrorAction Stop
        }

        Write-Host "Installation completed successfully."
    } catch [System.UnauthorizedAccessException] {
        Write-Host "`nError: Permiso denegado. Abre PowerShell como Administrador para instalar en $targetPath"
        exit 1
    } catch {
        Write-Host "`nError: $_"
        exit 1
    }
} elseif ($Command -eq "uninstall") {
    Write-Host "Uninstalling from $targetPath ..."
    
    $loomExe = Join-Path $BinDir "loom.exe"
    $loomHeader = Join-Path $IncludeDir "loom.h"
    $loomA = Join-Path $LibDir "libloom.a"
    $loomLib = Join-Path $LibDir "libloom.lib"

    try {
        if (Test-Path $loomExe) { Write-Host "  RM $loomExe"; Remove-Item -Force $loomExe -ErrorAction Stop }
        if (Test-Path $loomHeader) { Write-Host "  RM $loomHeader"; Remove-Item -Force $loomHeader -ErrorAction Stop }
        if (Test-Path $loomA) { Write-Host "  RM $loomA"; Remove-Item -Force $loomA -ErrorAction Stop }
        if (Test-Path $loomLib) { Write-Host "  RM $loomLib"; Remove-Item -Force $loomLib -ErrorAction Stop }

        Write-Host "Uninstallation completed successfully."
    } catch [System.UnauthorizedAccessException] {
        Write-Host "`nError: Permiso denegado. Abre PowerShell como Administrador para desinstalar de $targetPath"
        exit 1
    } catch {
        Write-Host "`nError: $_"
        exit 1
    }
} else {
    Write-Host "Unknown command: $Command. Valid commands are 'build', 'install', 'uninstall'."
}
