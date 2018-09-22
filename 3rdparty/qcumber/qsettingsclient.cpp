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

#include "qsettingsclient.h"

/*!
	\file qsettingsclient.cpp
	
	\brief Implementation of the QSettingsClient class.
*/

#include "qsettingsserver.h"

QSettingsClient::QSettingsClient(QSettingsServer *s, const QString& g)
 : pServer(s), base(g)
{
	
}

QSettingsClient::QSettingsClient(const QSettingsClient& o, const QString& g)
 : pServer(o.pServer), base(o.base + g)
{
	
}

QSettingsClient::~QSettingsClient()
{
	
}

QSettingsClient& QSettingsClient::operator = (const QSettingsClient& o)
{
	pServer = o.pServer;
	base = o.base;
	
	groups.clear();
	
	return *this;
}

void QSettingsClient::beginGroup(const QString& s)
{
	groups.push(s);
}

void QSettingsClient::endGroup()
{
	if ( groups.isEmpty() )
		return;
	
	QString g = groups.pop();
}

void QSettingsClient::clear()
{
	if ( pServer )
		pServer->remove(base);
}

void QSettingsClient::remove(const QString& key)
{
	if ( pServer )
		pServer->remove(group() + "/" + key);
}

bool QSettingsClient::contains(const QString& key) const
{
	return pServer ? pServer->contains(group() + "/" + key) : false;
}

QStringList QSettingsClient::allKeys() const
{
	return pServer ? pServer->allKeys(group()) : QStringList();
}

QStringList QSettingsClient::childKeys() const
{
	return pServer ? pServer->childKeys(group()) : QStringList();
}

QStringList QSettingsClient::childGroups() const
{
	return pServer ? pServer->childGroups(group()) : QStringList();
}

void QSettingsClient::setValue(const QString& key, const QVariant& val)
{
	if ( pServer )
		pServer->setValue(group() + "/" + key, val);
}

QVariant QSettingsClient::value(const QString& key, const QVariant& val) const
{
	return pServer ? pServer->value(group() + "/" + key, val) : val;
}

QString QSettingsClient::group() const
{
	QString gr = base;
	
	if ( !gr.isEmpty() )
		gr += "/";
	
	gr += QStringList(groups.toList()).join("/");
	
	return gr;
}
