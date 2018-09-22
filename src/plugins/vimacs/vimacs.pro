TEMPLATE = lib

CONFIG += plugin

TARGET = vimacs

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

HEADERS += vim.h \
	emacs.h

SOURCES += extra.cpp \
	vim.cpp \
	emacs.cpp

TRANSLATIONS += ../../../translations/vimacs_untranslated.ts

QPLUGIN_SCHEMES += vimacs.xml

RESOURCES += plugin.qrc

win32 {
	QMAKE_QPLUGIN_GENERATOR = ..\..\..\qplugin_generator.exe
} else {
	QMAKE_QPLUGIN_GENERATOR = ../../../qplugin_generator
}

load(../../../installs/features/qplugin_from_scheme.prf)

APP_NAME = $$quote(Edyuk Vimacs plugin)

APP_AUTHOR = fullmetalcoder
