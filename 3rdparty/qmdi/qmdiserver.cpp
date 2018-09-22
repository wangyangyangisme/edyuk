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
	\file qmdiserver.cpp
	\brief Implementation of the qmdi server class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiServer
*/
 
#include "qmdiserver.h"

#include "qmdihost.h"
#include "qmdiclient.h"


/*!
	\class qmdiServer
	\brief a default niterface for mdi servers
	
	This class is used only to get messages from the qmdiClient
	wanting to disconnect from the mdi layer. It also provide a
	convinience function for these client to determine wether
	the user want to save them.
	
	Most of the time you won't need to use this class. qmdiWorkspace
	is much simpler to use, brings a powerful GUI and already inherits
	from qmdiServer.
	
	Classes which derive this class, MUST implement the clientDeleted
	function.
*/
qmdiServer::qmdiServer(qmdiHost *h)
 : pHost(h)
{
}


/*!
	Empty destructor. Destroyes the object.
	
	Since this class needs to be dynamic_casted by the derived classes,
	to assign the correct qmdiSever, this class needs to have a vitrual table.
	If the class has a virtual function, it's only smart to have a virtual
	destructor.
	
	\see qmdiClient
	\see qmdiTabWidget
*/
qmdiServer::~qmdiServer()
{
	
}

/*!
	\return the host of this server
	
*/
qmdiHost* qmdiServer::host()
{
	return pHost;
}

/*!
	\brief save all clients
	
	Any subclass of qmdiServer should reimplement it...
*/
void qmdiServer::saveAll()
{
	
}
 
/*!
	\brief save current client
	
	Any subclass of qmdiServer should reimplement it...
*/
void qmdiServer::saveCurrent()
{
	
}

/*!
	\brief save current client as
	
	Any subclass of qmdiServer should reimplement it...
*/
void qmdiServer::saveCurrentAs()
{
	
}

/*!
	\brief close all clients
	
	Any subclass of qmdiServer should reimplement it...
*/
bool qmdiServer::closeAll()
{
	return true;
}

/*!
	\brief close current client
	
	Any subclass of qmdiServer should reimplement it...
*/
bool qmdiServer::closeCurrent()
{
	return true;
}

/*!
	\brief close current client
	
	Any subclass of qmdiServer should reimplement it...
*/
void qmdiServer::printCurrent()
{
	;
}

/*!
	\brief adds a client to a mdi server
	\param c client to add
	
	This function is stub as well and should be reimplemented. The default
	implementation just makes sure the client is in the correct server, this one.
*/
void qmdiServer::addClient(qmdiClient *c)
{
	if ( !c )
		return;
	
	c->setServer(this);
}

/*!
	\brief callback to get alarm of deleted object
	\param o the deleted object
	
	This function gets called on the destructor of qmdiClient,
	to announce that the object is about to be deleted. This
	function should be used to remove the menus and toolbars
	and other cleanup actions needed.
	
	Why not using the signal QObject::destroyed( QObject * obj = 0 ) ?
		- Because that signal is non blocking, and you will get yourself in
		race conditions: this function might be called after the object itself
		has been delete.
	
	This means that the qmdiClient needs to know the mdi server (usually a
	qmdiWorkspace) and ask to be removed before it gets destructed.
	
	Why this function is not pure virtual?
		- Since I found that it gives you warnings, about calling a pure virtual
		function, lame excuse, which I would like to get rid of :)
		
		- On some rare implementations the mdi server implemented, would like
		to ignore those events. I prefear that the dummy functions be implemented 
		by the library, and not by the end clients.
*/
void qmdiServer::clientDeleted( QObject * )
{
	// stub function
	// If not added, the function had to be pure virtual
}

/*!
	\brief function meant to ask the user if he wants to save changes in a client
	\param c client querying for save
	
	The default implementation automatically save the client, if not NULL.
	Any subclass of qmdiServer should reimplement it...
*/
bool qmdiServer::maybeSave(qmdiClient *c)
{
	if ( !c )
		return false;
	
	c->save();
	
	return true;
}

/*!
	\brief save a client under a new name
	\param c client querying for save
	
	The default implementation automatically does nothingt.
	Any subclass of qmdiServer should reimplement it...
*/
void qmdiServer::saveClientAs(qmdiClient *)
{
	
}
