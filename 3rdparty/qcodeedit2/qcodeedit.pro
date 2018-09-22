TEMPLATE = subdirs

CONFIG += debug

SUBDIRS += lib example designer-plugin

CONFIG(debug, debug|release) {
	# placeholder
	QCODE_EDIT_EXTRA_DEFINES += _DEBUG_BUILD_
} else {
	# placeholder
	QCODE_EDIT_EXTRA_DEFINES += _RELEASE_BUILD_
}

include(install.pri)
