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

#include "actiongroup.h"

/*!
	\file actiongroup.cpp
	\brief Implementation of the action group class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiActionGroup
*/

#include <qmenu.h>
#include <qaction.h>
#include <qwidget.h>
#include <qtoolbar.h>

/*!
	\ingroup mdi
	@{
	
	\class qmdiActionGroup
	\brief an abstraction layer for QMenu and QToolBar
	
	This class defines the items that you see on a QMenu and
	QToolBar, with a much simplified interface. This class has
	the ability to merge two menus, and thus allowing the new menu
	to overwrite the actions of the orignal one.
	
	The action group has a name, which will be used for creating a
	popup menu on a QMenuBar, or setting the toolbar name.
	
	\see qmdiActionGroupList
	\see name
	
	Edyuk :
		This class now features a "modification state" system to reduce the
		flicker when updating through qmdiHost.
		Moreover,it is now possible to add widgets to toolbars, as with regular
		QToolBar objects.
*/

/*!
	Empty constructor.
*/
qmdiActionGroup::qmdiActionGroup()
 : bMod(false)
{
}

/*!
	Default constructor. Builds a new qmdiActionGroup
	with a given name. The action group will contain no 
	actions by default, representing an empty menu or toolbar.
*/
qmdiActionGroup::qmdiActionGroup(const QString& name)
 : bMod(false)
{
	setName(name);
}


/*!
	Empty destructor
*/
qmdiActionGroup::~qmdiActionGroup()
{
	clear();
}

/*!
	\brief Clear the action group of every actions and widget

*/
void qmdiActionGroup::clear()
{
	//foreach ( QObject *o, actionGroupItems )
	//	if ( !o->parent() )
	//		delete o;
	
	actionGroupItems.clear();
}


/*!
	\brief returns the name of the action group
	\return the name of the action group
	
	Gets the name for the action group. The name will be used
	for describing the toolbar or menu item, and thus is very
	important to set it correctly.
*/
QString qmdiActionGroup::name() const
{
	return sName;
}

/*!
	\brief sets an name for this action group
	\param name the new name for the action group
	
	Sets the name for the action group. The name will be used
	as object name of the menu/toolbar and thus in QMainWindow::saveState()
	and QMainWindow::restoreState()
	
	\see getName
*/
void qmdiActionGroup::setName(const QString& name)
{
	sName = name;
}

/*!
	\brief returns modification state of the action group
	\return modification state
	
	Gets the modification state of the action group. This
	is especially useful when updating GUI to reduce flicker.
*/
bool qmdiActionGroup::isModified() const
{
	return bMod;
}

/*!
	\overload
	
	Same as insertSeparator(size());
	
	\see insertSeparator
*/
void qmdiActionGroup::addSeparator()
{
	insertSeparator(count());
}

/*!
	\overload
	
	Same as insertWidget(\a widget, size());
	WidgetAction
*/
void qmdiActionGroup::addWidget(QWidget *widget)
{
	insertWidget(widget, count());
}

/*!
	\overload
	
	Same as insertAction(\a action, size());
	
	\see insertAction
*/
void qmdiActionGroup::addAction( QAction *action )
{
	insertAction(action, count());
}

/*!
	\brief Inserts a separator into the action
	
	\note A separator is a special action ( \see QAction::setSeparator(bool) ).
	
	\see insertAction
	\see removeAction
*/
void qmdiActionGroup::insertSeparator(int i)
{
	QAction *separator = new QAction(0);
	separator->setSeparator(true);
	
	insertAction(separator, i);
}

/*!
	\brief inserts a widget into the toolbar
	
	This function will add a widget to the
	toolbar represented by this action group.
	
	\see containsWidget
	\see removeWidget
	\see addWidget
*/
void qmdiActionGroup::insertWidget(QWidget *widget, int i)
{
	if ( containsWidget(widget) )
		return;
	
	actionGroupItems.insert(i, widget);
	bMod = true;
}

/*!
	\brief inserts a new action into the action group
	\param action item to be added to the action group
	\param i position of the new item in the action group
	
	When calling this function, you are adding a new 
	item to the toolbar or menu represented by the action
	group.
	
	Actions are added to the end of the list. There is no way
	to reorder the actions once they are in the group.
	
	\see insertSeparator
	\see containsAction
	\see removeAction
*/
void qmdiActionGroup::insertAction(QAction *action, int i)
{
	if ( containsAction(action) )
		return;
	
	actionGroupItems.insert(i, action);
	bMod = true;
}

/*!
	\brief remove an action from the action group
	\param action QAction item to be removed
	
	Use this function for removing items from the menu or 
	toolbar reporesented by this action group.
	
	\see addAction
*/
void qmdiActionGroup::removeAction( QAction *action )
{
	actionGroupItems.removeAll(action);
	bMod = true;
}

/*!
	\brief remove a widget from the action group
	\param action QAction item to be removed
	
	Use this function for removing items from the
	toolbar reporesented by this action group.
	
	\see addWidget
*/
void qmdiActionGroup::removeWidget(QWidget *widget)
{
	actionGroupItems.removeAll(widget);
	bMod = true;
}

/*!
	\return The number of actions/widgets in the action group
*/
int qmdiActionGroup::count() const
{
	return actionGroupItems.count();
}

/*!
	\return the position of an action/widget in the action group
*/
int qmdiActionGroup::indexOf(QObject *o) const
{
	return actionGroupItems.indexOf(o);
}

/*!
	\brief returns if an action is found in this group
	\param action QAction to be tested
	\return true if the action is found in this group action
	
	Use this function for testing if some action is found on
	the action group.
*/
bool qmdiActionGroup::containsAction(QAction *action) const
{
	return actionGroupItems.contains(action);
}

/*!
	\brief returns if a widget is found in this group
	\param action QWidget to be tested
	\return true if the widget is found in this group action
	
	Use this function for testing if some widget is found on
	the action group.
*/
bool qmdiActionGroup::containsWidget(QWidget *widget) const
{
	return actionGroupItems.contains(widget);
}

/*!
	\brief merges another action group actions into this action group
	\param group the new group to be merged
	
	Use this call if you want to merge the items of another group into
	one. The actions of the new group will be placed at the end of the
	list of actions available on this
	
	\see unmergeGroup
*/
void qmdiActionGroup::mergeGroup(qmdiActionGroup *group)
{
	if ( !group )
		return;
	
	int i = 0;
	
	foreach( QObject *o, group->actionGroupItems )
	{
		if ( !actionGroupItems.contains(o) )
		{
			++i;
			actionGroupItems << o;
		}
	}
	
	bMod |= (bool)i;
}


/*!
	\brief unmerges another action group actions into this action group
	\param group the group to be removed from this group
	
	Use this call if you want to unmerge the items of another group into
	one.
	
	\see mergeGroup
*/
void qmdiActionGroup::unmergeGroup(qmdiActionGroup *group)
{
	if ( !group )
		return;
	
	int i = 0;
	
	foreach( QObject *o, group->actionGroupItems )
		i += actionGroupItems.removeAll(o);
	
	bMod |= (bool)i;
}

/*!
	\brief generates an updated menu from the items on the group list
	\param menu a
	\return an updated menu
	
	Call this function to update a QMenu from these definitions.
	If \param menu is \b NULL then a new menu will be allocated.
	
	The returned value is not unallocated by this function, and it's
	up to the programmer to unallocate the memory used by the created menu.
	
	If you are inserting that QMenu into a QMenuBar the memory deallocation
	will be handeled by QMenuBar, and you don't have to bother about it.
	
	If the action group contains no items, no menu will be generated, and 
	NULL will be the returned value. If the passed \param menu is not NULL
	it will be deallocated.
	
	\see updateToolBar
*/
QMenu* qmdiActionGroup::updateMenu()
{
	//qDebug("updating menu %s", qPrintable(sName));
	
	actionGroupItems.removeAll(0);
	
	if ( actionGroupItems.isEmpty() )
		return 0;
	
	QMenu *menu = new QMenu(sName);
	
	foreach( QObject *o, actionGroupItems )
	{
		if ( !o )
		{
			//qDebug("invalid object in menu...");
			continue;
		}
		
		QAction *a = qobject_cast<QAction*> (o);
		
		if ( a )
		{
			menu->addAction(a);
		} else
			qWarning("Invalid object in menu : %s", qPrintable(sName));
			
	}

	return menu;
}


/*!
	\brief generates an updated toolbar from the items on the group list
	\param toolbar the toolbar to update
	\return an updated toolbar
	
	Call this function to update a QToolBar from these definitions.
	If \param toolbar is \b NULL then a new toolbar will be allocated.
	
	The returned value is not unallocated by this function, and it's
	up to the programmer to unallocate the memory used by the created menu.
	
	If you are inserting that QToolBar into a QMainWindow the memory deallocation
	will be handled by QMainWindow, and you don't have to bother about it.
	
	\see updateMenu
*/
void qmdiActionGroup::updateToolBar(QToolBar *toolbar)
{
	if ( !toolbar )
		return;
	
	bool updatesEnabled = toolbar->updatesEnabled();
	
	if ( updatesEnabled )
		toolbar->setUpdatesEnabled(false);
	
	toolbar->hide();
	toolbar->clear();
	toolbar->setObjectName(sName);
	
	foreach( QObject *o, actionGroupItems )
	{
		if ( !o )
			continue;
		
		QWidget *w = qobject_cast<QWidget*>(o);
		
		if ( w )
		{
			toolbar->addWidget(w)->setVisible(true);
		} else {
			QAction *a = qobject_cast<QAction*>(o);
			
			if ( a )
			{
				toolbar->addAction(a);
				a->setVisible(true);
			}
		}
	}
	
	
	if ( actionGroupItems.count() )
		toolbar->show();
	else
		toolbar->hide();
	
	if ( updatesEnabled )
		toolbar->setUpdatesEnabled(true);
	
	bMod = false;
}

/*! @} */
