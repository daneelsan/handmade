@echo off

IF NOT EXIST ..\data mkdir ..\data
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

del *.pdb > NUL 2> NUL

echo %cd%
cl -DHANDMADE_INTERNAL=1 -DHANDMADE_SLOW=1 -FAsc -Zi ..\code\win32_handmade.c User32.lib Gdi32.lib

popd
