: <<'WINDOWS_BATCH'
@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

cmake --preset windows && cmake --build --preset windows-debug --parallel && ".\build\windows\bin\Debug\shape_grammars.exe"
set "run_exit=%errorlevel%"
popd
exit /b %run_exit%
WINDOWS_BATCH

# Run on macOS with: sh ./run.cmd
set -e
CDPATH= cd -- "$(dirname -- "$0")"

# Some Command Line Tools installations only have libc++ headers in the SDK.
if [ -z "${CXX:-}" ] && [ "$(command -v c++)" = /usr/bin/c++ ] &&
    ! printf '#include <cstddef>\n' | c++ -fsyntax-only -x c++ - >/dev/null 2>&1; then
    sdk_headers="$(xcrun --sdk macosx --show-sdk-path)/usr/include/c++/v1"
    if [ -r "$sdk_headers/cstddef" ]; then
        export CPLUS_INCLUDE_PATH="$sdk_headers${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}"
    fi
fi

cmake --preset macos-debug
cmake --build --preset macos-debug --parallel

exec ./build/macos-debug/bin/shape_grammars
