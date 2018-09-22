cd src\lib
qmake -t vclib -o edyuk_lib.vcproj

cd ..\exec
qmake -t vcapp -o edyuk_exe.vcproj

cd ..\qplugin_generator
qmake -t vcapp -o qplugin_generator.vcproj

cd ..\plugins\default
qmake -t vcapp -o default.vcproj

cd ..\assistant
qmake -t vcapp -o assistant.vcproj

cd ..\designer
qmake -t vcapp -o designer.vcproj

cd ..\gdb
qmake -t vcapp -o gdb.vcproj

cd ..\..\..
