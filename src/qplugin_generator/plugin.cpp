/****************************************************************************
**
** Generated
**    by QPlugin Generator $version$, (C) fullmetalcoder 2007
**    from $input$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "$output$.h"

/*!
	\file $output$.cpp
	\brief Implementation of a QPlugin-based plugin class : $class$
*/

$include_component_list$

QHash<QString, QVariant> $class$::m_keys;

$class$::$class$()
{
	Q_INIT_RESOURCE($scheme_resource$);
	
$init_component_list$
}

$class$::~$class$()
{
$delete_component_list$
}

QString $class$::tr(const QString& txt) const
{
	return tr(qPrintable(txt));
}

QString $class$::configScheme() const
{
	return ":$scheme_path$";
}

QHash<QString, PropertyWatchList>& $class$::watches()
{
	static QHash<QString, PropertyWatchList> _w;
	
	return _w;
}

void $class$::setConfigKey(const QString& key, const QVariant& value)
{
	m_keys[key] = value;
	
	if ( watches().contains(key) )
	{
		PropertyWatchList list = watches()[key];
		
		foreach ( PropertyWatch notify, list )
			notify(value);
	}
}

QStringList $class$::types()
{
$component_types$
}

QStringList $class$::keys(const QString& t)
{
$component_keys$
	
	return QStringList();
}

void* $class$::object(const QString& id, const QString& t)
{
$object_for_component$
	
	return 0;
}

void $class$::stubTrForSchemeStrings()
{
$scheme_strings$
}

QPLUGIN_EXPORT_IMPL($class$)
