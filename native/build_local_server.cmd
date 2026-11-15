@echo off
set "VCTOOLS=F:\VSBuildTools\2022\VC\Tools\MSVC\14.44.35207"
set "SDKROOT=F:\zzz-rev\tools\sdk-nuget\headers\c"
set "INCLUDE=%VCTOOLS%\include;%SDKROOT%\Include\10.0.26100.0\ucrt;%SDKROOT%\Include\10.0.26100.0\shared;%SDKROOT%\Include\10.0.26100.0\um"
set "LIB=%VCTOOLS%\lib\x64;%SDKROOT%\Lib\10.0.26100.0\ucrt\x64;%SDKROOT%\Lib\10.0.26100.0\um\x64"
set "PATH=%VCTOOLS%\bin\Hostx64\x64;%PATH%"
set "LOCAL_SERVER_BUILD_DIR=%~f1"
if not exist "%LOCAL_SERVER_BUILD_DIR%" mkdir "%LOCAL_SERVER_BUILD_DIR%"
"%VCTOOLS%\bin\Hostx64\x64\cl.exe" /nologo /std:c++17 /W4 /WX /EHsc /O2 /MT /LD /utf-8 /DUNICODE /D_UNICODE /I"%~dp0." /Fo:"%LOCAL_SERVER_BUILD_DIR%\local_server_bridge.obj" "%~dp0local_server_bridge.cpp" /link user32.lib /OUT:"%LOCAL_SERVER_BUILD_DIR%\local_server_bridge.dll" /IMPLIB:"%LOCAL_SERVER_BUILD_DIR%\local_server_bridge.lib" /INCREMENTAL:NO /MAP:"%LOCAL_SERVER_BUILD_DIR%\local_server_bridge.map"
exit /b %errorlevel%
