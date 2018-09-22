QT *= network

DEFINES *= _QCUMBER_BUILD_

# Input

HEADERS += qcumber.h \
	qsingleapplication.h \
	qmanagedsocket.h \
	qmanagedrequest.h \
	qshortcutmanager.h \
	qshortcutdialog.h \
	qsettingsserver.h \
	qsettingsclient.h \
	qwidgetstack.h

SOURCES += qsingleapplication.cpp \
	qmanagedsocket.cpp \
	qmanagedrequest.cpp \
	qshortcutmanager.cpp \
	qshortcutdialog.cpp \
	qsettingsserver.cpp \
	qsettingsclient.cpp \
	qwidgetstack.cpp

win32 {
	HEADERS += qinterprocesschannel_win32.h
	SOURCES += qinterprocesschannel_win32.cpp
} else {
	HEADERS += qinterprocesschannel.h
	SOURCES += qinterprocesschannel.cpp
}


FORMS += shortcutdialog.ui
