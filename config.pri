#
# Configuration file used in Edyuk build process
# 

VERSION = 1.1.0

# If you modify this file make sure that you follow the recommandations
# given by the comments. If you modify the file including this one (i.e.
# src/lib/lib.pro, src/exec/exec.pro and src/default/default.pro) make
# sure that two variables are properly set BEFORE the include directive :
# 	* EDYUK => controls the "type" of the project (lib, exec, default)
#	* TARGET => used and modified by this config file
# 
# Important note : all TARGET changes should be made in that file and not
# in the sub-projects.
#

DEFINES += _EDYUK_ \
	_QMDI_ \
	__QMDI__ \
	_QCUMBER_ \
	_QCODE_EDIT_ \
	_QCODE_MODEL_ \
	_QPROJECT_MODEL_ \
	_QSAFE_SHARED_SETTINGS_

#
# Controls build mode. Changes that value to :
#	* "debug"	=> edyuk will be built in debug mode
#	* "release"	=> edyuk will be built in release mode (default under
#	window$)
#	* "debug_and_release"	=> edyuk will be built in release mode
#	by default but proper makefiles are created to allow seamless
#	debug build (default under Linux)
#	* "debug_and_release build_all"	=> edyuk will be built
#	in both debug and release mode
#
# Qt packages under windows doesn't contains debug libraries so
# under this platform the default has been set to "release"
#

CONFIG += debug_and_release

TMPDIR = .build/$$[QT_VERSION]-

win32 {
	TMPDIR = $$join(TMPDIR,,,win32)
}

unix {
	TMPDIR = $$join(TMPDIR,,,unix)
}

macx {
	TMPDIR = $$join(TMPDIR,,,macx)
}

#
# Controls destination of ui files
#

UI_DIR = $${TMPDIR}/ui

#
# Controls destination of moc files
#

MOC_DIR = $${TMPDIR}/moc

#
# Controls destination of rcc files
#

RCC_DIR = $${TMPDIR}/rcc

#
# Controls destination of object files
#

OBJECTS_DIR = $${TMPDIR}/obj

#
# if the lib is being built, set _EDYUK_BUILD_ define. Otherwise adds a
# library path so that app and plugin link smoothly with core library.
# The first part is especially important under Window$, don't remove!
#

contains(EDYUK,lib) {
	DEFINES += _EDYUK_BUILD_
} else {
	contains(EDYUK,plugin) {
		LIBS += -L"../../.."
	} 	else {
		LIBS += -L"../.."
	}

}

#
# special qmake variables that I found in *.pri files
# given by Trolltech to build Qt.
#
# Do they really have an effect ? Dunno but does that really matter? ;-)
#

QMAKE_TARGET_COMPANY = "Edyk team"

QMAKE_TARGET_PRODUCT = "Edyuk"

QMAKE_TARGET_DESCRIPTION = "Integrated Development Environment for Qt4."

QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2006-2007 fullmetalcoder"

#
# Controls build settings specific to debug and release modes
#
# Important warning : don't move the bracket ! I'm not kidding
# qmake gets mad if the CONFIG() statement and the bracket aren't
# on the same line... It creates two build targets using regular
# debug and release data but apply to each the content of the 
# debug scope.
#

CONFIG(debug,release|debug) {
	#
	# Edyuk is build in debug mode : this macros causes
	# hundreds of debug output everywhere in the app.
	#

	DEFINES += _EDYUK_DEBUG_ \
		_DEBUG_

	#
	# Appends "_debug" to the name of : 
	# 	* the app
	#	* the core library
	#	* the plugins
	#
	# Be careful modifying this! Plugins won't be loaded
	# correctly, unless you change devpluginmanager.cpp
	# accordingly, moreover app and plugins won't be
	# linked correctly unless you propagate the changes
	# to the LIBS variable...
	#

	TARGET = $$join(TARGET,,,_debug)

	#
	# objects files are put in separate location according
	# to the build mode. You can change that but it would
	# be uselesss...
	#

	OBJECTS_DIR = $$join(OBJECTS_DIR,,,/debug)

	#
	# links the current project to Edyuk's core library,
	# except if it's the library itself...
	#

	!contains(EDYUK,lib) {
		LIBS += -ledyuk_debug
	}

} else {
	#
	# objects files are put in separate location according
	# to the build mode. You can change that but it would
	# be uselesss...
	#

	OBJECTS_DIR = $$join(OBJECTS_DIR,,,/release)

	#
	# links the current project to Edyuk's core library,
	# except if it's the library itself...
	#

	!contains(EDYUK,lib) {
		LIBS += -ledyuk
	}

}

#
# Some versions of qmake create buggy makefiles under Win when
# using a non-empty UIDIR...
#

win32 {
}

#
# Under Unix the application name is suffixed with
# ".bin" so that the run script won't be overwritten
# and will still work.
# If you change this modify the build script to keep
# it working.
#

contains(EDYUK,exec) {
	unix {
		TARGET = $$join(TARGET,,,.bin)
	}

}

#
# Here we get rid of bundles... If we wanted to provide
# some qmake wouldn't generate them properly anyway given
# the depency schemes...
#

CONFIG -= app_bundle \
	lib_bundle

# tricks to get some debug info under win without being forced to build debug libs
#QMAKE_CXXFLAGS += -g
#QMAKE_LFLAGS_RELEASE -= -Wl,-s
