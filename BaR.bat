@echo off
cd build
cmake .. 
cmake --build . --target test_3 --config Release
cd ..
build\Release\test_3.exe
