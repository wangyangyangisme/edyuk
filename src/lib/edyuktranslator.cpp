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

#include "edyuktranslator.h"

/*!
	\file edyuktranslator.cpp
	\brief Implementation of the EdyukTranslator class.
	
	\see EdyukTranslator
*/

#include <QDir>
#include <QMenu>
#include <QLocale>
#include <QActionGroup>
#include <QApplication>
#include <QLibraryInfo>
#include <QTranslator>


/*!
	\class EdyukTranslator
	\brief class in charge of run-time translation
*/

/*!
	\brief Constructor
*/
EdyukTranslator::EdyukTranslator(QSettingsServer *s)
 : QSettingsClient(s, "lang"), sLang("untranslated")
{
	pMenu = new QMenu(tr("Language"));
	pMenu->setIcon(QIcon(":/langs.png"));
	
	g = new QActionGroup(this);
	g->setExclusive(true);
	
	connect(g	, SIGNAL( triggered(QAction*) ), 
			this, SLOT  ( setLanguage(QAction*) ) );
	
	scanLangs();
}

EdyukTranslator::~EdyukTranslator()
{
	sLang.clear();
	delete pMenu;
	
	qDeleteAll(translators);
	translators.clear();
}

/*!
	\return Action of the menu holding all languages availables
	
	\see QMenu::menuAction()
*/
QAction* EdyukTranslator::action() const
{
	return pMenu->menuAction();
}

/*!
	\return current language
	
	Note : The language returned is true for edyuk core library but
	may not apply to all plugins, depending on available translations.
*/
QString EdyukTranslator::currentLanguage() const
{
	return sLang;
}

/*!
	\brief Sets default language
*/
void EdyukTranslator::setDefaultLanguage()
{
	QString lang;
	
	switch ( value("mode").toInt() )
	{
		case 0 :
			//lang = "untranslated";
			break;
			
		case 1 :
			lang = QLocale::system().name().left(2);
			break;
			
		case 2 :
			lang = value("last").toString();
			
			if ( lang.isEmpty() )
				lang = QLocale::system().name().left(2);
			
			break;
			
		default:
			qWarning("Invalid translation mode : settings may be corrupted");
			break;
	}
	
	if ( !lang.isEmpty() )
		setLanguage(lang);
}

/*!
	Scan the content of the "translations" subdirectory of current application
	and construct the corresponing menu that allows run-time translation.
*/
void EdyukTranslator::scanLangs()
{
	pMenu->clear();
	
	foreach ( QAction *a, g->actions() )
		g->removeAction(a);
	
	QRegExp expr("^edyuk_(\\w+)\\.qm$");
	
	foreach ( QString dp, Edyuk::dataPathes() )
	{
		QDir dir(dp + "/translations");
		QFileInfoList fl = dir.entryInfoList(QDir::Files | QDir::Readable);
		
		//qDebug("looking for translation in %s", qPrintable(dp));
		
		foreach ( QFileInfo info, fl )
		{
			if ( info.fileName().contains(expr) )
			{
				QString lang = expr.cap(1);
				
				QAction *a = new QAction(lang, this);
				a->setCheckable(true);
				
				if ( lang == currentLanguage() )
					a->setChecked(true);
				
				g->addAction(a);
				cross.insert(a, lang);
			}
		}
	}
	
	pMenu->addActions(g->actions());
}

/*!
	\arg lang Language to set. Edyuk uses the same format same as QLocale::name()
	Set the application language to \arg lang
*/
void EdyukTranslator::setLanguage(const QString& lang)
{
	#ifdef _EDYUK_DEBUG_
	qDebug("setting language to : %s", qPrintable(lang));
	#endif
	
	foreach ( QTranslator *t, translators )
	{
		QCoreApplication::removeTranslator(t);
		delete t;
	}
	
	translators.clear();
	
	//if ( lang != "unstranslated" )
	//{
		QStringList files;
		QString suff = "_" + lang + ".qm";
		
		foreach ( QString dp, Edyuk::dataPathes() )
		{
			QDir dir(dp + "/translations");
			QFileInfoList fl = dir.entryInfoList(QDir::Files | QDir::Readable);
			
			//qDebug("looking for translation in %s", qPrintable(dp));
			
			foreach ( QFileInfo info, fl )
				if ( info.fileName().endsWith(suff) )
					files << info.absoluteFilePath();
		}
		
		QTranslator *qt = new QTranslator(this);
		
		if ( qt->load("qt_" + lang, QLibraryInfo::location(QLibraryInfo::TranslationsPath)) )
			QCoreApplication::installTranslator(qt);
		else
			delete qt;
		
		foreach ( QString s, files )
		{
			//qDebug("entry : %s", qPrintable(s));
			
			//qDebug("translator found!");
			
			QTranslator *t = new QTranslator(this);
			
			if ( t->load(s) )
			{
				QCoreApplication::installTranslator(t);
				#ifdef _EDYUK_DEBUG_
				qDebug("successfuly loaded data from %s", qPrintable(s));
				#endif
			} else {
				delete t;
				#ifdef _EDYUK_DEBUG_
				qDebug("failed to load data from %s", qPrintable(s));
				#endif
			}
		}
	//}
	
	pMenu->setTitle(tr("Language"));
	
	QAction *a = cross.key(lang);
	
	if ( a )
	{
		sLang = lang;
		a->setChecked(true);
	} else {
		sLang.clear();
	}
	
	setValue("last", sLang);
	
	emit languageChanged(sLang);
	
	QEvent e((QEvent::Type)Edyuk::RunTimeTranslation);
	QCoreApplication::sendEvent(qApp, &e);
}

/*!
	\arg a menu action triggered
	Set language corresponding to triggered action.
	
	\see action
*/
void EdyukTranslator::setLanguage(QAction *a)
{
	QHash<QAction*, QString>::const_iterator i = cross.find(a);
	
	if ( i == cross.constEnd() )
		return;
	
	setLanguage(*i);
}
