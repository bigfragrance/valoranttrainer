@echo off
set "VCTOOLS=F:\VSBuildTools\2022\VC\Tools\MSVC\14.44.35207"
set "SDKROOT=F:\zzz-rev\tools\sdk-nuget\headers\c"
set "INCLUDE=%VCTOOLS%\include;%SDKROOT%\Include\10.0.26100.0\ucrt;%SDKROOT%\Include\10.0.26100.0\shared;%SDKROOT%\Include\10.0.26100.0\um"
set "LIB=%VCTOOLS%\lib\x64;%SDKROOT%\Lib\10.0.26100.0\ucrt\x64;%SDKROOT%\Lib\10.0.26100.0\um\x64"
set "PATH=%VCTOOLS%\bin\Hostx64\x64;%PATH%"
if not exist "%~dp0..\.local\offline-gi-instance" mkdir "%~dp0..\.local\offline-gi-instance"
"%VCTOOLS%\bin\Hostx64\x64\cl.exe" /nologo /std:c++17 /W4 /WX /EHsc /O2 /MT /LD /utf-8 /DUNICODE /D_UNICODE /Fo:"%~dp0..\.local\offline-gi-instance\offline_gi_instance.obj" "%~dp0offline_gi_instance.cpp" /link /OUT:"%~dp0..\.local\offline-gi-instance\offline_gi_instance.dll" /IMPLIB:"%~dp0..\.local\offline-gi-instance\offline_gi_instance.lib" /INCREMENTAL:NO /MAP:"%~dp0..\.local\offline-gi-instance\offline_gi_instance.map"
exit /b %errorlevel%
