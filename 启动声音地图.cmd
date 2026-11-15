@echo off
cd /d "%~dp0"
if exist "outputs\SoundMapLocator\SoundMapLocator.exe" (
  start "" "outputs\SoundMapLocator\SoundMapLocator.exe"
) else (
  pythonw "sound_locator\map_app.py"
)
