@echo off
setlocal

set CONFIG=Release
set GENERATOR=MinGW Makefiles
set CXX=D:/c_byq/mingw64/bin/g++.exe

REM parse args: "clean" and "run"
set RUN=
:loop_args
if "%~1"=="" goto args_parsed
if /I "%~1"=="clean" (
  rmdir /s /q build
  shift
  goto loop_args
)
if /I "%~1"=="run" (
  set RUN=1
  shift
  goto loop_args
)
shift
goto loop_args

:args_parsed

cmake -S . -B build -G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%CONFIG% -DCMAKE_CXX_COMPILER=%CXX%
if errorlevel 1 exit /b %errorlevel%

cmake --build build --config %CONFIG% -- -j
if errorlevel 1 exit /b %errorlevel%

if defined RUN (
  if exist build\Compiler.exe (
    build\Compiler.exe
  ) else if exist Compiler.exe (
    Compiler.exe
  ) else (
    echo Executable not found.
  )
)

endlocal
