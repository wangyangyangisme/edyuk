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

#include "qsettingsserver.h"

/*!
	\file qsettingsserver.cpp
	
	\brief Implementation of the QSettingsServer class.
*/

#include <QFile>
#include <QSettings>
#include <QStringList>

QSettingsServer::QSettingsServer(QObject *p)
 : QObject(p)
{
	pSettings = new QSettings(this);
}

QSettingsServer::QSettingsServer(const QString& fn, QObject *p)
 : QObject(p)
{
	if ( !QFile::exists(fn) )
	{
		QFile f(fn);
		f.open(QFile::WriteOnly | QFile::Text);
	}

	pSettings = new QSettings(fn, QSettings::IniFormat, this);
}

QSettingsServer::QSettingsServer(const QString& fn, QSettings::Format f,
								QObject *p)
 : QObject(p)
{
	pSettings = new QSettings(fn, f, this);
}

QSettingsServer::QSettingsServer(QSettings::Format f, QSettings::Scope s,
								const QString& org, const QString& app,
								QObject *p)
 : QObject(p)
{
	pSettings = new QSettings(f, s, org, app, this);
}

QSettingsServer::~QSettingsServer()
{
	qDeleteAll(m_watchers);
}

void QSettingsServer::addWatcher(QSettingsWatcher *w)
{
	if ( !w )
		return;
	
	m_watchers << w;
}

void QSettingsServer::removeWatcher(QSettingsWatcher *w)
{
	m_watchers.removeAll(w);
}

void QSettingsServer::setDefault()
{
	QWriteLocker locker(&m_lock);
	
	pSettings->clear();
}

void QSettingsServer::remove(const QString& key)
{
	QWriteLocker locker(&m_lock);
	
	pSettings->remove(key);
	
	dispatch(key, QVariant());
}

bool QSettingsServer::contains(const QString& key) const
{
	QReadLocker locker(&m_lock);
	
	return pSettings->contains(key);
}

QStringList QSettingsServer::allKeys() const
{
	QReadLocker locker(&m_lock);
	
	return pSettings->allKeys();
}

QStringList QSettingsServer::allKeys(const QString& g) const
{
	QReadLocker locker(&m_lock);
	
	QStringList l;
	
	pSettings->beginGroup(g);
	l = pSettings->allKeys();
	pSettings->endGroup();
	
	return l;
}

QStringList QSettingsServer::childKeys(const QString& g) const
{
	QReadLocker locker(&m_lock);
	
	QStringList l;
	
	pSettings->beginGroup(g);
	l = pSettings->childKeys();
	pSettings->endGroup();
	
	return l;
}

QStringList QSettingsServer::childGroups(const QString& g) const
{
	QReadLocker locker(&m_lock);
	
	QStringList l;
	
	pSettings->beginGroup(g);
	l = pSettings->childGroups();
	pSettings->endGroup();
	
	return l;
}

void QSettingsServer::setValue(const QString& key, const QVariant& val)
{
	QWriteLocker locker(&m_lock);
	
	pSettings->setValue(key, val);
	
	dispatch(key, val);
}

QVariant QSettingsServer::value(const QString& key, const QVariant& val) const
{
	QReadLocker locker(&m_lock);
	
	return pSettings->value(key, val);
}

void QSettingsServer::dispatch(const QString& key, const QVariant& v)
{
	foreach ( QSettingsWatcher *w, m_watchers )
	{
		if ( w->isWorthCatching(key) )
			w->changed(key, v);
	}
}
