@echo off
setlocal
cd /d "%~dp0.."
if exist ".tools\cmake-3.31.6-windows-x86_64\bin\cmake.exe" set "PATH=%CD%\.tools\cmake-3.31.6-windows-x86_64\bin;%PATH%"
where cmake >nul 2>nul
if errorlevel 1 (
 echo CMake missing. Install: winget install --id Kitware.CMake -e
 exit /b 1
)
if exist ".tools\llvm-mingw-20250305-ucrt-x86_64\bin\clang.exe" (
 set "PATH=%CD%\.tools\llvm-mingw-20250305-ucrt-x86_64\bin;%PATH%"
 cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
) else (
 cmake -S . -B build -G "Visual Studio 17 2022" -A x64
)
if errorlevel 1 (
 echo Configuration failed. See README for compiler installation and dependency download requirements.
 exit /b 1
)
cmake --build build --config Release --parallel 4
if errorlevel 1 exit /b 1
echo Build complete. Run tools\run_simulator.bat
