@echo off
cd /d "%~dp0"
set "TEMP=%~dp0.local\runtime\temp"
set "TMP=%TEMP%"
if not exist "%TEMP%" mkdir "%TEMP%"
python -X utf8 "%~dp0tools\training_server.py" --open
if errorlevel 1 pause
