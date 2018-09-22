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

#include "qpluginconfig.h"

#include "qpluginconfigwidget.h"

#include <QDir>
#include <QFile>
#include <QHash>
#include <QVariant>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QDomDocument>
#include <QLibraryInfo>

QPluginConfig::Entry::Entry()
 : m_owner(0)
{
}

QPluginConfig::Entry::Entry(QPluginConfig *o, const QString& t, QDomElement e)
 : m_elem(e), m_owner(o)
{
	m_type = t;
	
	m_hasSettings = e.firstChildElement("Config").elementsByTagName("Key").count();
}

QPluginConfig::Entry::~Entry()
{
	
}

QPluginConfig* QPluginConfig::Entry::owner() const
{
	return m_owner;
}

QString QPluginConfig::Entry::name() const
{
	return m_elem.attribute("name");
}

QString QPluginConfig::Entry::icon() const
{
	return m_elem.attribute("icon");
}

QString QPluginConfig::Entry::type() const
{
	return m_type;
}

QString QPluginConfig::Entry::label() const
{
	return m_owner->tr(m_elem.attribute("label"));
}

QString QPluginConfig::Entry::description() const
{
	return m_owner->tr(m_elem.firstChildElement("Description").firstChild().toText().data().simplified());
}

bool QPluginConfig::Entry::hasSettings() const
{
	return m_hasSettings;
}

static void setValue(QDomElement e, const QString& key, const QString& value)
{
	QDomNodeList l = e.elementsByTagName("Key");
	
	for ( int i = 0; i < l.count(); ++i )
	{
		QDomElement v, elem = l.at(i).toElement();
		
		if ( elem.attribute("id") != key )
			continue;
		
		v = elem.firstChildElement("Value");
		
		if ( v.isNull() )
		{
			v = e.ownerDocument().createElement("Value");
			e.appendChild(v);
		}
		
		QDomText txt = v.firstChild().toText();
		
		if ( txt.isNull() )
		{
			txt = e.ownerDocument().createTextNode(value);
			v.appendChild(txt);
		} else {
			txt.setData(value);
		}
		
		//qDebug("saved [%s = %s]", qPrintable(key), qPrintable(value));
		
		return;
	}
}

/*!

*/
void QPluginConfig::Entry::setProperty(const QString& k, const QString& v)
{
	m_buffer[k] = v;
}

/*!
	\brief Apply all changes made to the last widget obtained through componentConfigWidget()
*/
void QPluginConfig::Entry::applyConfigChanges()
{
	if ( m_buffer.count() )
	{
		QString prefix = m_type + "/" + m_elem.attribute("name") + "/";
		QDomElement e = m_elem.firstChildElement("Config");
		QHash<QString, QString>::const_iterator it = m_buffer.constBegin();
		
		while ( it != m_buffer.constEnd() )
		{
			// change for current "session"
			//qDebug("%s = %s", qPrintable(prefix + it.key()), qPrintable(*it));
			
			m_owner->m_plugin->setConfigKey(prefix + it.key(), *it);
			
			// save change for next "sessions"
			setValue(e, it.key(), *it);
			
			++it;
		}
	}
	
	m_buffer.clear();
}

/*!
	\brief Discard all non-applied config changes
*/
void QPluginConfig::Entry::discardConfigChanges()
{
	m_buffer.clear();
}

QWidget* QPluginConfig::Entry::widget() const
{
	return QPluginConfigWidget::create(*this);
}

//

QPluginConfig::QPluginConfig(QPlugin *p, const QString& lib)
 :
	m_plugin(p),
	m_scheme(getSchemeStorageLocation(p->configScheme(), QFileInfo(lib).lastModified())),
	m_library(lib)
{
	m_doc = new QDomDocument;
	
	QFile f(m_scheme);
	
	if ( !f.open(QFile::ReadOnly | QFile::Text) )
	{
		qWarning("Unable to load plugin config.");
		return;
	}
	
	m_doc->setContent(&f);
	f.close();
	
	
	// "restore" saved settings key
	QDomElement t = m_doc->documentElement().firstChildElement("Component");
	
	while ( !t.isNull() )
	{
		QString type = t.attribute("type");
		QDomElement id = t.firstChildElement("Class");
		
		while ( !id.isNull() )
		{
			QString oid = id.attribute("name");
			
			Entry e(this, type, id);
			
			m_entries << e;
			
			// restore config on plugin load...
			QDomElement key = id.firstChildElement("Config")
								.firstChildElement("Key");
			
			while ( !key.isNull() )
			{
				QString key_id = key.attribute("id"),
						value = key	.firstChildElement("Value")
									.firstChild()
									.toText()
									.data();
				
				value = QPluginConfig::substitute(value);
				
				m_plugin->setConfigKey(type + "/" + oid + "/" + key_id, value);
				
				key = key.nextSiblingElement("Key");
			}
			
			id = id.nextSiblingElement("Class");
		}
		
		t = t.nextSiblingElement("Component");
	}
}

/*!
	\brief dtor
*/
QPluginConfig::~QPluginConfig()
{
	QFile f(m_scheme);
	
	if ( !f.open(QFile::WriteOnly | QFile::Text) )
	{
		qWarning("Unable to store plugin config. [%s]", qPrintable(m_scheme));
		return;
	}
	
	QTextStream out(&f);
	
	out << m_doc->toString();
	
	delete m_doc;
	f.close();
}

/*!
	\return short, human readable information that would allow
	user to chose between several plugins f a conflict arised.
*/
QString QPluginConfig::info() const
{
	return tr("bin : %1, settings stored in %2").arg(m_library).arg(m_scheme);
}

/*!
	\return the XML file where settings are stored
*/
QString QPluginConfig::settings() const
{
	return m_scheme;
}


/*!
	\return the filename from which plugin was loaded
*/
QString QPluginConfig::library() const
{
	return m_library;
}

/*!
	\return The list of entries availables in this plugin
	An entry corresponds to an "actual plugin class", as reported by the plugin scheme file.
	Each entry has distinct settings and a bunch of metadata which is managed by the Entry
	class.
*/
QList<QPluginConfig::Entry> QPluginConfig::entries() const
{
	return m_entries;
}

/*!
	\brief Ask the plugin to translate a string
	\note This function is used to achieve translation of
	strings found in config scheme files...
*/
QString QPluginConfig::tr(const QString& s) const
{
	return m_plugin->tr(s);
}

static bool isTrue(const QString& s)
{
	QString platform
	#ifdef Q_WS_WIN
		= "win32"
	#elif defined(Q_WS_X11)
		= "unix"
	#elif defined(Q_WS_MAC)
		= "mac"
	#else
	
	#endif
	;
	
	if ( s == platform )
		return true;
	
	return s.isEmpty();
}

/*!
	\brief Substitute generic sequences $${...}
*/
QString QPluginConfig::substitute(const QString& v)
{
	QString s;
	
	static QHash<QString, QString> _subs;
	
	if ( _subs.isEmpty() )
	{
		_subs["QT_INSTALL_PREFIX"] = QLibraryInfo::location(QLibraryInfo::PrefixPath);
		_subs["QT_INSTALL_DATA"] = QLibraryInfo::location(QLibraryInfo::DataPath);
		_subs["QT_INSTALL_DOC"] = QLibraryInfo::location(QLibraryInfo::DocumentationPath);
		_subs["QT_INSTALL_HEADERS"] = QLibraryInfo::location(QLibraryInfo::HeadersPath);
		_subs["QT_INSTALL_LIBS"] = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
		_subs["QT_INSTALL_BINS"] = QLibraryInfo::location(QLibraryInfo::BinariesPath);
		_subs["QT_INSTALL_PLUGINS"] = QLibraryInfo::location(QLibraryInfo::PluginsPath);
		_subs["QT_INSTALL_TRANSLATIONS"] = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
		_subs["QT_INSTALL_CONFIGURATION"] = QLibraryInfo::location(QLibraryInfo::SettingsPath);
		_subs["QT_INSTALL_EXAMPLES"] = QLibraryInfo::location(QLibraryInfo::ExamplesPath);
		_subs["QT_INSTALL_DEMOS"] = QLibraryInfo::location(QLibraryInfo::DemosPath);
		_subs["QT_VERSION"] = QT_VERSION_STR;
		
		_subs["QPLUGIN_STORAGE"] = storageLocation();
	}
	
	for ( int i = 0; i < v.count(); ++i )
	{
		if ( v.mid(i, 3) == "$${" )
		{
			i += 3;
			int e = v.indexOf('}', i);
			
			if ( e == -1 )
				break;
			
			QString sub, id = v.mid(i, e - i);
			i = e;
			
			if ( id.contains(',') )
			{
				QStringList alter = id.split(',');
				QStringList cond, val;
				
				foreach ( const QString& a, alter )
				{
					if ( a.count(':') == 1 )
					{
						QStringList tmp = a.split(':');
						cond << tmp.at(0);
						val  << tmp.at(1);
					} else {
						cond << QString();
						val  << a;
					}
				}
				
				for ( int j = 0; j < cond.count(); ++j )
				{
					if ( isTrue(cond.at(j)) )
					{
						sub = val.at(j);
						break;
					}
				}
				
				s += sub;
				
			} else {
				s += _subs[id];
			}
		//} else if ( ((i + 1) < v.count()) && (v.at(i) == '\\') ) {
		//	s += v.at(++i);
		} else {
			s += v.at(i);
		}
	}
	
	return s;
}

static QString m_defaultStorage = QDir::homePath() + QDir::separator() + ".qps";
QString QPluginConfig::m_storage;

/*!
	\return the current storage location
	
	QPluginConfig extract XML config schemes from QPlugin objects and
	"concretize" them in the storage location. These schemes hold many
	useful informations :
	<ul>
		<li>Plugin structure (type and keys available, though is is not forced)
		<li>Config scheme used to create config widgets
		<li>Current value of each config key
	</ul>
	
	\note the default value for this is $HOME/.qps
*/
QString QPluginConfig::storageLocation()
{
	if ( m_storage.isEmpty() )
		setStorageLocation(m_defaultStorage);
	
	return m_storage;
}

/*!
	\brief Set the current storage location
	
	If the given location does not exist, it is created through QDir::mkPath()
	If path creation fails, the storage location falls back to default storage.
	
	\see storageLocation()
*/
void QPluginConfig::setStorageLocation(QString path)
{
	if ( !QFile::exists(path) )
		if ( !QDir::home().mkpath(path) )
			path = m_defaultStorage;
	
	m_storage = path;
}

/*!
	\brief Get a valid storage location for the given scheme
*/
QString QPluginConfig::getSchemeStorageLocation(const QString& scheme, const QDateTime& mod)
{
	QString valid = QDir(storageLocation()).absoluteFilePath(QFileInfo(scheme).fileName());
	
	/*
		looks like copy from resource ends up creating read-only files...
	*/
	//QFile::copy(scheme, valid);
	
	QDateTime creat = QFileInfo(valid).lastModified();
	
	//qDebug() << "created : " << creat << endl << "modified : " << mod;
	
	if ( !QFile::exists(valid) || (creat < mod) )
	{
		//qDebug("blanking out scheme data for %s", qPrintable(valid));
		
		QFile fin(scheme), fout(valid);
		
		if ( fin.open(QFile::ReadOnly | QFile::Text) && fout.open(QFile::WriteOnly | QFile::Text) )
		{
			QTextStream in(&fin), out(&fout);
			
			out << in.readAll();
			
			fout.close();
			fin.close();
		} else {
			qWarning("scheme copy failed...");
		}
	}
	
	return valid;
}
