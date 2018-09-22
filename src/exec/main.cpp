/****************************************************************************
**
** Copyright (C) 2006-2008 fullmetalcoder <fullmetalcoder@hotmail.fr>
**
** This file is part of the Edyuk project <http://edyuk.org>
**
** This file may be used under the terms of the GNU General Public License
** version 2 as published by the Free Software Foundation and appearing in the
** file GPL.txt included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "version.h"
#include "edyukapplication.h"

#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

int main (int argc, char **argv)
{
	/*
		Setup app stuffs needed for QSettings and QSingleApplication use
	*/
	EdyukApplication::setApplicationName("edyuk-" EDYUK_VERSION_STR);
	EdyukApplication::setOrganizationName("FullMetalCoder");
	EdyukApplication::setOrganizationDomain("http://edyuk.sourceforge.net/");
	
	EdyukApplication::addLibraryPath(EdyukApplication::applicationDirPath());
	EdyukApplication::addLibraryPath(EdyukApplication::applicationDirPath() + "/plugins");
	
	/*
		Create the application object
	*/
	EdyukApplication app(argc, argv);
	
	/*
		Execute the application
	*/
	int exit = app.exec();
	
	/*
		Check application return code
	*/
	if ( !exit )
		qDebug("Edyuk successfully terminated!!!");
	
	return exit;
}
