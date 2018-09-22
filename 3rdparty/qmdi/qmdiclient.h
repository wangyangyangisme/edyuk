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

#ifndef _QMDI_CLIENT_H_
#define _QMDI_CLIENT_H_

/*!
	\file qmdiclient.h
	\brief Definition of the qmdi client class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiClient
*/

#include "qmdi.h"

class qmdiHost;
class qmdiServer;
class qmdiPerspective;

#include "actiongrouplist.h"

class QMDI_API qmdiClient
{
	friend class qmdiHost;
	friend class qmdiServer;
	friend class qmdiWorkspace;
	
	public:
		qmdiClient(qmdiServer *s = 0);
		virtual ~qmdiClient();
		
		QString name() const;
		QString fileName() const;
		virtual void setFileName(const QString& f);
		
		bool isPrintable() const;
		bool isContentModified() const;
		
		qmdiServer* server() const;
		virtual void setServer(qmdiServer *s);
		
		qmdiPerspective* perspective() const;
		virtual void setPerspective(qmdiPerspective *p);
		
		virtual void save();
		virtual void print();
		
		virtual void retranslate();
		
		QList<QAction*> actions() const;
		
	protected:
		void notifyDeletion();
		
		virtual void setPrintable(bool y);
		virtual void setContentModified(bool y);
		
		qmdiActionGroupList toolbars;
		qmdiActionGroupList menus;
		
	private:
		bool bMod, bPrint;
		QString sName, sFileName;
		
		qmdiServer *pServer;
		qmdiPerspective *pPerspective;
};

#endif // _QMDI_CLIENT_H_
