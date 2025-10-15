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

REM Loop recursively through .hpp and .cpp files
for /r src %%f in (*.hpp *.cpp) do (
    REM Get relative path from current directory
    set "REL=%%f"
    set "REL=!REL:%CD%\=!"
    REM Replace backslashes with forward slashes
    set "REL=!REL:\=/!"
    echo #include "../!REL!" >> "%INCLUDES_OUTFILE%"
)

echo %GREEN%Generated %INCLUDES_OUTFILE%%RESET%.

REM ---------------------------------------------
REM Building app

set BUILD=debug

if "%~1"=="-r" set BUILD=release
if "%~1"=="-d" set BUILD=debug

set OUTFILE=./build/%BUILD%/dw.exe
echo Building %OUTFILE% [%BUILD%]...

set CC=clang
set DEFINES=-DWIN32_LEAN_AND_MEAN -DNOMINMAX -D_CRT_SECURE_NO_WARNINGS
set L_FLAGS=-luser32.lib -Wl,-nodefaultlib:libcmt

if "%BUILD%"=="debug" (
    set CC_FLAGS=-std=c++17 -I./ -O0 --debug -fms-runtime-lib=dll
) else (
    set CC_FLAGS=-std=c++17 -I./ -Ofast -fms-runtime-lib=dll
)

if not exist "./build/%BUILD%" (
    mkdir "./build/%BUILD%"
)

%CC% %CC_FLAGS% %DEFINES% ./main.cpp %L_FLAGS% -o %OUTFILE%

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
