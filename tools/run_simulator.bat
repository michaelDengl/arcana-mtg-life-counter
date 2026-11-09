@echo off
setlocal
cd /d "%~dp0.."
set "SIM_EXE=build\mtg_simulator.exe"
if exist build\Release\mtg_simulator.exe set "SIM_EXE=build\Release\mtg_simulator.exe"
if not exist "%SIM_EXE%" (
 echo Simulator missing. Run tools\build_simulator.bat first.
 exit /b 1
)
"%SIM_EXE%"
exit /b %errorlevel%
