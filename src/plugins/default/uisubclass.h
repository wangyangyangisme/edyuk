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

#ifndef _UI_SUBCLASS_H_
#define _UI_SUBCLASS_H_

#include <QDialog>
#include "ui_subclassing.h"

class FormSignalsModel;
class QSortFilterProxyModel;

class UiSubclass : public QDialog, private Ui::Subclassing
{
	Q_OBJECT
	
	public:
		UiSubclass(const QString& form, QWidget *p = 0);
		
		QStringList createdFiles() const;
		
	public slots:
		void on_buttonBox_accepted();
		
		void on_leClassName_textChanged();
		void on_leFilter_textChanged();
		
	private:
		QStringList m_files;
		FormSignalsModel *m_model;
		QSortFilterProxyModel *m_proxy;
};

#endif
