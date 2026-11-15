@echo off
setlocal
cd /d "%~dp0"
set "TEMP=%~dp0.local\runtime\temp"
set "TMP=%TEMP%"
set "PYTHONPYCACHEPREFIX=%~dp0.local\runtime\pycache"
set "LOCALAPPDATA=%~dp0.local\runtime\LocalAppData"
set "APPDATA=%~dp0.local\runtime\Roaming"
set "UE-LocalDataCachePath=%~dp0.local\runtime\DerivedDataCache"
set "PYTHONUTF8=1"
for %%D in ("%TEMP%" "%PYTHONPYCACHEPREFIX%" "%LOCALAPPDATA%" "%APPDATA%" "%UE-LocalDataCachePath%") do if not exist "%%~D" mkdir "%%~D"
python -X utf8 "%~dp0tools\start_local_server_console.py" --open %*
set "launcherExitCode=%errorlevel%"
if not "%launcherExitCode%"=="0" pause
exit /b %launcherExitCode%
