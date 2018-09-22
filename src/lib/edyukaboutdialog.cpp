/****************************************************************************
**
** Copyright (C) 2006-2009 fullmetalcoder <fullmetalcoder@hotmail.fr>
**
** This file is part of the Edyuk project <http://edyuk.org>
** 
** This file may be used under the terms of the GNU General Public License
** version 3 as published by the Free Software Foundation and appearing in the
** file GPL.txt included in the packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "edyukaboutdialog.h"

/*!
	\file edyukaboutdialog.cpp
	\brief Implementation of the EdyukAboutDialog class.
	
	\see EdyukAboutDialog
*/

#include <QFile>
#include <QTextStream>
#include <QApplication>

/*!
	\ingroup gui
	@{
	
	\class EdyukAboutDialog
	\brief "About Edyuk" dialog
	
*/

EdyukAboutDialog::EdyukAboutDialog(QWidget *p)
 : QDialog(p)
{
	setupUi(this);
	
	QString file = QApplication::applicationDirPath() + "/doc/";
	
	QFont courier("Courier", 10);
	courier.setStyleHint(QFont::Courier);
	
	QFile f; //(file+"about.htm");
	QTextStream in(&f);
	
	//if ( f.open(QFile::ReadOnly | QFile::Text) )
	//	eAbout->setHtml(in.readAll());
	
	//f.close();
	
	
	f.setFileName(":/GPL.txt");
	if ( f.open(QFile::ReadOnly | QFile::Text) )
	{
		eLicense->setPlainText(in.readAll());
		eLicense->document()->setDefaultFont(courier);
	}
	
	f.close();
	f.setFileName(":/Changelog.txt");
	if ( f.open(QFile::ReadOnly | QFile::Text) )
	{
		eChangelog->setPlainText(in.readAll());
		eChangelog->document()->setDefaultFont(courier);
	}
	
	f.close();
	f.setFileName(":/thanks.htm");
	if ( f.open(QFile::ReadOnly | QFile::Text) )
		eThanks->setHtml(in.readAll());
}

EdyukAboutDialog::~EdyukAboutDialog()
{
	;
}

/*! @} */
