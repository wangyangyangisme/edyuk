TEMPLATE = lib

CONFIG += plugin \
	uitools \
	help

TARGET = assistant

DESTDIR = ../../../plugins

EDYUK = plugin

include(../../../config.pri)

QT *= xml

INCLUDEPATH += ../../../3rdparty/qpluginsystem \
	../../../3rdparty/qmdi \
	../../../3rdparty/qcumber \
	../../../3rdparty/qcodeedit2/lib \
	../../../3rdparty/qcodeedit2/lib/document \
	../../../3rdparty/qcodeedit2/lib/language \
	../../lib

HEADERS += index.h \
	assistant.h \
	assistantclient.h \
	assistantperspective.h \
	qrcedit.h

SOURCES += extra.cpp \
	index.cpp \
	assistant.cpp \
	assistantclient.cpp \
	assistantperspective.cpp \
	qrcedit.cpp

FORMS += search.ui

TRANSLATIONS += ../../../translations/assistant_untranslated.ts

QPLUGIN_SCHEMES += assistant.xml

RESOURCES += plugin.qrc

win32 {
	QMAKE_QPLUGIN_GENERATOR = ..\..\..\qplugin_generator.exe
} else {
	QMAKE_QPLUGIN_GENERATOR = ../../../qplugin_generator
}

load(../../../installs/features/qplugin_from_scheme.prf)

APP_NAME = $$quote(Edyuk Assistant plugin)

APP_AUTHOR = fullmetalcoder
