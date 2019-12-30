setlocal
cd /d %~dp0
call "setenv_x64.bat"
cd %GRA_ROOT%
..\CMake\bin\cmake-gui.exe -G "Visual Studio 15"