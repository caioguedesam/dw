@echo off
REM ---------------------------------------------
REM Generating build includes
setlocal enabledelayedexpansion

REM Define the ANSI escape character
for /F %%a in ('echo prompt $E ^| cmd') do @set "ESC=%%a"

REM Define color variables
set "RED=%ESC%[31m"
set "GREEN=%ESC%[32m"
set "BLUE=%ESC%[34m"
set "RESET=%ESC%[0m"

for /F "tokens=1-4 delims=:.," %%a in ("%time%") do (
   set /A "start=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
)

REM Output file
set INCLUDES_OUTFILE=./generated/build_includes.hpp
echo Generating %INCLUDES_OUTFILE%...

if not exist "./generated" (
    mkdir "./generated"
)

REM Write header
echo #pragma once > "%INCLUDES_OUTFILE%"

set "IGNORED_FILES=src/main.cpp src/dependencies.cpp"

REM Loop recursively through .hpp and .cpp files
for /r src %%f in (*.hpp *.cpp) do (
    REM Get relative path from current directory
    set "REL=%%f"
    set "REL=!REL:%CD%\=!"
    REM Replace backslashes with forward slashes
    set "REL=!REL:\=/!"

    set "IGNORE_FILE=0"
    for %%I in (!IGNORED_FILES!) do (
        if /I "!REL!"=="%%I" set "IGNORE_FILE=1"
    )

    if !IGNORE_FILE!==0 (
        echo #include "../!REL!" >> "%INCLUDES_OUTFILE%"
    )

    rem if /i not "!REL!"=="src/main.cpp" (
    rem     echo #include "../!REL!" >> "%INCLUDES_OUTFILE%"
    rem )
)

echo %GREEN%Generated %INCLUDES_OUTFILE%%RESET%.

REM ---------------------------------------------
REM Building app

set BUILD=debug
set GENERATE_JSON=0
set BUILD_DEPENDENCIES=0
for %%A in (%*) do (
    if "%%~A"=="-r" set BUILD=release
    if "%%~A"=="-d" set BUILD=debug
    if "%%~A"=="--json" set GENERATE_JSON=1
    if "%%~A"=="--dependencies" set BUILD_DEPENDENCIES=1
)

if "%~1"=="-r" set BUILD=release
if "%~1"=="-d" set BUILD=debug

set DEPFILE=./build/%BUILD%/dw_dependencies
set OUTFILE=./build/%BUILD%/dw.exe

set VULKAN_SDK_PATH=%VULKAN_SDK:\=/%
set CC=clang
set CC_FLAGS=-std=c++17 -I./ -I%VULKAN_SDK_PATH%/Include/ -fms-runtime-lib=dll
set DEFINES=-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS
set L_FLAGS=-luser32.lib -Wl,-nodefaultlib:libcmt -l%DEPFILE:/=\%.lib

if "%BUILD%"=="debug" (
    set CC_FLAGS_O=-O0 --debug
) else (
    set CC_FLAGS_O=-Ofast
)

if not exist "./build/%BUILD%" (
    mkdir "./build/%BUILD%"
)

rem Building dependencies
if %BUILD_DEPENDENCIES%==1 (
    echo Building %DEPFILE%.lib...
    %CC% %CC_FLAGS% -Ofast -Wno-nullability-completeness -c %DEFINES% ./src/dependencies.cpp -o %DEPFILE:/=\%.obj
    lib /OUT:%DEPFILE:/=\%.lib %DEPFILE:/=\%.obj user32.lib gdi32.lib %VULKAN_SDK_PATH%/Lib/vulkan-1.lib >nul
    del "%DEPFILE:/=\%.obj"
)

rem Building app
echo Building %OUTFILE% [%BUILD%]...
%CC% %CC_FLAGS% %CC_FLAGS_O% %DEFINES% ./src/main.cpp %L_FLAGS% -o %OUTFILE%

rem Get end time:
for /F "tokens=1-4 delims=:.," %%a in ("%time%") do (
   set /A "end=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
)

rem Get elapsed time:
set /A elapsed=end-start

rem Show elapsed time:
set /A hh=elapsed/(60*60*100), rest=elapsed%%(60*60*100), mm=rest/(60*100), rest%%=60*100, ss=rest/100, cc=rest%%100
if %mm% lss 10 set mm=0%mm%
if %ss% lss 10 set ss=0%ss%
if %cc% lss 10 set cc=0%cc%

echo %GREEN%Build finished in %mm%:%ss%:%cc%.%RESET% 

rem Compile_commands.json generation (for clangd)
set COMPILE_COMMAND=clang %CC_FLAGS% %DEFINES% -c ./src/main.cpp
set CURR_DIR=%~dp0
set CURR_DIR=!CURR_DIR:\=/!
set CURR_DIR=%CURR_DIR:~0,-1%
set JSON_FILE=./compile_commands.json

if %GENERATE_JSON%==1 (
    REM Write JSON file
    (
        echo [
        echo   {
        echo     "directory": "!CURR_DIR!",
        echo     "command": "%COMPILE_COMMAND%",
        echo     "file": "src/main.cpp"
        echo   }
        echo ]
    ) > "%JSON_FILE%"
    
    echo %BLUE%Generated %JSON_FILE%%RESET%.
)
