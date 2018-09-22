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

#ifndef _QMDI_CLIENT_FACTORY_H_
#define _QMDI_CLIENT_FACTORY_H_

/*!
	\class qmdiClientFactory
	
	\brief Abstract class meant to provide easy inter-operability
	with 3rdparty libraries such as QCodeEdit.
	
	Such an inter-operability can be achieved by subclassing qmdiClientFactory
	an implementing custom client widgets (for instance, QCodeEdit provides
	qmdiClient-based widgets when qmdilib is present)
*/

#include <QObject>

class qmdiClient;

class qmdiClientFactory : public QObject
{
	Q_OBJECT
	
	public:
		qmdiClientFactory(QObject *p = 0) : QObject(p) {}
		virtual ~qmdiClientFactory() {}
		
		virtual qmdiClient* createClient(const QString& filename) const = 0;
		
	signals:
		void clientCreated(qmdiClient *client);
};

#endif // _QMDI_CLIENT_FACTORY_H_
