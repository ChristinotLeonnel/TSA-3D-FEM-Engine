@echo off
rem Configuration, compilation et tests (Ninja + MSVC) dans .\build
setlocal
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release || exit /b 1
cmake --build build || exit /b 1
if /i "%1"=="test" ctest --test-dir build --output-on-failure || exit /b 1
endlocal
