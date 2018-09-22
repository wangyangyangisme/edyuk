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

#include "qshortcutmanager.h"

#include "qshortcutdialog.h"

#include <QTextStream>

/*!
	\file qshortcutmanager.cpp
	\brief implementation of the QShortcutManager class
	
	\see QShortcutManager
*/

/*!
	\class QShortcutManager
	\brief Generic shortcut management class.
	
	This class manages shortcuts for the complete application, plugins included.
	The only thing needed is to register actions and to make sure not to override
	shortcuts of managed actions.
	
	\see registerAction()
	\see unregisterAction()
*/

#include <QDir>
#include <QFile>

#if QT_VERSION < 0x00040200
// quintptr is typedef(ed only since Qt 4.2 ... :(
typedef void* quintptr;
#endif

QString QShortcutManager::sPath;

/*!
	\brief Constructor
*/
QShortcutManager::QShortcutManager()
 : sLang("untranslated")
{
	pDialog = new QShortcutDialog(this);
	
	pDoc = new QDomDocument("SHORTCUTS");
	
	readXml();
}

/*!
	\brief Destructor
*/
QShortcutManager::~QShortcutManager()
{
	writeXml();
	
	delete pDoc;
}

/*!
	\brief Shows the shortcut configuration dialog
*/
void QShortcutManager::configure()
{
	pDialog->exec();
}

/*!
	\brief Applies the changes
*/
void QShortcutManager::applyAll()
{
	
}

/*!
	\brief Applies the change
*/
void QShortcutManager::apply(const QString& s, const QString& t)
{
	QHash<QString, QString>::iterator i;
	
	#ifdef _DEBUG_
	//qDebug("target = %s;", qPrintable(t));
	#endif
	
	QStringList l = t.split('/');
	
	QString name = l.takeAt(l.count() - 1);
	QString context = l.join("/");
	
	if ( !s.isEmpty() )
	{
		if ( m_shortcuts.contains(s) )
		{
			if ( m_shortcuts.value(s) == t )
			{
				//qDebug("not applying shortcut...");
				return;
			}
			
			apply("", m_shortcuts.value(s));
		}
		
		m_shortcuts[s] = t;
	}
	
	#ifdef _DEBUG_
	qDebug("context = %s; name = %s; shortcut = %s;",
			qPrintable(context),
			qPrintable(name),
			qPrintable(s));
	#endif
	
	QDomElement e = node(name, context, true);
	
	// update XML config file
	e.setAttribute("shortcut", s);
	
	//qDebug("applying shortcut to %i actions", m_actions[t].count());
	
	// apply shortcut sequences to registered actions
	foreach ( QAction *a, m_actions[t] )
	{
		a->setShortcut(s);
	}
	
}

/*!
	\brief changes shortcut data according to \a lang
	
	\param lang current language
	
	If another language was previously managed, shortcuts are automatically
	saved.
*/
void QShortcutManager::languageChanged(const QString& lang)
{
	if ( lang == sLang )
		return;
	
	#ifdef _DEBUG_
	qDebug() << "giving up " << sLang << " and switching to " << lang;
	#endif
	
	// save previous shortcuts
	writeXml();
	
	sLang = lang;
	
	// read new shortcuts
	readXml();
	
	QHash<QString, QList<QAction*> >::iterator i;
	
	for ( i = m_actions.begin(); i != m_actions.end(); i++ )
	{
		QDomElement e = node(i.key().section('/', -1), i.key().section('/', 0, -2));
		
		if ( e.isNull() )
			continue;
		
		QString ks = e.attribute("shortcut");
		
		if ( ks.isEmpty() )
		{
			apply(e.attribute("default"), i.key());
		} else {
			apply(ks, i.key());
		}
	}
	
	pDialog->retranslate();
}

/*!

*/
void QShortcutManager::translateContext(const QString& cxt, const QString& tcxt)
{
	m_translations[cxt] = tcxt;
}

/*!
	\internal
*/
void QShortcutManager::readXml()
{
	QFile f(file(sLang));
	
	#ifdef _DEBUG_
	qDebug() << "reading xml shortcut file : " << file(sLang) << " ...";
	#endif
	
	if ( f.open(QFile::ReadOnly | QFile::Text) )
	{
		pDoc->setContent(&f);
	} else {
		pDoc->setContent(QLatin1String("<!DOCTYPE SHORTCUTS>\n<SHORTCUTS>\n\n</SHORTCUTS>\n"));
	}
	
	if ( pDoc->documentElement().isNull() )
	{
		QDomElement root = pDoc->createElement("SHORTCUTS");
		
		pDoc->appendChild(root);
	}
}

/*!
	\internal
*/
void QShortcutManager::writeXml()
{
	if ( !sLang.isEmpty() )
	{
		QFile prev(file(sLang));
		QTextStream  out(&prev);
		
		if ( prev.open(QFile::WriteOnly | QFile::Text) )
			out << pDoc->toString(4).replace("    ", "\t");
		else
			qWarning("Can\'t save %s shortcuts : check out permissions",
					qPrintable(sLang));
	}
}

/*!
	\brief check if action \a a is already registered
	
	\return true if a is found, otherwise false
*/
bool QShortcutManager::contains(QAction *a) const
{
	foreach ( QList<QAction*> l, m_actions )
		if ( l.contains(a) )
			return true;
	
	return false;
}

/*!
	\brief register an action in the shortcut manager
	
	\param a action to register
	\param cxt context of the action, usually a menu/toolbar name
	\param def default shortcut
*/
void QShortcutManager::registerAction(QAction *a, const QString& cxt, const QString& def)
{
	if ( !a || contains(a) )
		return;
	
	#ifdef _DEBUG_
	//qDebug() << "adding action : " << a->text()
	//		<< " from " << cxt
	//		<< " with " << def;
	#endif
	
	connect(a	, SIGNAL( destroyed(QObject*) ),
			this, SLOT  ( destroyed(QObject*) ) );
	
	QString qname = cxt + "/" + a->text();
	
	m_actions[qname] << a;
	
	QDomElement e = node(a, cxt);
	
	e.setAttribute("default", def);
	
	QString sh = e.attribute("shortcut");
	
	if (
			sh.isEmpty()
		&&
			(
				!m_shortcuts.contains(def)
			||
				(m_shortcuts.value(def) == qname)
			)
		)
		sh = def;
	
	if ( sh.count() )
	{
		e.setAttribute("shortcut", sh);
		m_shortcuts[sh] = qname;
		
		a->setShortcut(QKeySequence(sh));
	}
	
	//apply(def, cxt + "/" + a->text());
}

/*!
	\brief unregister an action from the shortcut manager
	
	\param a action to unregister
*/
void QShortcutManager::unregisterAction(QAction *a)
{
	if ( !a )
		return;
	
	foreach ( QList<QAction*> l, m_actions )
	{
		if ( l.contains(a) )
		{
			l.removeAll(a);
			a->setShortcut(QKeySequence(""));
		}
	}
}

/*!
	\return the location where XML shortcut settings are stored
*/
QString QShortcutManager::settingsPath()
{
	return sPath.count() ? sPath : QDir::homePath()
									+ QDir::separator()
									+ "."
									+ QApplication::applicationName()
									+ QDir::separator();
}

/*!
	\brief Set the location where settings are stored
	\param path The path where XML shortcut settings are stored
*/
void QShortcutManager::setSettingsPath(const QString& path)
{
	sPath = path;
}

/*!
	\param lang Language whose shortcut file is asked
	\return file name of the shrtcut file corresponding to language \a lang

*/
QString QShortcutManager::file(const QString& lang)
{
	return settingsPath()
			+ "shortcuts_"
			+ lang
			+ ".xml";
}

/*!
	\overload
	
	\param a action whose xml representation is searched
	\param cxt context of the action (usually menu/toolbar name)
	
	\return the corresponding dom element, if none found, one is created.
*/
QDomElement QShortcutManager::node(QAction *a, const QString& cxt)
{
	if ( !a )
		qFatal("Can\'t find xml for a NULL action!!!");
	
	return node(a->text(), cxt, true);
}

/*!
	\brief find the xml element representing the name \a n in context \a cxt
	
	\param a action whose xml representation is searched
	\param cxt context of the action (usually menu/toolbar name)
	
	\return the corresponding dom element, if none found, one is created.
*/
QDomElement QShortcutManager::node(const QString& n, const QString& cxt, bool create)
{
	QDomElement elem;
	QDomNodeList nodes = pDoc->elementsByTagName("action");
	
	for ( int i = 0; i < nodes.size(); ++i )
	{
		elem = nodes.at(i).toElement();
		
		if ( elem.attribute("name") == n && elem.attribute("context") == cxt )
			return elem;
	}
	
	if ( !create )
		return QDomElement();
	
	elem = pDoc->createElement("action");
	
	elem.setAttribute("name", n);
	elem.setAttribute("context", cxt);
	
	pDoc->documentElement().appendChild(elem);
	
	return elem;
}

/*!
	\internal
*/
void QShortcutManager::destroyed(QObject *o)
{
	if ( !o )
		return;
	
	//qDebug("spotted deletion...");
	
	QHash<QString, QList<QAction*> >::iterator l = m_actions.begin();
	
	while ( l != m_actions.end() )
	{
		QList<QAction*>::iterator i = l->begin();
		
		while ( i != l->end() )
		{
			if ( quintptr(*i) == quintptr(o) )
			{
				//qDebug("erasing occurence of %s", qPrintable((*i)->text()));
				i = l->erase(i);
				
				/*
				if ( l->isEmpty() )
				{
					l = m_actions.erase(l);
					continue;
				}
				*/
			} else {
				++i;
			}
		}
		
		++l;
	}
}
