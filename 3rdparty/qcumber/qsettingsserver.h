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

#ifndef _QSETTINGS_SERVER_H_
#define _QSETTINGS_SERVER_H_

#include "qcumber.h"

/*!
	\file qsettingsserver.h
	
	\brief Definition of the QSettingsServer class.
*/

#include <QObject>
#include <QSettings>
#include <QReadWriteLock>

class QSettingsClient;

class QSettingsWatcher
{
	public:
		virtual ~QSettingsWatcher() {}
		
		virtual bool isWorthCatching(const QString& key) const = 0;
		virtual void changed(const QString& key, const QVariant& value) = 0;
};

class QCUMBER_EXPORT QSettingsServer : public QObject
{
	public:
		QSettingsServer(QObject *p = 0);
		QSettingsServer(const QString& fn, QObject *p = 0);
		QSettingsServer(const QString& fn, QSettings::Format f, QObject * p = 0);
		QSettingsServer(QSettings::Format f, QSettings::Scope s,
						const QString& org, const QString& app = QString(),
						QObject * p = 0);
		
		virtual ~QSettingsServer();
		
		void addWatcher(QSettingsWatcher *w);
		void removeWatcher(QSettingsWatcher *w);
		
		void remove(const QString& key);
		bool contains(const QString& key) const;
		
		QStringList allKeys() const;
		
		QStringList allKeys(const QString& g) const;
		QStringList childKeys(const QString& g) const;
		QStringList childGroups(const QString& g) const;
		
		void setValue(const QString& key, const QVariant& val);
		QVariant value(const QString& key, const QVariant& val = QVariant()) const;
		
		virtual void setDefault();
		
	protected:
		inline QSettings* settings() { return pSettings; }
		
	private:
		void dispatch(const QString& key, const QVariant& v);
		
		QSettings *pSettings;
		QList<QSettingsWatcher*> m_watchers;
		
		mutable QReadWriteLock m_lock;
};

#endif // _QSETTINGS_SERVER_H_
