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

#ifndef _QPLUGIN_MANAGER_H_
#define _QPLUGIN_MANAGER_H_

#include "qpluginsystem.h"

/*!
	\file qpluginmanager.h
	
	\brief Definition of the QPluginManager class.
*/

#include <QHash>
#include <QStringList>

class QPlugin;
class QPluginConfig;
class QPluginConfigWidget;

class QPLUGIN_EXPORT QPluginManager
{
	friend class QPluginManagerConfig;
	
	public:
		typedef void (*Handler)(void *obj);
		
		QPluginManager();
		virtual ~QPluginManager();
		
		void clear();
		
		QStringList blacklist() const;
		void setBlacklist(const QStringList& l);
		
		void enablePlugin(QPlugin *p);
		void enablePlugin(const QString& p);
		void disablePlugin(QPlugin *p);
		void disablePlugin(const QString& p);
		bool isPluginEnabled(QPlugin *p) const;
		bool isPluginEnabled(const QString& p) const;
		void setPluginEnabled(QPlugin *p, bool e);
		void setPluginEnabled(const QString& p, bool e);
		
		QStringList types() const;
		QStringList keys(const QString& t) const;
		void* object(const QString& id, const QString& t);
		
		template <class T>
		T* object(const QString& id, const QString& t)
		{
			return static_cast<T*>(object(id, t));
		}
		
		void addPlugin(const QString& p);
		void addPluginPath(const QString& p);
		
		void addHandler(const QString& type, Handler h);
		
		static QPluginManager* instance();
		
	private:
		QStringList m_files, m_blacklist;
		QHash<QString, Handler> m_handlers;
		QHash<QPlugin*, QPluginConfig*> m_plugins;
};

#endif //!_QPLUGIN_MANAGER_H_
