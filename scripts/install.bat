@echo off
REM install.bat — the ONLY thing you run.
REM If g++ isn't found, this downloads and silently installs MSYS2 +
REM MinGW-w64 gcc automatically (needs internet, takes a few minutes the
REM first time). Then compiles CnR.cpp and install.cpp, installs CnR.exe,
REM and adds it to your PATH.
setlocal enabledelayedexpansion

echo CnR installer (auto-setup + build + install)
echo ==============================================
echo.

set "HERE=%~dp0"
cd /d "%HERE%"

REM ============================================================
REM STEP 0: Make sure g++ is available. If not, install MSYS2.
REM ============================================================
where g++ >nul 2>nul
if not errorlevel 1 goto :gcc_ready

echo g++ not found. Setting up a compiler automatically (MSYS2 + MinGW-w64)...
echo This only happens once and needs an internet connection.
echo.

set "MSYS2_INSTALL_DIR=C:\msys64"
set "MSYS2_INSTALLER=%TEMP%\msys2-installer.exe"
set "MSYS2_URL=https://github.com/msys2/msys2-installer/releases/latest/download/msys2-base-x86_64.exe"

if exist "%MSYS2_INSTALL_DIR%\mingw64\bin\g++.exe" (
    echo Found existing MSYS2 install at %MSYS2_INSTALL_DIR%, skipping download.
    goto :add_msys_path
)

echo [setup 1/4] Downloading MSYS2 installer...
powershell -NoProfile -Command "Invoke-WebRequest -Uri '%MSYS2_URL%' -OutFile '%MSYS2_INSTALLER%'"
if not exist "%MSYS2_INSTALLER%" (
    echo ERROR: download failed. Check your internet connection, or install
    echo MSYS2 manually from https://www.msys2.org and re-run this script.
    pause
    exit /b 1
)

echo [setup 2/4] Installing MSYS2 to %MSYS2_INSTALL_DIR% ^(silent, background^)...
"%MSYS2_INSTALLER%" install --root "%MSYS2_INSTALL_DIR%" --confirm-command
if errorlevel 1 (
    echo ERROR: MSYS2 installation failed.
    echo Try running it manually: %MSYS2_INSTALLER%
    pause
    exit /b 1
)

echo [setup 3/4] Installing MinGW-w64 GCC toolchain via pacman...
"%MSYS2_INSTALL_DIR%\usr\bin\bash.exe" -lc "pacman -Sy --noconfirm mingw-w64-x86_64-gcc"
if errorlevel 1 (
    echo ERROR: pacman failed to install the compiler.
    echo Open "MSYS2 MinGW64" from the Start menu and run manually:
    echo   pacman -S mingw-w64-x86_64-gcc
    pause
    exit /b 1
)

echo [setup 4/4] Compiler installed.
echo.

:add_msys_path
REM Add MinGW64's bin to THIS session's PATH so g++ works below, and to
REM the permanent user PATH so future terminals have it too.
set "PATH=%MSYS2_INSTALL_DIR%\mingw64\bin;%PATH%"

echo %PATH% | findstr /I /C:"%MSYS2_INSTALL_DIR%\mingw64\bin" >nul
for /f "usebackq tokens=2,*" %%A in (`reg query "HKCU\Environment" /v Path 2^>nul`) do set "OLD_USER_PATH=%%B"
echo !OLD_USER_PATH! | findstr /I /C:"%MSYS2_INSTALL_DIR%\mingw64\bin" >nul
if errorlevel 1 (
    if defined OLD_USER_PATH (
        setx PATH "!OLD_USER_PATH!;%MSYS2_INSTALL_DIR%\mingw64\bin" >nul
    ) else (
        setx PATH "%MSYS2_INSTALL_DIR%\mingw64\bin" >nul
    )
)

where g++ >nul 2>nul
if errorlevel 1 (
    echo ERROR: g++ still not found after setup. Something went wrong.
    echo Try opening a NEW terminal window and re-running this script
    echo ^(PATH changes sometimes need a fresh terminal session^).
    pause
    exit /b 1
)

:gcc_ready
echo Compiler found: 
where g++
echo.

REM ============================================================
REM STEP 1: Check source files exist
REM ============================================================
if not exist "CnR.cpp" (
    echo ERROR: CnR.cpp not found in this folder.
    pause
    exit /b 1
)
if not exist "install.cpp" (
    echo ERROR: install.cpp not found in this folder.
    pause
    exit /b 1
)
if not exist "win_socket_compat.h" (
    echo ERROR: win_socket_compat.h not found in this folder.
    echo CnR.cpp needs this to compile on Windows.
    pause
    exit /b 1
)

REM ============================================================
REM STEP 2: Compile CnR.exe
REM ============================================================
echo [1/3] Compiling CnR.exe...
g++ -std=c++17 -O3 -pthread -static CnR.cpp -o CnR.exe -lws2_32
if errorlevel 1 (
    echo ERROR: CnR.exe compilation failed. See errors above.
    pause
    exit /b 1
)
echo       -^> CnR.exe built.
echo.

REM ============================================================
REM STEP 3: Compile the installer helper
REM ============================================================
echo [2/3] Compiling installer...
g++ -std=c++17 -O2 -static install.cpp -o install-helper.exe -lole32 -luuid
if errorlevel 1 (
    echo ERROR: installer compilation failed. See errors above.
    pause
    exit /b 1
)
echo       -^> installer built.
echo.

REM ============================================================
REM STEP 4: Run the installer (copies CnR.exe, adds it to PATH)
REM ============================================================
echo [3/3] Installing...
install-helper.exe
if errorlevel 1 (
    echo ERROR: install step failed.
    pause
    exit /b 1
)

echo.
echo All done. Open a NEW terminal window and try:  CnR "args"
pause