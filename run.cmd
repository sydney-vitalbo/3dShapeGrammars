@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

cmake --preset windows
if errorlevel 1 goto :done

cmake --build --preset windows-debug --parallel
if errorlevel 1 goto :done

".\build\windows\bin\Debug\shape_grammars.exe"

:done
set "run_exit=%errorlevel%"
popd
exit /b %run_exit%
