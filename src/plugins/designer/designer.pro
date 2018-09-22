TEMPLATE = lib

CONFIG += plugin \
	uitools

TARGET = designer

DESTDIR = ../../../plugins

EDYUK = plugin

include(../../../config.pri)

QT *= xml

INCLUDEPATH += . \
	../../../3rdparty/qpluginsystem \
	../../../3rdparty/qmdi \
	../../../3rdparty/qcumber \
	../../../3rdparty/qcodeedit2/lib \
	../../../3rdparty/qcodeedit2/lib/document \
	../../../3rdparty/qcodeedit2/lib/language \
	../../lib

include(designer.pri)

SOURCES += extra.cpp

TRANSLATIONS += ../../../translations/designer_untranslated.ts

QPLUGIN_SCHEMES += designer.xml

RESOURCES += plugin.qrc

win32 {
	QMAKE_QPLUGIN_GENERATOR = ..\..\..\qplugin_generator.exe
} else {
	QMAKE_QPLUGIN_GENERATOR = ../../../qplugin_generator
}

load(../../../installs/features/qplugin_from_scheme.prf)

APP_NAME = $$quote(Edyuk Designer plugin)

APP_AUTHOR = fullmetalcoder
