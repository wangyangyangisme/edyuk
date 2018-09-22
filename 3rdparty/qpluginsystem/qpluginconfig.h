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

#ifndef _QPLUGIN_CONFIG_H_
#define _QPLUGIN_CONFIG_H_

#include "qpluginsystem.h"

#include <QHash>
#include <QDomElement>

class QString;
class QDateTime;
class QDomDocument;

class QPluginConfig
{
	public:
		class Entry
		{
			friend class QPluginConfigWidget;
			
			public:
				Entry();
				Entry(QPluginConfig *o, const QString& t, QDomElement e);
				~Entry();
				
				QPluginConfig* owner() const;
				
				QString name() const;
				QString icon() const;
				QString type() const;
				QString label() const;
				QString description() const;
				
				bool hasSettings() const;
				
				QWidget* widget() const;
				
				void setProperty(const QString& k, const QString& v);
				
				void applyConfigChanges();
				void discardConfigChanges();
				
			private:
				QString m_type;
				bool m_hasSettings;
				QDomElement m_elem;
				QPluginConfig *m_owner;
				QHash<QString, QString> m_buffer;
		};
		
		QPluginConfig(QPlugin *p, const QString& lib);
		~QPluginConfig();
		
		QString info() const;
		QString library() const;
		QString settings() const;
		
		QString tr(const QString& s) const;
		
		QList<Entry> entries() const;
		
		static QString substitute(const QString& v);
		
		static QString storageLocation();
		static void setStorageLocation(QString path);
		static QString getSchemeStorageLocation(const QString& scheme, const QDateTime& mod);
		
	private:
		QPlugin *m_plugin;
		QDomDocument *m_doc;
		QList<Entry> m_entries;
		QString m_scheme, m_library;
		
		static QString m_storage;
};

#endif // _QPLUGIN_CONFIG_H_
