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

#ifndef _QSETTINGS_CLIENT_H_
#define _QSETTINGS_CLIENT_H_

#include "qcumber.h"

/*!
	\file qsettingsclient.h
	
	\brief Definition of the QSettingsClient class.
*/

#include <QStack>
#include <QObject>
#include <QVariant>
#include <QStringList>

class QSettingsServer;

class QCUMBER_EXPORT QSettingsClient
{
	public:
		QSettingsClient(QSettingsServer *s, const QString& g = QString());
		QSettingsClient(const QSettingsClient& o, const QString& g = QString());
		
		virtual ~QSettingsClient();
		
		QSettingsClient& operator = (const QSettingsClient& o);
		
		bool isValid() const { return server(); }
		
		void beginGroup(const QString& s);
		void endGroup();
		
		void clear();
		
		void remove(const QString& key);
		bool contains(const QString& key) const;
		
		QStringList allKeys() const;
		QStringList childKeys() const;
		QStringList childGroups() const;
		
		void setValue(const QString& key, const QVariant& val);
		QVariant value(const QString& key, const QVariant& val = QVariant()) const;
		
	protected:
		inline QSettingsServer* server() const { return pServer; }
		
	private:
		QString group() const;
		
		QSettingsServer *pServer;
		
		QString base;
		QStack<QString> groups;
};

#endif // _QSETTINGS_CLIENT_H_
