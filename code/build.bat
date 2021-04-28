@echo off

IF NOT EXIST ..\data mkdir ..\data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

echo %cd%
cl -Zi ..\code\win32_handmade.c user32.lib

popd
