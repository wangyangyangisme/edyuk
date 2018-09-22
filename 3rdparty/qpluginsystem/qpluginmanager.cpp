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

#include "qpluginmanager.h"

/*!
	\file qpluginmanager.cpp
	
	\brief Implementation of the QPluginManager class.
*/

#include "qpluginconfig.h"

#include <QDir>
#include <QFile>
#include <QLibrary>
#include <QFileInfo>

/*!
	\brief Creates an empty plugin manager
*/
QPluginManager::QPluginManager()
{
	
}

/*!
	\brief dtor
	\see clear()
*/
QPluginManager::~QPluginManager()
{
	clear();
}

/*!
	\return The plugin blacklist
*/
QStringList QPluginManager::blacklist() const
{
	return m_blacklist;
}

/*!
	\brief Set the black list
	Blacklisted plugins will never be loaded by by addPlugin() and addPluginPath().
	\note The blacklist must be made of absolute file pathes. A plugin key can optionally
	be appended (path + "@" + key) to fine tune blacklisting.
	\note Changes to the blacklist will not affect already loaded plugins
*/
void QPluginManager::setBlacklist(const QStringList& l)
{
	m_blacklist = l;
}

/*!
	\brief Enable a plugin 
*/
void QPluginManager::enablePlugin(QPlugin *p)
{
	QPluginConfig *c = m_plugins.value(p);
	m_blacklist.removeAll(c->library());
}

/*!
	\brief Enable a plugin 
*/
void QPluginManager::enablePlugin(const QString& p)
{
	m_blacklist.removeAll(p);
}

/*!
	\brief Disable a plugin 
*/
void QPluginManager::disablePlugin(QPlugin *p)
{
	QPluginConfig *c = m_plugins.value(p);
	
	if ( !m_blacklist.contains(c->library()) )
		m_blacklist << c->library();
}

/*!
	\brief Disable a plugin 
*/
void QPluginManager::disablePlugin(const QString& p)
{
	if ( !m_blacklist.contains(p) )
		m_blacklist << p;
}

/*!
	\return Whether a given plugin is enabled 
*/
bool QPluginManager::isPluginEnabled(QPlugin *p) const
{
	QPluginConfig *c = m_plugins.value(p);
	
	return m_blacklist.contains(c->library());
}

/*!
	\return Whether a given plugin is enabled 
*/
bool QPluginManager::isPluginEnabled(const QString& p) const
{
	return !m_blacklist.contains(p);
}

/*!
	\brief Enable or disable a plugin 
*/
void QPluginManager::setPluginEnabled(QPlugin *p, bool e)
{
	QPluginConfig *c = m_plugins.value(p);
	bool in = m_blacklist.contains(c->library());
	
	if ( in && e )
	{
		m_blacklist.removeAll(c->library());
	} else if ( !in && !e ) {
		m_blacklist << c->library();
	}
}

/*!
	\brief Enable or disable a plugin 
*/
void QPluginManager::setPluginEnabled(const QString& p, bool e)
{
	bool in = m_blacklist.contains(p);
	
	if ( in && e )
	{
		m_blacklist.removeAll(p);
	} else if ( !in && !e ) {
		m_blacklist << p;
	}
}

/*!
	\brief Clear the plugin manager
	\note All QPlugin objects are <br>destroyed</br> and libraries are very
	likely to be unloaded : make sure you don't use any object obtained from
	that plugin manager once it has been destroyed.
*/
void QPluginManager::clear()
{
	QHash<QPlugin*, QPluginConfig*>::const_iterator it = m_plugins.constBegin();
	
	while ( it != m_plugins.constEnd() )
	{
		delete *it;
		delete it.key();
		
		++it;
	}
	
	m_plugins.clear();
}

/*!
	\brief Load a plugin and "manage" it
	\param p path of the plugin file to load
	\note the given path must point to a valid shared library which
	exports a QPlugin implemnetation using the QPLUGIN_EXPORT_IMPL
	macro.
*/
void QPluginManager::addPlugin(const QString& p)
{
	QFileInfo info(p);
	
	if ( m_blacklist.contains(info.absoluteFilePath()) )
	{
		qDebug("blacklisted plugin : %s", qPrintable(p));
		return;
	}
	
	bool matchDebugRelease =
	#ifdef _DEBUG_
		info.baseName().endsWith("_debug")
	#else
		!info.baseName().endsWith("_debug")
	#endif
		;
	
	if ( !matchDebugRelease )
		return;
	
	if ( !(QFile::exists(p) && QLibrary::isLibrary(p)) )
		return (void)qWarning("QPluginManager : invalid file (%s)", qPrintable(p));
	
	if ( m_files.contains(p) )
		return;
	
	QLibrary lib(p);
	
	if ( !lib.load() )
		return (void)qWarning(	"QPluginManager : Can not load library %s\n\t%s",
								qPrintable(p),
								qPrintable(lib.errorString()));
	
	QPluginInstanciator instance = (QPluginInstanciator)lib.resolve(QPLUGIN_INSTANCE_STR);
	
	if ( !instance )
		return (void)qWarning(	"QPluginManager : Can not instanciate plugin\n\t%s",
								qPrintable(lib.errorString()));
	
	//qDebug("found plugin : %s", qPrintable(p));
	
	QPlugin *plugin = instance();
	
	if ( !plugin )
		return (void)qWarning(	"QPluginManager : instanciator returned a NULL pointer.");
	
	QPluginConfig *conf = new QPluginConfig(plugin, p);
	
	m_files << p;
	m_plugins[plugin] = conf;
	
	foreach ( const QString& otype, plugin->types() )
	{
		if ( !m_handlers.contains(otype) )
			continue;
		
		Handler handle = m_handlers[otype];
		
		foreach ( const QString& okey, plugin->keys(otype) )
		{
			if ( !m_blacklist.contains(info.absoluteFilePath() + "@" + okey) )
				handle(plugin->object(okey, otype));
		}
	}
}

/*!
	\brief Adds all plugins found in a given directory
	\param p path where to look for plugins
	
	\see addPlugin(const QString& p)
*/
void QPluginManager::addPluginPath(const QString& p)
{
	QString fn;
	QDir dir(p);
	
	foreach ( QString f, dir.entryList(QDir::Files | QDir::Readable) )
	{
		fn = dir.absoluteFilePath(f);
		
		if ( QLibrary::isLibrary(fn) )
			addPlugin(fn);
	}
}

/*!
	\return A list of plugin types currently loaded
*/
QStringList QPluginManager::types() const
{
	QStringList l;
	
	QHash<QPlugin*, QPluginConfig*>::const_iterator it = m_plugins.constBegin();
	
	while ( it != m_plugins.constEnd() )
	{
		l << it.key()->types();
		
		++it;
	}
	
	return l;
}

/*!
	\return A list of plugin keys available for a given type
	\param t plugin type to search keys for
*/
QStringList QPluginManager::keys(const QString& t) const
{
	QStringList l, tmp;
	
	QHash<QPlugin*, QPluginConfig*>::const_iterator it = m_plugins.constBegin();
	
	while ( it != m_plugins.constEnd() )
	{
		tmp = it.key()->keys(t);
		
		QString base = (*it)->library() + "@";
		
		foreach ( QString k, tmp )
			if ( !m_blacklist.contains(base + k) )
				l << k;
		
		++it;
	}
	
	return l;
}

/*!
	\return A "plugin object" of given type and key
	\param id plugin key of the object
	\param t plugin type of the object
	
	\note Here, "plugin object" does NOT refer to a QPlugin object
	but to an object obtained from an implementation of such an object.
*/
void* QPluginManager::object(const QString& id, const QString& t)
{
	QList<QPlugin*> l;
	QHash<QPlugin*, QPluginConfig*>::const_iterator it = m_plugins.constBegin();
	
	while ( it != m_plugins.constEnd() )
	{
		if ( it.key()->keys(t).contains(id) )
			l << it.key();
		
		++it;
	}
	
	switch ( l.count() )
	{
		case 0 :
			break;
			
		case 1 :
			return l.at(0)->object(id, t);
			
		default:
			qWarning(	"QPluginLoader : Conflict when instanciating object %s of type %s\n"
						"%i potential providers...",
						qPrintable(id),
						qPrintable(t),
						l.count());
			
			//foreach ( QPlugin *p, l )
			//	qDebug("\t- %s", qPrintable(m_plugins.value(p)->info()));
			
			break;
	}
	
	return 0;
}

/*!
	\typedef Handler
	Handler is typedef as a function pointer taking a single
	argument : a pointer to a "plugin object"
	
	\note Here, "plugin object" does NOT refer to a QPlugin object
	but to an object obtained from an implementation of such an object.
*/

/*!
	\brief Add a handler for a type of plugin
	\param type Type of plugin to which the handler is hooked
	\param h Handler function
	
	Once added, handler get called every time a new plugin is
	loaded which provides object of type \a type
	
	\note The can only be a single handler for each type. New
	handlers remove the old ones...
*/
void QPluginManager::addHandler(const QString& type, Handler h)
{
	m_handlers[type] = h;
}

/*!
	\brief Returns a pointer to the global plugin manager instance
	
	\note Though QPluginManager is NOT a singleton, in most cases a
	shared instance fits. This function avoids the hassle of global
	variable and won't create an object as long as it does not get
	called.
*/
QPluginManager* QPluginManager::instance()
{
	static QPluginManager globalPluginManager;
	
	return &globalPluginManager;
}
