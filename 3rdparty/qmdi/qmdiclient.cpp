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
	\file qmdiclient.cpp
	\brief Implementation of the qmdi client class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiClient
*/

#include "qmdiclient.h"
#include "qmdiserver.h"
#include "qmdiperspective.h"

#include <QAction>
#include <QFileInfo>

/*!
	\class qmdiClient
	\brief interface for menus and toolbars
	
	If you want to use qmdilib, the new widgets which will define
	their own menus and toolbars must be derived from this class, either
	directly or indirectly (through qmdiWidget).
	MDI stands for Multiple Document Interface. Thus each client is more or
	less meant to hold a "document". As MDI is an "interface" between the
	user an these "documents" clients must provides means to interact with
	their documents in a generic way and that is the primary purpose of
	qmdiClient, dynamic menu creation being a secondary one.
	
	\code
	class newclient: public QTextEdit, public qmdiClient
	{
		...
	};
	\endcode
	
	You will have also to insert this mdi client into an mdi server such as
	qmdiWorkspace.
	
	Defining menus and toolbars is easy:
	
	\code
	menus["&File"]->addAction( actionNew );
	menus["&File"]->addSeparator();
	menus["&File"]->addAction( actionQuit );
	
	toolbars["General"]->addAction( actionNew );
	\endcode
	
	The menus and toolbars will be merged automatically for you.
	Despite its rather convenient interface, qmdiclient is a low-level class,
	use qmdiWidget instead, it already inherits from QWidget and features an
	automatic MDI's initalization and cleanup system.
	
	\see qmdiActionGroupList
	\see qmdiWidget
	\see qmdiWorkspace
*/

qmdiClient::qmdiClient(qmdiServer *s)
 : bMod(false), bPrint(false), pServer(s), pPerspective(0)
{
}

/*!
	Empty destructor neede by virtual functions
*/
qmdiClient::~qmdiClient()
{
	menus.clear();
	toolbars.clear();
}


/*!
	\return wether the "document" owned by this client can be printed
	
	This function shall return true if print() is implemented.
	
	\see setPrintable()
*/
bool qmdiClient::isPrintable() const
{
	return bPrint;
}

/*!
	\return the name of the client
	
	\see fileName()
*/
QString qmdiClient::name() const
{
	return sName;
}

/*!
	\return the file name corresponding to the content of the client
	
	\see name()
	\see setFileName()
*/
QString qmdiClient::fileName() const
{
	return sFileName;
}

/*!
	\return wether the "document" owned by this client has been modified
	
	\see setContentModified()
*/
bool qmdiClient::isContentModified() const
{
	return bMod;
}

/*!
	\return The mdi server to which this client belongs
*/
qmdiServer* qmdiClient::server() const
{
	return pServer;
}

/*!
	\brief Sets the mdi server to which this client belongs
	
	If the client is already present in another server, it removes
	iself from this old server.
*/
void qmdiClient::setServer(qmdiServer *s)
{
	if ( pServer && pServer != s )
		notifyDeletion();
	
	pServer = s;
}

/*!
	\brief save function
	
	Each subclass of qmdiClient should reimplement it so as to save their
	internal "document"
	
	Note : As for most of qmdiServer functions, this should be pure virtual
	but if the end coder doesn't want to provide it he won't need to create
	stub functions...
	
	\see isContentModified()
*/
void qmdiClient::save()
{
	;
}

/*!
	\brief print function
	
	Each subclass of qmdiClient should reimplement it so as to print their
	internal "document". If this function is reimplemented, isPrintable() shall
	return true.
	
	Note : As for most of qmdiServer functions, this should be pure virtual
	but if the end coder doesn't want to provide it he won't need to create
	stub functions...
	
	\see isPrintable()
*/
void qmdiClient::print()
{
	;
}

/*!
	\brief This function is meant to provide a run-time translation framework.
	
	It has a similar role to the retranslateUi() function provided by all ui
	classes created by uic.
*/
void qmdiClient::retranslate()
{
	;
}

/*!
	\brief utility function to remove a client from its server
*/
void qmdiClient::notifyDeletion()
{
	if ( pServer )
	{
		pServer->clientDeleted(dynamic_cast<QObject*>(this));
		pServer = 0;
	}
}

/*!
	\brief Sets the client as "printable"
	
	\see isPrintable()
*/
void qmdiClient::setPrintable(bool y)
{
	bPrint = y;
}

/*!
	\brief Sets the client as "modified"
	
	\see isContentModified()
*/
void qmdiClient::setContentModified(bool y)
{
	bMod = y;
}

/*!
	\brief Sets the client as filename as \arg f
	
	Note : Name is set as QFileInfo(f).fileName()
	
	\see name()
	\see fileName()
*/
void qmdiClient::setFileName(const QString& f)
{
	sFileName = f;
	
	sName = QFileInfo(f).fileName();
}

/*!
	\return The perspective owning the client
	
*/
qmdiPerspective* qmdiClient::perspective() const
{
	return pPerspective;
}

/*!
	\brief Sets the perspective owning the client
	
*/
void qmdiClient::setPerspective(qmdiPerspective *p)
{
	pPerspective = p;
}

/*!
	\return All the actions provided by the client as menu items
*/
QList<QAction*> qmdiClient::actions() const
{
	QList<QAction*> l;
	
	foreach ( qmdiActionGroup *g, menus.actionGroups )
	{
		foreach ( QObject *o, g->actionGroupItems )
		{
			QAction *a = qobject_cast<QAction*>(o);
			
			if ( a && !a->isSeparator() )
				l << a;
		}
	}
	
	return l;
}
