@echo off
setlocal
set "PROGRAM=%~1"
if "%PROGRAM%"=="" set "PROGRAM=all"
if not defined CMAKE_GENERATOR (
  where cl >nul 2>nul
  if errorlevel 1 set "CMAKE_GENERATOR=MinGW Makefiles"
)
cmake -S . -B build
if errorlevel 1 exit /b 1
if "%PROGRAM%"=="all" (
  cmake --build build --config Release --parallel
  exit /b
)
cmake --build build --config Release --target %PROGRAM% --parallel
if errorlevel 1 exit /b 1
shift
set "RUN_ARGS="
:args
if "%~1"=="" goto run
set RUN_ARGS=%RUN_ARGS% %1
shift
goto args
:run
build\%PROGRAM%.exe %RUN_ARGS% %ARGS%
exit /b %errorlevel%
