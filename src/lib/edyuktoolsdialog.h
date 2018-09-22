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

#ifndef _EDYUK_TOOLS_DIALOG_H_
#define _EDYUK_TOOLS_DIALOG_H_

#include "edyuk.h"

#include "ui_toolsdialog.h"

/*!
	\file edyuktoolsdialog.h
	\brief Definition of the EdyukToolsDialog class
	
	\see EdyukToolsDialog
*/

class EdyukToolsManager;

class EdyukToolsDialog : public QDialog, private Ui::ToolsDialog
{
	Q_OBJECT
	
	public:
		EdyukToolsDialog(EdyukToolsManager *m, QWidget *p = 0);
		
		void exec();
		void retranslate();
		
	private slots:
		void on_lwTools_currentRowChanged(int i);
		
		void on_bNew_clicked();
		void on_bDelete_clicked();
		
		void on_bUp_clicked();
		void on_bDown_clicked();
		
		void on_leCaption_editingFinished();
		void on_leProg_editingFinished();
		void on_leProg_textChanged();
		void on_lePWD_editingFinished();
		void on_lePWD_textChanged();
		void on_leArgs_editingFinished();
		
		void on_tbSelectProgram_clicked();
		void on_tbSelectWorkingDir_clicked();
		
	private:
		EdyukToolsManager *pManager;
};

#endif // _EDYUK_TOOLS_DIALOG_H_
