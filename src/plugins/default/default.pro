TEMPLATE = lib

CONFIG += plugin uitools

QT += script

TARGET = default

DESTDIR = ../../../plugins

EDYUK = plugin

include(../../../config.pri)

INCLUDEPATH += ../../../3rdparty/qpluginsystem \
	../../../3rdparty/qprojectmodel2 \
	../../../3rdparty/qcodemodel2 \
	../../../3rdparty/qmdi \
	../../../3rdparty/qcodeedit2/lib \
	../../../3rdparty/qcodeedit2/lib/document \
	../../../3rdparty/qcodeedit2/lib/language \
	../../../3rdparty/qcodeedit2/lib/widgets \
	../../lib \
	qmake

HEADERS += qmakeparser.h \
	qmakebuilder.h \
	gnumakebuilder.h \
	qcpplexer.h \
	qcppparser.h \
	cppcompletion.h \
	qmakesettings.h \
	qmakebackend.h \
	uisubclass.h \
	qmake/project.h \
	qmake/property.h \
	qmake/option.h

SOURCES += extra.cpp \
	qmakeparser.cpp \
	qmakebuilder.cpp \
	gnumakebuilder.cpp \
	qcpplexer.cpp \
	qcppparser.cpp \
	cppcompletion.cpp \
	qmakesettings.cpp \
	qmakebackend.cpp \
	uisubclass.cpp \
	qmakereadonlyparser.cpp \
	qmake/project.cpp \
	qmake/property.cpp \
	qmake/option.cpp

FORMS += projectsettings.ui subclassing.ui

TRANSLATIONS += ../../../translations/default_untranslated.ts

QPLUGIN_SCHEMES += default.xml

RESOURCES += plugin.qrc

win32 {
	QMAKE_QPLUGIN_GENERATOR = ..\..\..\qplugin_generator.exe
} else {
	QMAKE_QPLUGIN_GENERATOR = ../../../qplugin_generator
}

load(../../../installs/features/qplugin_from_scheme.prf)

APP_NAME = $$quote(Edyuk C++ plugin)

APP_AUTHOR = fullmetalcoder
