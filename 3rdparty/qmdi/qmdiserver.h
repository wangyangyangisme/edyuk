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

#ifndef _QMDI_SERVER_H_
#define _QMDI_SERVER_H_

/*!
	\file qmdiserver.h
	\brief Definition of the qmdi server class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiServer
*/

#include "qmdi.h"

class qmdiHost;
class qmdiClient;

class QMDI_API qmdiServer
{
	friend class qmdiPerspective;
	
	public:
		qmdiServer(qmdiHost *h = 0);
		virtual ~qmdiServer();
		
		qmdiHost* host();
		
		virtual void saveAll(); 
		virtual void saveCurrent();
		virtual void saveCurrentAs();
		
		virtual bool closeAll();
		virtual bool closeCurrent();
		
		virtual void printCurrent();
		
		virtual bool maybeSave(qmdiClient *c);
		virtual void saveClientAs(qmdiClient *c);
		
		virtual void addClient(qmdiClient *c);
		virtual void clientDeleted(QObject *o);
		
	private:
		qmdiHost *pHost;
};

#endif // _QMDI_SERVER_H_
