@echo off
set "VCTOOLS=F:\VSBuildTools\2022\VC\Tools\MSVC\14.44.35207"
set "SDKROOT=F:\zzz-rev\tools\sdk-nuget\headers\c"
set "INCLUDE=%VCTOOLS%\include;%SDKROOT%\Include\10.0.26100.0\ucrt;%SDKROOT%\Include\10.0.26100.0\shared;%SDKROOT%\Include\10.0.26100.0\um"
set "LIB=%VCTOOLS%\lib\x64;%SDKROOT%\Lib\10.0.26100.0\ucrt\x64;%SDKROOT%\Lib\10.0.26100.0\um\x64"
set "PATH=%VCTOOLS%\bin\Hostx64\x64;%PATH%"
set "TRAINER_BUILD_DIR=%~dp0..\.local\native-module"
if not "%~1"=="" set "TRAINER_BUILD_DIR=%~f1"
if not exist "%TRAINER_BUILD_DIR%" mkdir "%TRAINER_BUILD_DIR%"
"%VCTOOLS%\bin\Hostx64\x64\cl.exe" /nologo /std:c++17 /W4 /WX /EHsc /O2 /MT /LD /utf-8 /DUNICODE /D_UNICODE /Fo:"%TRAINER_BUILD_DIR%\trainer_bridge.obj" "%~dp0trainer_bridge.cpp" /link /OUT:"%TRAINER_BUILD_DIR%\trainer_bridge.dll" /IMPLIB:"%TRAINER_BUILD_DIR%\trainer_bridge.lib" /INCREMENTAL:NO /MAP:"%TRAINER_BUILD_DIR%\trainer_bridge.map"
exit /b %errorlevel%
