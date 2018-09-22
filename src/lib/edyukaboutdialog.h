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

#ifndef _EDYUK_ABOUT_DIALOG_H_
#define _EDYUK_ABOUT_DIALOG_H_

#include "edyuk.h"

#include "ui_aboutdialog.h"

/*!
	\file edyukaboutdialog.h
	\brief Definition of the EdyukAboutDialog class
	
	\see EdyukAboutDialog
*/

class EdyukAboutDialog : public QDialog, private Ui::AboutDialog
{
	Q_OBJECT
	
	public:
		EdyukAboutDialog(QWidget *w = 0);
		~EdyukAboutDialog();
		
	private:
		
};

#endif // !_EDYUK_ABOUT_DIALOG_H_
