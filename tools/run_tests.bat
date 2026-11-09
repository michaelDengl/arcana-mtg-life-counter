@echo off
setlocal
cd /d "%~dp0.."
if exist ".tools\cmake-3.31.6-windows-x86_64\bin\ctest.exe" set "PATH=%CD%\.tools\cmake-3.31.6-windows-x86_64\bin;%PATH%"
where ctest >nul 2>nul
if errorlevel 1 (
 echo CTest missing. Install CMake, then open a new terminal.
 exit /b 1
)
ctest --test-dir build -C Release --output-on-failure
exit /b %errorlevel%
