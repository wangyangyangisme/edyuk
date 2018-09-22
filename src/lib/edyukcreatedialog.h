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

#ifndef _EDYUK_CREATE_DIALOG_H_
#define _EDYUK_CREATE_DIALOG_H_

#include "edyuk.h"

#include "ui_createnewdialog.h"

/*!
	\file edyukcreatedialog.h
	\brief Definition of the EdyukCreateDialog class.
	
	\see EdyukCreateDialog
*/

#include <QHash>

struct EdyukTemplate;
class QListWidgetItem;

class EdyukTemplateManager;

class EdyukCreateDialog : public QDialog, private Ui::CreateNewDialog
{
	Q_OBJECT
	
	public:
		enum Filter
		{
			Project,
			File,
			Extra,
			All
		};
		
		EdyukCreateDialog(EdyukTemplateManager *m, QWidget *p = 0);
		virtual ~EdyukCreateDialog();
		
		Filter filter() const;
		void setFilter(Filter f);
		
	protected:
		virtual void showEvent(QShowEvent *e);
		
	private slots:
		void creation();
		
		void on_tbLocation_clicked();
		void on_cbFilter_currentIndexChanged(const QString& s);
		void on_lwTemplates_itemClicked(QListWidgetItem*);
		
	private:
		Filter m_filter;
		EdyukTemplateManager *pTemplates;
		QHash<QListWidgetItem*, EdyukTemplate> m_links;
};

#endif // !_EDYUK_CREATE_DIALOG_H_
