@echo off
REM -MTd
set CompilerFlags=-MTd -nologo -Zi -IC:\dev\external\sdl\current\include -DARO_WINDOWS
set LinkerFlags=-nodefaultlib:msvcrt -subsystem:windows -libpath:C:\dev\external\sdl\current\lib\x64 -incremental:no -opt:ref SDL2.lib SDL2main.lib

IF NOT EXIST build mkdir build
pushd build

REM 32-bit build
cl %CompilerFlags% ..\code\main.cpp /link %LinkerFlags%
popd