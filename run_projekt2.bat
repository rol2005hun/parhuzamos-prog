@echo off
echo =======================================================
echo Project 2 - Mandelbrot ^& Snowflake Build ^& Run
echo =======================================================
echo.

:: Check if UCRT64 CMake is installed
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake is not in the PATH! 
    echo Open the "MSYS2 UCRT64" terminal and run this script from there,
    echo or add the C:\msys64\ucrt64\bin folder to your Windows PATH.
    pause
    exit /b
)

cd projekt2

echo [1/3] CMake Configure...
cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release --toolchain toolchain-ucrt64.cmake
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] CMake Configure failed!
    pause
    exit /b
)

echo.
echo [2/3] Project Build (Ninja)...
cmake --build build --parallel
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b
)

echo.
echo [3/3] Running...
cd build
start SnowflakeMandelbrot.exe
cd ../..
