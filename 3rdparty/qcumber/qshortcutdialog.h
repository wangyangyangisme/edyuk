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

#ifndef _QSHORTCUT_DIALOG_H_
#define _QSHORTCUT_DIALOG_H_

#include "qcumber.h"

#include "ui_shortcutdialog.h"

/*!
	\file qshortcutdialog.h
	\brief Definition of the QShortcutDialog class
	
	\see QShortcutDialog
*/

#include <QHash>
#include <QDialog>

class QTreeWidgetItem;
class QShortcutManager;

class QCUMBER_EXPORT QShortcutDialog : public QDialog, private Ui::ShortcutDialog
{
	Q_OBJECT
	
	public:
		QShortcutDialog(QShortcutManager *m, QWidget *p = 0);
		
		void retranslate();
		
		void exec();
		
	private slots:
		void on_twShortcuts_itemDoubleClicked(QTreeWidgetItem *i, int col);

	private:
		void updateAmbiguousList();
		
		QHash<QString, int> m_used;
		QShortcutManager *pManager;
};

#endif // _DEV_SHORTCUT_DIALOG_H_
