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

#include "edyuksettings.h"

#include "edyukgui.h"
#include "edyukapplication.h"
#include "edyukconfigdialog.h"

#include "qsettingsclient.h"

/*!
	\file edyuksettings.cpp
	\brief Implementation of the EdyukSettings class
	
	\see EdyukSettings
*/

#include <QMenu>
#include <QPoint>
#include <QTimer>
#include <QProcess>
#include <QMessageBox>

/*!
	\class EdyukSettings

Edyuk's corelib's settings map is as follows:
 -----------------------------------------------------------------------------------------------
|	purpose			|	key			|	type			|
|-----------------------------------------------------------------------------------------------|
|	max nb of files  	|	recent/filecount	|	int			|
|	recent files  		|	recent/files		|	QStringList		|
|	max nb of projects 	|	recent/projectcount	|	int			|
|	recent projects  	|	recent/projects		|	QStringList		|
|-----------------------------------------------------------------------------------------------|
|	file reopening mode	|	opened/file_mode	| 	int			|
|	project reopening mode	|	opened/project_mode	| 	int			|
|	opened file	  	|	opened/file		|	QString			|
|	opened files  		|	opened/files		|	QStringList		|
|	opened project  	|	opened/project		|	QString			|
|	opened projects  	|	opened/projects		|	QStringList		|
|-----------------------------------------------------------------------------------------------|
|	instance :		|	gui/instance		|	int			|
|	main win states :	|	gui/winstate		|	Qt::WindowStates	|
|	main win title : 	|	gui/title		|	QString			|
|	main win width :	|	gui/width		|	int			|
|	main win height : 	|	gui/height		|	int			|
|	main win position : 	|	gui/pos			|	QPoint			|
|	gui style :		|	gui/style		|	QString			|
|	reload perspective :	|	gui/mode		|	int			|
|	perspective :		|	gui/perspective		|	QString			|
|	mw state for persp:	|	gui/state/%1		|	QByteArray		|
|-----------------------------------------------------------------------------------------------|
|	language selection :	|	lang/mode		|	int			|
|	last languages :	|	lang/last		|	QString			|
|-----------------------------------------------------------------------------------------------|
|	default core :		|	plugins/core		|	QString			|
|	enabled misc :		|	plugins/misc		|	QStringList		|
|	comp for lang %1 :	|	plugins/comp/%1		|	QString			|
|	debug for symb %1 :	|	plugins/debug/%1	|	QString			|
 -----------------------------------------------------------------------------------------------

*/

#ifdef Q_WS_WIN
	const QString EdyukSettings::PATH_VAR = "PATH";
#elif defined(Q_WS_X11)
	const QString EdyukSettings::PATH_VAR = "PATH";
#elif defined(Q_WS_MAC)
	const QString EdyukSettings::PATH_VAR = "PATH";
#endif

EdyukSettings::EdyukSettings(QObject *p)
 : QSettingsServer(p), pCfgDlg(0)
{
	m = new QMenu(tr("&Reopen..."));
	m->setIcon(QIcon(":/reopen.png"));
	connect(m	, SIGNAL( triggered(QAction*) ),
			this, SLOT  ( recent(QAction*) ) );
	
	aClear = new QAction(QIcon(":/clear.png"), tr("&Clear history"), this);
	connect(aClear	, SIGNAL( triggered() ),
			this	, SLOT  ( clearRecents() ) );
	
	buildRecents();
	
	if ( allKeys().isEmpty() )
		setDefault();
}

EdyukSettings::EdyukSettings(const QString& loc, QObject *p)
 : QSettingsServer(loc, p), pCfgDlg(0)
{
	m = new QMenu(tr("&Reopen..."));
	m->setIcon(QIcon(":/reopen.png"));
	connect(m	, SIGNAL( triggered(QAction*) ),
			this, SLOT  ( recent(QAction*) ) );
	
	aClear = new QAction(QIcon(":/clear.png"), tr("&Clear history"), this);
	connect(aClear	, SIGNAL( triggered() ),
			this	, SLOT  ( clearRecents() ) );
	
	buildRecents();
	
	if ( allKeys().isEmpty() )
		setDefault();
}

EdyukSettings::~EdyukSettings()
{
	delete m;
	delete pCfgDlg;
}

void EdyukSettings::retranslate()
{
	m->setTitle(tr("&Reopen..."));
	
	if ( pCfgDlg )
		pCfgDlg->retranslate();
}

void EdyukSettings::setSnippetManager(QSnippetManager *m)
{
	if ( !pCfgDlg )
		pCfgDlg = new EdyukConfigDialog(this);
	
	pCfgDlg->setSnippetManager(m);
}

void EdyukSettings::loadFormatSchemes(QLanguageFactory *f)
{
	if ( !pCfgDlg )
		pCfgDlg = new EdyukConfigDialog(this);
	
	pCfgDlg->loadFormatSchemes(f);
}

void EdyukSettings::setDefault()
{
	settings()->clear();
	
	QSettingsClient s(this);
	
	// recent files/projects
	s.beginGroup("recent");
	
	s.setValue("filecount", 15);
	s.setValue("projectcount", 5);
	
	s.endGroup();
	// !recent
	
	// opened files/projects
	s.beginGroup("opened");

	s.setValue("file_mode", Edyuk::ReopenAll);
	s.setValue("project_mode", Edyuk::ReopenAll);
	
	s.setValue("file", QString());
	s.setValue("files", QStringList());
	
	s.setValue("project", QString());
	s.setValue("projects", QStringList());
	
	s.endGroup();
	// !opened
	
	// gui
	s.beginGroup("gui");
	
	s.setValue("instance", Edyuk::InstanceSingle);
	s.setValue("winstate", Qt::WindowMaximized);
	s.setValue("width", 0);
	s.setValue("height", 0);
	s.setValue("pos", QPoint());
	s.setValue("style", QString());
	s.setValue("mode", Edyuk::ReloadPerspectiveLastUsed);
	s.setValue("perspective", QString());
	
	s.remove("state");
	
	s.endGroup();
	// !gui
	
	// lang
	s.beginGroup("lang");
	
	s.setValue("mode", Edyuk::Untranslated);
	s.setValue("last", QString());
	
	s.endGroup();
	// !lang
}

QString EdyukSettings::environment(const QString& var)
{
	QRegExp x( QString("^%1=([^\n]+)$").arg(var) );
	QStringList l = QProcess::systemEnvironment();
	
	foreach ( QString s, l )
	{
		if ( x.indexIn(s) != -1 )
			return x.cap(1);
	}
	
	return QString();
}

QStringList EdyukSettings::environment(const QStringList& dirs)
{
	QStringList l = QProcess::systemEnvironment();
	
	if ( dirs.isEmpty() )
		return l;
	
	QString PATH = PATH_VAR + "=";
	
	#ifdef Q_WS_WIN
	QString sep = ";";
	#else
	QString sep = ":";
	#endif
	
	for ( QStringList::iterator i = l.begin(); i != l.end(); i++ )
	{
		if ( !i->startsWith(PATH) )
			continue;
		
		foreach ( QString dir, dirs )
			(*i) += sep + dir;
		
		#ifdef _DEV_DEBUG_
		QMessageBox::warning(0, 0, i->split(';').join("\n"));
		#endif
		
		break;
	}
	
	return l;
}

QAction* EdyukSettings::recent() const
{
	return m->menuAction();
}

void EdyukSettings::buildRecents()
{
	#ifdef _DEV_DEBUG_
	qDebug() << "rebuilding recent files/projects menu...";
	#endif
	
	m->clear();
	m->addAction(aClear);
	
	qDeleteAll(recentFiles.keys());
	recentFiles.clear();
	qDeleteAll(recentProjects.keys());
	recentProjects.clear();
	
	#ifdef _DEV_DEBUG_
	qDebug() << "cleaned previous data...";
	#endif
	
	int n = 0;
	QString name, s, prefix;
	
	QStringList l, keys = QStringList()<<"projects"<<"files";
	
	QSettingsClient set(this, "recent");
	
	foreach ( QString name, keys )
	{
		l = set.value(name).toStringList();
		
		m->addSeparator();
		
		foreach ( QString s, l )
		{
			prefix = QString::number(n) + " : ";
			
			if ( n++ < 10 )
				prefix.prepend('&');
			
			QAction *a = new QAction(prefix + s, this);
			m->addAction(a);
			
			if ( name == "files" )
				recentFiles.insert(a, s);
			else if ( name == "projects" )
				recentProjects.insert(a, s);
			else
				qWarning(	"Unrecognized recent type : <br>%s",
							name.toLatin1().data());
		}
	}
	
	#ifdef _DEV_DEBUG_
	qDebug() << "filled with new data...";
	#endif
}

void EdyukSettings::clearRecents()
{
	remove("recent");
	buildRecents();
}

void EdyukSettings::configure()
{
	if ( !pCfgDlg )
		pCfgDlg = new EdyukConfigDialog(this);
	
	pCfgDlg->reload();
	pCfgDlg->exec();
}

void EdyukSettings::addRecentFile(const QString& fn)
{
	addRecent(fn, false);
}

void EdyukSettings::addRecentProject(const QString& fn)
{
	addRecent(fn, true);
}

void EdyukSettings::addRecent(const QString& n, bool project)
{
	if ( n.isEmpty() )
		return;
	
	int max;
	QString s;
	QStringList l;
	
	QSettingsClient set(this, "recent");
	
	if ( project )
	{
		max = set.value("projectcount", 5).toInt();
		s = "projects";
	} else {
		max = set.value("filecount", 15).toInt();
		s = "files";
	}
	
	l = set.value(s).toStringList();
	
	for( int i = 0; i < l.size(); i++)
		if ( l.at(i) == n )
			l.removeAt(i);
	
	l.prepend(n);
	
	while ( l.size() > max )
		l.pop_back();
	
	set.setValue(s, l);
	
	QTimer::singleShot(0, this, SLOT( buildRecents() ));
}

void EdyukSettings::recent(QAction *a)
{
	#ifdef _DEV_DEBUG_
	qDebug() << "opening recent file/project...";
	#endif
	
	QHash<QAction*, QString>::iterator i;
	
	i = recentFiles.find(a);
	
	if ( i != recentFiles.end() )
	{
		sRecent = *i;
		
		emit recentFile(sRecent);
		return;
	}
	
	i = recentProjects.find(a);
	
	if ( i != recentProjects.end() )
	{
		sRecent = *i;
		
		emit recentProject(sRecent);
	}
}
