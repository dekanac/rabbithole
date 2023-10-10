@echo off
rem Change to the directory where the batch file is located
cd /d "%~dp0"

rem Create a new directory for the build files
if not exist "build" mkdir build

rem Change to the new build directory
cd build

rem Run cmake to generate the project files
cmake .. -G "Visual Studio 17 2022" -A x64

cd ..\res\shaders\
rem Call the other batch script
call compile.bat

rem Pause again to keep the command window open after the other script has executed
pause