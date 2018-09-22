#
#	Edyuk's installer definitions.
#
# As part of the Edyuk project, this file is licensed under GPL v2
#
# Note : this file is meant to work with Unix-based systems only.
# Windows users may prefer the binary installer and Mac addicts
# would certainly like a bundle...
#
### START of INSTALL section

!build_pass {
	#
	# Location where Edyuk will be installed
	#

	sandbox {
		# config for sandbox install

		EDYUK_INSTALL_PREFIX = /usr/local/edyuk

		#
		# Necessary files without which Edyuk can't run
		#

		target.files += edyuk \
			edyuk*.bin \
			libedyuk*

		target.path = $${EDYUK_INSTALL_PREFIX}

		plugins.files += plugins/*

		plugins.path = $${EDYUK_INSTALL_PREFIX}/plugins

		INSTALLS += target \
			plugins
	} 	else {
		# config for "transparent" / "smooth" install (distro packager friendly)

		EDYUK_INSTALL_PREFIX = /usr/share/edyuk

		#
		# Necessary files without which Edyuk can't run
		#

		edyuk_exec.files += edyuk \
			edyuk*.bin

		edyuk_libs.files += libedyuk*

		edyuk_plugins.files += plugins/*

		edyuk_exec.path = /usr/bin/

		edyuk_libs.path = /usr/lib/

		edyuk_plugins.path = $${EDYUK_INSTALL_PREFIX}/plugins/

		INSTALLS += edyuk_exec \
			edyuk_libs \
			edyuk_plugins
	}

	#
	# Miscanellous files, generally appreciated... ;-)
	#

	misc.files += *.txt

	misc.path = $${EDYUK_INSTALL_PREFIX}

	translations.files += translations/*.qm

	translations.path = $${EDYUK_INSTALL_PREFIX}/translations

	doc.files += doc/*

	doc.path = $${EDYUK_INSTALL_PREFIX}/doc

	qxs.files += qxs/*

	qxs.path = $${EDYUK_INSTALL_PREFIX}/qxs

	templates.files += templates/*

	templates.path = $${EDYUK_INSTALL_PREFIX}/templates

	INSTALLS += translations \
		doc \
		qxs \
		templates \
		misc

	#
	#	SDK : Edyuk plugin development kit
	#

	sdk.path = $$[QT_INSTALL_HEADERS]/Edyuk

	SUB = src/lib \
		3rdparty/qmdi \
		3rdparty/qcumber \
		3rdparty/qcodemodel2 \
		3rdparty/qprojectmodel2 \
		3rdparty/qpluginsystem \
		3rdparty/qcodeedit2/lib \
		3rdparty/qcodeedit2/lib/document \
		3rdparty/qcodeedit2/lib/language \
		3rdparty/qcodeedit2/lib/qnfa \
		3rdparty/qcodeedit2/lib/widgets

	for(s,SUB) {
		eval(sdk.files*=$${s}/*.h)
	}

	libs.path = $$[QT_INSTALL_LIBS]/Edyuk

	libs.files += lib*

	features.files += installs/features/*

	features.path = $$[QT_INSTALL_PREFIX]/mkspecs/features

	qplugingenerator.path = $$[QT_INSTALL_BINS]

	qplugingenerator.files += qplugin_generator*

	INSTALLS += sdk \
		features \
		qplugingenerator

	unix {
		#
		# This copies the .desktop file (equivalent of Window$' .lnk) to :
		#	1 ) KDE menu
		#	2 ) Gnome menu
		#	3 ) User desktop [disabled : too invasive...]
		#
		# Looks like KDE is so smart that he finds both of them ;-)
		# But do not understand that they represent the same app...
		#
		#kde.files = $$DESKTOP
		#kde.path = /usr/share/applications/kde
		#INSTALLS += kde
		#

		sandbox {
			DESKTOP = installs/desktop/sandbox/edyuk.desktop
		} 		else {
			DESKTOP = installs/desktop/edyuk.desktop
		}

		gnome.files = $$DESKTOP

		gnome.path = /usr/share/applications

		INSTALLS += gnome

		user.files = $$DESKTOP

		user.path = ~/Desktop

		#INSTALLS += user
		#
		# This puts the icons into "standard" location for both KDE and Gnome
		#
		# Bonus hint : it's automatic!!! just add an icon size or a desktop to
		# the variables below and it will work (provided that you create folder
		# /path/to/edyuk/icons/$$size, put an edyuk.png icon in it and that the
		# wonderful new desktop you use stores its icons in :
		# /usr/share/icons/$$desktop/$$size/apps)
		#

		DESK = default.kde \
			gnome

		ICON = 16x16 \
			22x22 \
			32x32 \
			48x48 \
			64x64 \
			128x128

		for(d,DESK) {
			for(i,ICON) {
				eval(icons_$${d}_$${i}.files+=installs/icons/$${i}/*)
				eval(icons_$${d}_$${i}.path+=/usr/share/icons/$${d}/$${i}/apps)

				INSTALLS += icons_$${d}_$${i}
			}

		}

		#
		# Places static files to state Edyuk's MIME abilities
		#

		db.files += x-pro.desktop \
			x-qrc.desktop

		db.path = /usr/share/mimelnk/text

		mimes.files += edyuk.xml

		mimes.path = /usr/share/mime/packages

		INSTALLS += db \
			mimes
	}

	### END of INSTALL section
}

