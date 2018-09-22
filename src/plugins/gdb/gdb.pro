TEMPLATE = lib

CONFIG += plugin

TARGET = gdb

DESTDIR = ../../../plugins

EDYUK = plugin

include(../../../config.pri)

INCLUDEPATH += ../../../3rdparty/qpluginsystem \
	../../../3rdparty/qmdi \
	../../../3rdparty/qcodeedit2/lib \
	../../../3rdparty/qcodeedit2/lib/document \
	../../../3rdparty/qcodeedit2/lib/language \
	../../../3rdparty/qcodeedit2/lib/widgets \
	../../lib

HEADERS += gdbdriver.h \
	gdbdriverui.h \
	gdbdriverthread.h \
	gdbresult.h \
	gdbmemory.h

SOURCES += extra.cpp \
	gdbdriver.cpp \
	gdbdriverui.cpp \
	gdbdriverthread.cpp \
	gdbmemory.cpp

FORMS += gdb.ui

TRANSLATIONS += ../../../translations/gdb_untranslated.ts

QPLUGIN_SCHEMES += gdb.xml

RESOURCES += plugin.qrc

win32 {
	QMAKE_QPLUGIN_GENERATOR = ..\..\..\qplugin_generator.exe
} else {
	QMAKE_QPLUGIN_GENERATOR = ../../../qplugin_generator
}

load(../../../installs/features/qplugin_from_scheme.prf)

APP_NAME = $$quote(Edyuk GDB plugin)

APP_AUTHOR = fullmetalcoder
