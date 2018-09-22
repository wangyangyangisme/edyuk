TEMPLATE = app

TARGET = qplugin_generator

DESTDIR = ../..

CONFIG -= app_bundle

CONFIG += debug_and_release

QT += xml

QT -= gui

RCC_DIR = .build

OBJECTS_DIR = .build

# Input

SOURCES += main.cpp

RESOURCES += qpg.qrc

CONFIG(debug,release|debug) {
	DEFINES += _DEBUG_
} else {
	DEFINES += _RELEASE_
}

APP_NAME = QPluginGenerator

APP_AUTHOR = fullmetalcoder
