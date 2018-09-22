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

#ifndef _QMDI_ACTION_GROUP_H_
#define _QMDI_ACTION_GROUP_H_

/*!
	\file actiongroup.h
	\brief Definition of the action group class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiActionGroup
*/
 
#include "qmdi.h"

#include <QPointer>

class QMDI_API qmdiActionGroup
{
	friend class qmdiClient;
	
	public:
		qmdiActionGroup();
		qmdiActionGroup(const QString& name);
		
		~qmdiActionGroup();
		
		void clear();
		
		QString	name() const;
		void setName(const QString& name);
		
		bool isModified() const;
		
		int count() const;
		int indexOf(QObject *o) const;
	
		void addSeparator();
		void addWidget(QWidget *widget);
		void addAction(QAction *action);
		
		void insertSeparator(int i);
		void insertWidget(QWidget *widget, int i);
		void insertAction(QAction *action, int i);
		
		void removeAction(QAction *action);
		void removeWidget(QWidget *widget);
		
		bool containsWidget(QWidget *widget) const;
		bool containsAction(QAction *action) const;
	
		void mergeGroup(qmdiActionGroup *group);
		void unmergeGroup(qmdiActionGroup *group);
		
		QMenu* updateMenu();
		void updateToolBar(QToolBar *toolbar);
		
	private:
		bool bMod;
		
		QString sName;
		QList< QPointer<QObject> > actionGroupItems;
};
#endif //_QMDI_ACTION_GROUP_
