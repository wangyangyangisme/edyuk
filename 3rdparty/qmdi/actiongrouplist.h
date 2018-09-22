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

#ifndef _QMDI_ACTION_GROUP_LIST_H_
#define _QMDI_ACTION_GROUP_LIST_H_

/*!
	\file actiongrouplist.h
	\brief Definition of the action group list class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiActionGroupList
*/

#include "qmdi.h"

#include "actiongroup.h"

class QMDI_API qmdiActionGroupList
{
	friend class qmdiClient;
	
	public:
		qmdiActionGroupList();
		~qmdiActionGroupList();
		
		void clear();
		
		void setTranslation(const QString& s, const QString& ts);
		
		qmdiActionGroup* operator[]( const QString& name );
		qmdiActionGroup* getActionGroup( const QString& name );
		void mergeGroupList( qmdiActionGroupList *group );
		void unmergeGroupList( qmdiActionGroupList *group );
		
		void updateMenu( QMenuBar *menubar );
		void updateToolBar( QList<QToolBar*>& toolbars, QMainWindow *window );
		
	private:
		QHash<QString, QString> translations;
		QList<qmdiActionGroup*> actionGroups;
};

#endif // _QMDI_ACTION_GROUP_LIST_H_
