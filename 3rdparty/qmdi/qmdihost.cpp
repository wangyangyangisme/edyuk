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

/*!
	\file qmdihost.cpp
	\brief Implemetation of the qmdi host class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiHost
*/

#include "qmdihost.h"

#include "qmdiclient.h"

#include <QMenuBar>
#include <QMainWindow>

/*!
	\class qmdiHost
	\brief Host class of the mdi layer
	
	qmdiHost is in charge of appliance of dynamic menus and toolbars to main
	window. This is done by merging one or more client to it, a task usually
	performed by a qmdiServer, and then calling updateGUI, passing the target
	window as parameter.
	
	As qmdiHost is the only application-wide class that MUST be subclassed,
	unlike qmdiServer which already has a convinience class : qmdiWorkspace,
	it is used to provide all application-specific data.
	
	\see qmdiServer

*/

/*!
	\brief ctor
*/
qmdiHost::qmdiHost()
 : m_locked(false)
{
	
}

/*!
	\brief dtor
*/
qmdiHost::~qmdiHost()
{
	menus.clear();
	toolbars.clear();
}

/*!
	\brief update the toolbars and menus
	\param window the window to update
	
	This function generates the menubar and toolbars from
	the toolbar and menus definitions. 
	
	You should call this method after every time you modify the menus
	or structures.
	
	The parameter window should be generally \b this , it's passed on
	as a parameter, since qmdiHost cannot dynamic_cast it self to an 
	QObject (this just does not work). On the other hand, this can give
	you more freedom, as you do not have to derive the main window 
	also from qmdiHost, and the host can be a separate object.
	
	\see qmdiActionGroupList
*/
void qmdiHost::updateGUI(QMainWindow *window)
{
	if ( m_locked )
		return;
	
	if ( !window )
	{
		qDebug("%s - warning, no main window specified", __FUNCTION__ );
		return;
	}
	
	bool bEnabled = window->updatesEnabled();
	
	if ( bEnabled )
	{
		window->setUpdatesEnabled(false);
		window->menuBar()->setUpdatesEnabled(false);
	}
	
	toolbars.updateToolBar(toolBarList, window);
	menus.updateMenu(window->menuBar());
	
	if ( bEnabled )
	{
		window->setUpdatesEnabled(true);
		window->menuBar()->setUpdatesEnabled(true);
	}
}

/*!
	\brief merges the mdi client \a client to the host
*/
void qmdiHost::mergeClient(qmdiClient *client)
{
	if ( !client )
		return;
	
	menus.mergeGroupList( &client->menus );
	toolbars.mergeGroupList( &client->toolbars );
}

/*!
	\brief unmerges the mdi client \a client from the host
*/
void qmdiHost::unmergeClient(qmdiClient *client)
{
	if ( !client )
		return;
	
	menus.unmergeGroupList( &client->menus );
	toolbars.unmergeGroupList( &client->toolbars );
}

/*!
	\brief Creates an empty client
	
	This method, meant to be reimplemented, is mainly used by qmdiWorkspace.
	
	\note The default implementation returns a null pointer.
*/
qmdiClient* qmdiHost::createEmptyClient()
{
	return 0;
}

/*!
	\brief Handler used by servers to notify the removal/deletion of a client
	
	\note The default implementation does nothing
 */
void qmdiHost::clientOpened(qmdiClient *c)
{
	Q_UNUSED(c)
}

/*!
	\brief Handler used by servers to notify the removal/deletion of a client
	
	\note The default implementation does nothing
 */
void qmdiHost::clientClosed(qmdiClient *c)
{
	Q_UNUSED(c)
}

/*!
	\brief Handler used by servers to notify a change of the current client
	
	\note The default implementation does nothing
*/
void qmdiHost::currentClientChanged(qmdiClient *c)
{
	Q_UNUSED(c)
}

/*!
	\brief gives file filters supported by the application
	\return a valid set of filters (suitable for QFileDialog...)
	
	This utility function is mainly used by qmdiWorkspace when saving a client
	under a new name.
*/
QString qmdiHost::filters() const
{
	QStringList l;
	
	l << "All files ( * )";
	
	return l.join(";;");
}
