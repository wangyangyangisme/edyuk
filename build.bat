
@echo off

REM subdir template is badly handled under Windows (at leaset under WinME),
REM so a batch file is provided to take care of the compilation process.

cd src\lib
qmake
mingw32-make

cd ..\exec
qmake
mingw32-make

cd ..\qplugin_generator
qmake
mingw32-make

cd ..\plugins\default
qmake
mingw32-make

cd ..\assistant
qmake
mingw32-make

cd ..\designer
qmake
mingw32-make

cd ..\gdb
qmake
mingw32-make

cd ..\..\..

:END
