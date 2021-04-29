@echo off

REM
REM  To run this at startup, use this as your shortcut target:
REM  %windir%\system32\cmd.exe /k w:\repos\handmade\misc\shell.bat
REM

pushd .
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
popd
set path=w:\repos\handmade\misc;%path%
set _NO_DEBUG_HEAP=1

REM This is just faking the computer name so we don't have to change the 4coder config PogChamp
REM set COMPUTERNAME=DANIELWINPC

