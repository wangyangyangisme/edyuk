
# test project....

TEMPLATE = app
TARGET = qpm
CONFIG += debug

DEPENDPATH += qmake

UI_DIR = .build
RCC_DIR = .build
MOC_DIR = .build
OBJECTS_DIR = .build

HEADERS += qmakeparser.h

SOURCES += main.cpp \
	qmakeparser.cpp

include(qprojectmodel.pri)
