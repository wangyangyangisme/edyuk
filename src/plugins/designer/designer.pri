CONFIG *= designer

qtAddLibrary(QtDesignerComponents)

#LIBS += -lQtDesignerComponents

SOURCES += qdesignerclient.cpp \
	qdesignerperspective.cpp \
	qdesignerwidgetbox.cpp \
	qdesignerobjectinspector.cpp \
	qdesignerpropertyeditor.cpp \
	qdesigneractioneditor.cpp \
	qdesignersignalsloteditor.cpp \
	qdesignerinternals.cpp

HEADERS += qdesignerclient.h \
	qdesignerperspective.h \
	qdesignerwidgetbox.h \
	qdesignerpropertyeditor.h \
	qdesignerobjectinspector.h \
	qdesigneractioneditor.h \
	qdesignersignalsloteditor.h \
	qdesignerinternals.h

SOURCES += 4.3.5/qdesignerinternals-4.3.5.cpp \
	4.4.3/qdesignerinternals-4.4.3.cpp \
	4.5.0/qdesignerinternals-4.5.0.cpp
