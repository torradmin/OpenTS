@echo off

cmake -S . -B build_2026 -G "Visual Studio 18 2026" -A Win32 -T v143
rem cmake --build build --config Debug
cmake --build build_2026 --config Release