
@echo off

REM subdir template is badly handled under Windows (at leaset under WinME),
REM so a batch file is provided to take care of the compilation process.

cd src\lib
qmake
nmake all >Build.log

cd ..\exec
qmake
nmake all >Build.log

cd ..\qplugin_generator
qmake
nmake all >Build.log

cd ..\plugins\default
qmake
nmake all >Build.log

cd ..\assistant
qmake
nmake all >Build.log

cd ..\designer
qmake
nmake all >Build.log

cd ..\gdb
qmake
nmake all >Build.log

cd ..\..\..

:END
