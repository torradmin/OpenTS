@echo off

cmake -S . -B build -G "Visual Studio 17 2022" -A Win32
cmake --build build --config Debug
rem cmake --build build --config Release