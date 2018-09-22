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

#ifndef _QPLUGIN_SYSTEM_H_
#define _QPLUGIN_SYSTEM_H_

#include <qglobal.h>

/*!
	\file qpluginsystem.h
	
	\brief Configuration stuff for QPluginSystem framework.
*/

/*!
	\brief Macro used to deal with cross-platform shared library creation
*/
#ifdef QPLUGIN_BUILD
	#if (defined(QT_SHARED) || defined(QT_DLL)) && !defined(QT_PLUGIN)
		#define QPLUGIN_EXPORT Q_DECL_EXPORT
	#else
		#define QPLUGIN_EXPORT
	#endif
#else
	#ifndef QPLUGIN_STATIC
		#define QPLUGIN_EXPORT Q_DECL_IMPORT
	#else
		#define QPLUGIN_EXPORT
	#endif
#endif

#define QPLUGIN_KEY(T) ""#T

#define QPLUGIN_COMPONENT(T) _##T##Inst
#define QPLUGIN_ADD_COMPONENT(T) T *_##T##Inst

#define QPLUGIN_INST(id, T, B)					\
	if ( id == ""#T )							\
	{											\
		if ( !_##T##Inst )						\
			_##T##Inst = new T;					\
		return dynamic_cast<B*>(_##T##Inst);	\
	}

/// \internal
#define QPLUGIN_INSTANCE_STR "q_plugin_instance"
/// \internal
#define QPLUGIN_INSTANCE_SYMBOL q_plugin_instance

/*!
	\brief Magic macro to export a plugin
*/
#define QPLUGIN_EXPORT_IMPL(ROOT)									\
	static ROOT *_q_plugin_instance;								\
	extern "C" Q_DECL_EXPORT QPlugin* QPLUGIN_INSTANCE_SYMBOL()		\
	{																\
		if ( !_q_plugin_instance )									\
			_q_plugin_instance = new ROOT;							\
		return _q_plugin_instance;									\
	}																\
	

#include <QStringList>

class QPlugin
{
	public:
		virtual ~QPlugin() {}
		
		virtual void retranslate() = 0;
		virtual QString tr(const QString& s) const = 0;
		
		virtual QString configScheme() const = 0;
		virtual void setConfigKey(const QString& key, const QVariant& value) = 0;
		
		virtual QStringList types() = 0;
		virtual QStringList keys(const QString& t) = 0;
		virtual void* object(const QString& id, const QString& t) = 0;
};

/*!
	\brief Signature of a plugin instantiation function
*/
typedef QPlugin* (*QPluginInstanciator)();

#endif //!_QPLUGIN_SYSTEM_H_
