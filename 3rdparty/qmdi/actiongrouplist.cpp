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

#include "actiongrouplist.h"

/*!
	\file actiongrouplist.cpp
	\brief Implementation of the action group list class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiActionGroupList
 */

#include "actiongroup.h"

#include <qobject.h>
#include <qstring.h>
#include <qmenubar.h>
#include <qtoolbar.h>
#include <qmainwindow.h>

/*!
	\class qmdiActionGroupList
	\brief abstraction layer for QMenuBar and a list of toolbars
	
	This class defines a QMenuBar and the list of toolbars available on
	a typical application. Each submenu or toolbar is defined by one 
	qmdiActionGroup. 
	
	This class has also the ability to merge other qmdiActionGroupList (this
	enables widgets to add their partial menus to the menus supplied by the
	main application).
	
	  Edyuk :
		The updaters no longer return anything because it was an waste of time
		and memory. Indeed, pointers and reference allow modifications...
*/


/*!
	Build an empty action group list. If you generate a menubar 
	from this empty class, you will get a NULL menu. Generating
	a toolbar set from this empty class will generate no toolbars.
*/
qmdiActionGroupList::qmdiActionGroupList()
{
	
}

/*!
	Destructor
*/
qmdiActionGroupList::~qmdiActionGroupList()
{
	clear();
}

/*!
	Clear the list of action groups.
	
	Warning : this function destroys all the owned action groups but not
	the actions they own themselves.
*/
void qmdiActionGroupList::clear()
{
	qDeleteAll(actionGroups);
	actionGroups.clear();
}

/*!
	\param s Menu/toolbar name to retranslate
	\param ts Translation of the given menu/toolbar name
	
	Sets the translation of a given menu/toolbar name to \arg ts
*/
void qmdiActionGroupList::setTranslation(const QString& s, const QString& ts)
{
	translations[s] = ts;
}

/*!
	\brief overloaded operator for getting the instance of a action group
	\param name the action group name you want to get
	\return an instace to an action group

 	This is just an overloaded function which calls getActionGroup().
	
	\see getActionGroup()
*/
qmdiActionGroup* qmdiActionGroupList::operator[]( const QString& name )
{
	return getActionGroup( name );
}


/*!
	\brief get the instance of a action group
	\param name the action group name you want to get
	\return an instace to an action group
	
	This function returns an instace to a action group. Action groups
	are abstractions of QMenu and QToolBar.
	
	If the action group requested is not available, a new instace will be
	created.
	
	\see updateMenu()
	\see updateToolBar()
*/
qmdiActionGroup* qmdiActionGroupList::getActionGroup( const QString& name )
{
	qmdiActionGroup *item = NULL;

	foreach( qmdiActionGroup* i, actionGroups )
	{
		if ( i->name() == name )
			return i;
	}
	
	// if menu does not exist, create it
	item = new qmdiActionGroup( name );
	actionGroups.append( item );
	return item;
}

/*!
	\brief merge another action group list
	\param group the new group to merge into this one
	
	This function merges an action group list definition into this
	action group list:
		- If in the new group there are action groups, the items will be
		appended to the existing ones
		- If in the new group there are new actions groups, those groups
		will be added to this action group list
	
	Note that just merging is not enough, and you might need also to update 
	the real widget which this action group list represents.
	
	\see unmergeGroupList
	\see updateMenu
	\see updateToolBar
*/
void qmdiActionGroupList::mergeGroupList( qmdiActionGroupList *group )
{
	foreach( qmdiActionGroup* i, group->actionGroups )
	{
		qmdiActionGroup *mine = getActionGroup( i->name() );
		mine->mergeGroup( i );
	}
}


/*!
	\brief unmerge an action group list
	\param group the old group to remove from this action group list
	
	This function removes external definitions from this action group list.
	If at the end of the unmerge, some action groups are empty, \b they \b will
	\b not \b be \b removed \b from \b this \b class. Since the generation of
	menus (using updateMenu() ) does not include empty menus, this is totally
	accepatable.
	
	Note that just unmerging an action group list will not totally reflect the
	GUI, and you might also need to update the real widget which this action
	group list represents.
	
	\see mergeGroupList
	\see updateMenu
	\see updateToolBar
*/
void qmdiActionGroupList::unmergeGroupList( qmdiActionGroupList *group )
{
	foreach( qmdiActionGroup* i, group->actionGroups )
	{
		qmdiActionGroup *mine = getActionGroup( i->name() );
		mine->unmergeGroup( i );
	}
}


/*!
	\brief update a QMenuBar from the definitions on this action group list
	\param menubar a QMenuBar to be updated
	\return the updated menubar (same instace which was passed)
	
	This function generates from the definitions on this class a valid
	QMenuBar which will be showed on a QMainWindow. 
	
	If \c menubar is NULL, a new QMenuBar will be allocated for you, and
	will be returned.
	
	You cannot generate items into a QMenuBar "by hand" and then "add"
	the definitions on this class. 
*/
void qmdiActionGroupList::updateMenu(QMenuBar *menubar)
{
	if ( !menubar )
		return;
	
	/*
	QList<QAction*> l = menubar->actions();
	
	foreach ( QAction *a, l )
	{
		if ( a->menu() )
			delete a->menu();
		
	}
	*/
	
	menubar->clear();
	
	foreach( qmdiActionGroup* i, actionGroups )
	{
		QMenu *m = i->updateMenu();
		
		if ( m )
		{
			/*
				This allows run-tim translation...
			*/
			QString t = translations[i->name()];
			
			if ( t.count() )
				m->setTitle(t);
			
			menubar->addMenu(m);
		}
	}
}

/*!
	\brief update a list of QToolBars from the definitions on this action group list
	\param window the window in which the toolbars should be placed
	\return a list of toolbars which has been created from this action group list
	
	This function generates from the defintions on this class a valid list of
	QToolBar which will be showed on the \c window .
	
	If the \c toolbars array will be NULL, a new one will be allocated for you.
	
	While you can add toolbars "manually" to your main window, it's not
	recomended, because new actions will not get merged into your toolbar.
	Instead you might get 2 toolbars with a similar name.
*/
void qmdiActionGroupList::updateToolBar(QList<QToolBar*>& toolbars, QMainWindow *window)
{
	foreach( qmdiActionGroup* i, actionGroups )
	{
		QToolBar *tb = 0;
		QString  actionName = i->name();
		
		// find the correct toolbar
		foreach( QToolBar *b, toolbars )
		{
			if (b->objectName() == actionName)
			{
				tb = b;
				break;
			}
		}

		// if none found, create one
		if ( !tb )
		{
			tb = new QToolBar(actionName, window);
			tb->setObjectName(actionName);
			tb->hide();
			toolbars << tb;
			window->addToolBar(tb);
		}
		
		/*
			This allows run-tim translation...
		*/
		QString t = translations[actionName];
		
		if ( t.count() )
			tb->setWindowTitle(t);
		
		// merge it with the corresponding group list
		if ( i->isModified() )		//and the flicker died... ;-)
			i->updateToolBar(tb);
	}
}

