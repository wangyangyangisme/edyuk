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

#include "edyukgui.h"

/*!
	\file edyukgui.cpp
	\brief Implementation of the EdyukGUI class.
	
	\see EdyukGUI
*/

#include "version.h"

#include "qshortcutmanager.h"

#include "qsettingsclient.h"
#include "qsettingsserver.h"

#include "qmdiworkspace.h"
#include "qmdistatusbar.h"
#include "qmdiperspective.h"

#include "qeditor.h"
#include "qcodeedit.h"
#include "qeditorfactory.h"
#include "qlanguagefactory.h"
#include "qcodecompletionengine.h"
#include "qlinemarksinfocenter.h"

#include "qproject.h"
#include "qprojectmodel.h"
#include "qprojectview.h"

#include "qbuildengine.h"
#include "qdebuggingengine.h"

#include "edyukaboutdialog.h"
#include "edyukcreatedialog.h"
#include "edyuktoolsmanager.h"
#include "edyukapplication.h"

#include "edyukmanagerdock.h"
#include "edyuklogdock.h"

#include <QUrl>
#include <QMenu>
#include <QFile>
#include <QTimer>
#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QStyleFactory>
#include <QSystemTrayIcon>
#include <QSessionManager>
#include <QDesktopServices>

/*!
	\ingroup gui
	@{
	
	\class EdyukGUI
	\brief The top of Edyuk's GUI layer.
	
	It inherits from qmdiMainWindow making GUI experience easier.
	
	\see qmdiMainWindow
*/

class DockSettingsWatcher : public QSettingsWatcher
{
	public:
		DockSettingsWatcher(EdyukManagerDock *md, EdyukLogDock *ld)
		 : _l(ld),  _m(md)
		{
			
		}
		
		virtual ~DockSettingsWatcher()
		{
			
		}
		
		virtual bool isWorthCatching(const QString& key) const
		{
			return key.startsWith("docks/");
		}
		
		virtual void changed(const QString& key, const QVariant& value)
		{
			if ( key == "docks/manager-display" )
				_m->setDisplayMode(value.toInt());
		}
		
	private:
		EdyukLogDock *_l;
		EdyukManagerDock *_m;
};

EdyukGUI::EdyukGUI(QSettingsServer *s)
 :	qmdiMainWindow(0),
	pServer(s)
{
	setClientFactory(new QEditorFactory(pServer));
	
	connect(this, SIGNAL( currentPerspectiveChanged(qmdiPerspective*) ),
			this, SLOT  ( perspectiveChanged(qmdiPerspective*) ) );
	
	connect(this, SIGNAL( currentPerspectiveAboutToChange(qmdiPerspective*) ),
			this, SLOT  ( perspectiveAboutToChange(qmdiPerspective*) ) );
	
	setUpdatesEnabled(false);
	//setUnifiedTitleAndToolBarOnMac(true);
	setToolButtonStyle(Qt::ToolButtonIconOnly);
	
	setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
	setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
	setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);
	
	setupActions();
	setupMenu();
	
	m_managerDock = new EdyukManagerDock(this, QSettingsClient(s, "docks"));
	m_managerDock->setObjectName("EdyukManagerDock");
	
	connect(m_managerDock	, SIGNAL( projectsOpened(bool) ),
			this			, SLOT  ( projectsOpened(bool) ) );
	
	connect(m_managerDock	, SIGNAL( projectOpened(QString) ),
			this			, SIGNAL( projectOpened(QString) ) );
	
	QToolButton *tool = new QToolButton(this);
	tool->hide();
	tool->setAutoRaise(true);
	tool->setDefaultAction(m_managerDock->toggleViewAction());
	
	status()->addButton(tool);
	addDockWidget(Qt::LeftDockWidgetArea, m_managerDock);
	
	m_logDock = new EdyukLogDock(this);
	m_logDock->setObjectName("EdyukLogDock");
	
	connect(QBuildEngine::instance(), SIGNAL( taskStarted() ),
			m_logDock				, SLOT  ( clear() ) );
	
	connect(QBuildEngine::instance(), SIGNAL( log(QString) ),
			m_logDock				, SLOT  ( log(QString) ) );
	
	connect(QBuildEngine::instance(), SIGNAL( message(QString, int, QString) ),
			m_logDock				, SLOT  ( message(QString, int, QString) ) );
	/*
	connect(QBuildEngine::instance(), SIGNAL( buildModeChanged(QString) ),
			this					, SLOT  ( buildModeChanged(QString) ) );
	
	connect(QBuildEngine::instance()	, SIGNAL( execTargetChanged(QString) ),
			QDebuggingEngine::instance(), SLOT  ( setTarget(QString) ) );
	*/
	connect(QBuildEngine::instance()	, SIGNAL( execTargetChanged(QString) ),
			this						, SLOT  ( execTargetChanged(QString) ) );
	
	connect(QDebuggingEngine::instance(), SIGNAL( started() ),
			m_logDock					, SLOT  ( clear() ) );
	
	connect(QDebuggingEngine::instance(), SIGNAL( log(QString) ),
			m_logDock					, SLOT  ( log(QString) ) );
	
	tool = new QToolButton(this);
	tool->hide();
	tool->setAutoRaise(true);
	tool->setDefaultAction(m_logDock->toggleViewAction());
	
	status()->addButton(tool);
	addDockWidget(Qt::BottomDockWidgetArea, m_logDock);
	
	retranslate();
	
	pServer->addWatcher(new DockSettingsWatcher(m_managerDock, m_logDock));
	
	QSettingsClient set(pServer, "gui");
	
	int winwidth = set.value("width", 0).toInt();
	int winheight = set.value("height", 0).toInt();
	
	if ( winwidth > 0 && winheight > 0 )
	{
		resize(winwidth, winheight);
		
		QPoint winpos = set.value("pos").toPoint();
		
		if ( !winpos.isNull() )
			move(winpos);
		
		int winstate = set.value("winstate").toInt();
		
		if ( !(winstate & Qt::WindowMinimized) )
			setWindowState(Qt::WindowStates(winstate));
		
	} else setWindowState(Qt::WindowMaximized);
	
	setWindowTitle("Edyuk " EDYUK_VERSION_STR);
	
	setUpdatesEnabled(true);
	
	if ( QSystemTrayIcon::isSystemTrayAvailable() )
	{
		// add systray
		m_trayIcon = new QSystemTrayIcon(QIcon(":/edyuk.png"), this);
		QMenu *m = new QMenu(this);
		connect(m->addAction(tr("&Restore")), SIGNAL( triggered() ), SLOT( show() ));
		connect(m->addAction(QIcon(":/exit.png"), tr("&Quit")), SIGNAL( triggered() ), SLOT( forceClose() ) );
		connect(m_trayIcon, SIGNAL( activated(QSystemTrayIcon::ActivationReason) ),
							SLOT( trayIconActivated(QSystemTrayIcon::ActivationReason) ) );
		
		m_trayIcon->setContextMenu(m);
		m_trayIcon->show();
	} else {
		m_trayIcon = 0;
	}
}

EdyukGUI::~EdyukGUI()
{
	;
}

void EdyukGUI::retranslate()
{
	qmdiMainWindow::retranslate();
	
	m_logDock->retranslate();
	m_managerDock->retranslate();
	
	translateActions();
	
	menus.setTranslation("&File", tr("&File"));
	menus.setTranslation("&Edit", tr("&Edit"));
	menus.setTranslation("&Search", tr("&Search"));
	menus.setTranslation("&View", tr("&View"));
	menus.setTranslation("&Project", tr("&Project"));
	menus.setTranslation("&Build", tr("&Build"));
	menus.setTranslation("&Debug", tr("&Debug"));
	menus.setTranslation("&Tools", tr("&Tools"));
	menus.setTranslation("&Help", tr("&Help"));
	
	toolbars.setTranslation("File", tr("File"));
	toolbars.setTranslation("Edit", tr("Edit"));
	toolbars.setTranslation("Search", tr("Search"));
	toolbars.setTranslation("Project", tr("Project"));
	toolbars.setTranslation("Build", tr("Build"));
	toolbars.setTranslation("Debug", tr("Debug"));
	toolbars.setTranslation("View", tr("View"));
	toolbars.setTranslation("Help", tr("Help"));
	
	COMPONENT(shortcutManager)->translateContext("Edit", tr("Edit"));
	COMPONENT(shortcutManager)->translateContext("View", tr("View"));
	COMPONENT(shortcutManager)->translateContext("View/Perspectives", tr("View/Perspectives"));
	COMPONENT(shortcutManager)->translateContext("File", tr("File"));
	COMPONENT(shortcutManager)->translateContext("Project", tr("Project"));
	COMPONENT(shortcutManager)->translateContext("Tools", tr("Tools"));
	COMPONENT(shortcutManager)->translateContext("Help", tr("Help"));
	
	//COMPONENT(shortcutManager)->translateContext("Completion", tr("Completion"));
	
	updateGUI();
}

void EdyukGUI::setupActions()
{
	aReopen = 0;
	
	connect(aNew, SIGNAL( triggered() ),
			this, SLOT  ( fileNew() ) );
	connect(aOpen, SIGNAL( triggered() ),
			this , SLOT  ( fileOpen() ) );
	
	EDYUK_SHORTCUT(aNew, "File", "Ctrl+N");
	EDYUK_SHORTCUT(aOpen, "File", "Ctrl+O");
	EDYUK_SHORTCUT(aSave, "File", "Ctrl+S");
	EDYUK_SHORTCUT(aSaveAs, "File", "Ctrl+F12");
	EDYUK_SHORTCUT(aSaveAll, "File", "");
	EDYUK_SHORTCUT(aClose, "File", "Ctrl+F4");
	EDYUK_SHORTCUT(aCloseAll, "File", "");
	EDYUK_SHORTCUT(aPrint, "File", "Ctrl+P");
	EDYUK_SHORTCUT(aExit, "File", "Ctrl+Q");
	
	EDYUK_SHORTCUT(aTile, "View", "");
	EDYUK_SHORTCUT(aCascade, "View", "");
	
	aSwapHeaderSource = new QAction(tr("Swap header|source"), this);
	connect(aSwapHeaderSource	, SIGNAL( triggered() ),
			this				, SLOT  ( swapHeaderSource() ) );
	
	EDYUK_SHORTCUT(aSwapHeaderSource, "Edit", "F11");
	
	addAction(aSwapHeaderSource);
	
	aFocusCurrent = new QAction(tr("Focus current editor"), this);
	connect(aFocusCurrent	, SIGNAL( triggered() ),
			this			, SLOT  ( focusCurrentClient() ) );
	
	EDYUK_SHORTCUT(aFocusCurrent, "Edit", "Esc");
	
	addAction(aFocusCurrent);
	
	QMenu *m = new QMenu(tr("&Style"));
	
	QActionGroup *g = new QActionGroup(this);
	g->setExclusive(true);
	connect(g	, SIGNAL( triggered(QAction*) ),
			this, SLOT  ( styleChanged(QAction*) ) );
	
	QString old = QSettingsClient(pServer, "gui").value("style").toString();
	QStringList styles = QStyleFactory::keys();
	
	foreach ( QString s, styles )
	{
		QAction *a = g->addAction(s);
		a->setCheckable(true);
		
		if ( s == old )
			a->trigger();
	}
	
	m->addActions(g->actions());
	
	aStyle = m->menuAction();
	aStyle->setIcon(QIcon(":/style.png"));
	aStyle->setMenuRole(QAction::PreferencesRole);
	
	aProjectDetailed = new QAction(QIcon(":/.png"), tr("Detailed view"), this);
	EDYUK_SHORTCUT(aProjectDetailed, "Project", "");
	//aProjectDetailed->setEnabled(true);
	aProjectDetailed->setCheckable(true);
	connect(aProjectDetailed, SIGNAL( toggled(bool) ),
			this			, SLOT  ( setProjectDetailed(bool) ) );
	
	aProjectOpen = new QAction(QIcon(":/open.png"), tr("Open project"), this);
	EDYUK_SHORTCUT(aProjectOpen, "Project", "");
	//aProjectOpen->setEnabled(true);
	connect(aProjectOpen, SIGNAL( triggered() ),
			this		, SLOT  ( projectOpen() ) );
	
	aProjectNew = new QAction(QIcon(":/.png"), tr("Create new project"), this);
	EDYUK_SHORTCUT(aProjectNew, "Project", "");
	//aProjectNew->setEnabled(true);
	connect(aProjectNew	, SIGNAL( triggered() ),
			this		, SLOT  ( projectNew() ) );
	
	aProjectSave = new QAction(QIcon(":/save.png"), tr("Save project"), this);
	EDYUK_SHORTCUT(aProjectSave, "Project", "");
	aProjectSave->setEnabled(false);
	connect(aProjectSave, SIGNAL( triggered() ),
			this		, SLOT  ( projectSave() ) );
	
	aProjectClose = new QAction(QIcon(":/close.png"), tr("Close project"), this);
	EDYUK_SHORTCUT(aProjectClose, "Project", "");
	aProjectClose->setEnabled(false);
	connect(aProjectClose	, SIGNAL( triggered() ),
			this			, SLOT  ( projectClose() ) );
	
	aProjectNewFile = new QAction(QIcon(":/.png"), tr("Create new file"), this);
	EDYUK_SHORTCUT(aProjectNewFile, "Project", "");
	aProjectNewFile->setEnabled(false);
	connect(aProjectNewFile	, SIGNAL( triggered() ),
			this			, SLOT  ( projectNewFile() ) );
	
	aProjectAdd = new QAction(QIcon(":/add.png"), tr("Add to project"), this);
	EDYUK_SHORTCUT(aProjectAdd, "Project", "");
	aProjectAdd->setEnabled(false);
	connect(aProjectAdd	, SIGNAL( triggered() ),
			this		, SLOT  ( projectAdd() ) );
	
	aProjectRemove = new QAction(	QIcon(":/remove.png"),
									tr("Remove from project"), this);
	EDYUK_SHORTCUT(aProjectRemove, "Project", "");
	aProjectRemove->setEnabled(false);
	connect(aProjectRemove	, SIGNAL( triggered() ),
			this			, SLOT  ( projectRemove() ) );
	
	aProjectOptions = new QAction(	QIcon(":/project_options.png"),
									tr("Project options"), this);
	EDYUK_SHORTCUT(aProjectOptions, "Project", "");
	aProjectOptions->setEnabled(false);
	connect(aProjectOptions	, SIGNAL( triggered() ),
			this			, SLOT  ( projectOptions() ) );
	
	aOptions = new QAction(QIcon(":/configure.png"), tr("Configure &Edyuk"), this);
	aOptions->setMenuRole(QAction::PreferencesRole);
	EDYUK_SHORTCUT(aOptions, "Tools", "");
	connect(aOptions, SIGNAL( triggered() ),
			pServer	, SLOT  ( configure() ) );
	
	
	aShortcuts = new QAction(QIcon(":/shortcuts.png"),
							tr("Configure &shortcuts"), this);
	aShortcuts->setMenuRole(QAction::PreferencesRole);
	EDYUK_SHORTCUT(aShortcuts, "Tools", "");
	//aShortcuts->setEnabled(false);
	connect(aShortcuts					, SIGNAL( triggered() ),
			COMPONENT(shortcutManager)	, SLOT  ( configure() ) );
	
	
	aTools = new QAction(QIcon(":/tools.png"), tr("Configure &tools"), this);
	aTools->setMenuRole(QAction::PreferencesRole);
	EDYUK_SHORTCUT(aTools, "Tools", "");
	//aTools->setEnabled(false);
	connect(aTools					, SIGNAL( triggered() ),
			COMPONENT(toolsManager)	, SLOT  ( configure() ) );
	
	aLanguage = 0;
	
	aAboutEdyuk = new QAction(QIcon(":/.png"), tr("&About Edyuk"), this);
	aAboutEdyuk->setMenuRole(QAction::AboutRole);
	EDYUK_SHORTCUT(aAboutEdyuk, "Help", "");
	connect(aAboutEdyuk	, SIGNAL( triggered() ),
			this		, SLOT  ( about() ) );
	
	aEdyukHelp = new QAction(QIcon(":/.png"), tr("&Help on Edyuk"), this);
	aEdyukHelp->setMenuRole(QAction::AboutRole);
	EDYUK_SHORTCUT(aEdyukHelp, "Help", "");
	connect(aEdyukHelp	, SIGNAL( triggered() ),
			this		, SLOT  ( help() ) );
	
}

void EdyukGUI::setRecentAction(QAction *a)
{
	int i = 3;
	
	if ( aReopen )
	{
		i = menus["&File"]->indexOf(aReopen);
		menus["&File"]->removeAction(aReopen);
		toolbars["File"]->removeAction(aReopen);
	}
	
	aReopen = a;
	
	if ( aReopen && i != - 1 )
	{
		menus["&File"]->insertAction(aReopen, i);
		toolbars["File"]->insertAction(aReopen, i);
	}
}

void EdyukGUI::setLanguageAction(QAction *a)
{
	int i = 6;
	
	if ( aLanguage )
	{
		i = menus["&Tools"]->indexOf(aLanguage);
		menus["&Tools"]->removeAction(aLanguage);
	}
	
	aLanguage = a;
	
	if ( aLanguage && i != - 1 )
	{
		menus["&Tools"]->insertAction(aLanguage, i);
	}
}

void EdyukGUI::setDefaultPerspective()
{
	QSettingsClient s(pServer, "gui");
	
	if ( s.value("mode").toBool() )
	{
		QString perspec = s.value("perspective").toString();
		
		if ( perspec.count() )
			return setPerspective(perspec);
		
		QStringList l = perspectiveNames();
		
		if ( l.contains("default (C++/Qt4)") )
			return setPerspective("default (C++/Qt4)");
		
		if ( l.count() )
			setPerspective(l.at(0));
	}
}

void EdyukGUI::setPerspectives(QList<qmdiPerspective*> l)
{
	#ifdef _EDYUK_DEBUG_
	qDebug("adding perspectives");
	#endif
	
	foreach ( qmdiPerspective *p, perspec )
	{
		if ( !l.contains(p) )
			removePerspective(p);
		else
			l.removeAll(p);
	}
	
	foreach ( qmdiPerspective *p, l )
		addPerspective(p);
	
}

void EdyukGUI::translateActions()
{
	aFocusCurrent->setText(tr("Focus current editor"));
	aSwapHeaderSource->setText(tr("Swap header|source"));
	
	aStyle->setText(tr("&Style"));
	
	aProjectDetailed->setText(tr("Detailed view"));
	aProjectOpen->setText(tr("Open project"));
	aProjectNew->setText(tr("Create new project"));
	aProjectSave->setText(tr("Save project"));
	aProjectClose->setText(tr("Close project"));
	aProjectNewFile->setText(tr("Create new file"));
	aProjectAdd->setText(tr("Add to project"));
	aProjectRemove->setText(tr("Remove from project"));
	aProjectOptions->setText(tr("Project options"));
	
	aOptions->setText(tr("Configure &Edyuk"));
	aShortcuts->setText(tr("Configure &shortcuts"));
	aTools->setText(tr("Configure &tools"));
	
	aAboutEdyuk->setText(tr("&About Edyuk"));
	aEdyukHelp->setText(tr("&Help on Edyuk"));
}

void EdyukGUI::setupMenu()
{
	menus.clear();
	toolbars.clear();
	
	menus["&File"]->addAction(aNew);
	menus["&File"]->addSeparator();
	menus["&File"]->addAction(aOpen);
	
	if ( aReopen )
		menus["&File"]->addAction(aReopen);
	
	menus["&File"]->addSeparator();
	menus["&File"]->addAction(aSave);
	menus["&File"]->addAction(aSaveAs);
	menus["&File"]->addAction(aSaveAll);
	menus["&File"]->addSeparator();
	menus["&File"]->addAction(aClose);
	menus["&File"]->addAction(aCloseAll);
	menus["&File"]->addSeparator();
	menus["&File"]->addAction(aPrint);
	menus["&File"]->addSeparator();
	menus["&File"]->addAction(aExit);
	
	
	menus["&Edit"];		//this is just a placeholder
							//qmdilib sorts menuitems by creation order so,
							//we can easily decide the menubar's look
	
	// placeholder
	menus["&Search"]->addAction(aSwapHeaderSource);
	
	menus["&View"]->addAction(aPerspective);
	menus["&View"]->addSeparator();
	menus["&View"]->addAction(aTile);
	menus["&View"]->addAction(aCascade);
	menus["&View"]->addSeparator();
	menus["&View"]->addAction(aStyle);
	
	// placeholder
	menus["&Project"]->addAction(aProjectDetailed);
	menus["&Project"]->addSeparator();
	menus["&Project"]->addAction(aProjectOpen);
	menus["&Project"]->addAction(aProjectNew);
	menus["&Project"]->addAction(aProjectSave);
	menus["&Project"]->addAction(aProjectClose);
	menus["&Project"]->addSeparator();
	menus["&Project"]->addAction(aProjectNewFile);
	menus["&Project"]->addAction(aProjectAdd);
	menus["&Project"]->addAction(aProjectRemove);
	menus["&Project"]->addSeparator();
	menus["&Project"]->addAction(aProjectOptions);
	
	// placeholder
	menus["&Build"];
	
	// placeholder
	menus["&Debug"];
	
	menus["&Tools"]->addAction(aOptions);
	menus["&Tools"]->addSeparator();
	menus["&Tools"]->addAction(aShortcuts);
	menus["&Tools"]->addAction(aTools);
	menus["&Tools"]->addSeparator();
	
	if ( aLanguage )
	{
		menus["&Tools"]->addAction(aLanguage);
		menus["&Tools"]->addSeparator();
	}
	
	menus["&Help"]->addAction(aAboutEdyuk);
	menus["&Help"]->addAction(aEdyukHelp);
	
	
	toolbars["File"]->addAction(aNew);
	toolbars["File"]->addSeparator();
	toolbars["File"]->addAction(aOpen);
	toolbars["File"]->addAction(aReopen);
	toolbars["File"]->addSeparator();
	toolbars["File"]->addAction(aSave);
	toolbars["File"]->addAction(aSaveAs);
	toolbars["File"]->addAction(aSaveAll);
	toolbars["File"]->addSeparator();
	toolbars["File"]->addAction(aClose);
	toolbars["File"]->addAction(aCloseAll);
	toolbars["File"]->addSeparator();
	toolbars["File"]->addAction(aPrint);
	
	toolbars["Perspectives"];	//placeholder
	
	toolbars["Edit"];			//placeholder
	
	toolbars["Search"];			//placeholder
	
	updateGUI();
	addToolBarBreak();
	
	toolbars["Project"];		//placeholder
	
	toolbars["Build"];
	
	toolbars["Debug"];
	
	updateGUI();
}

void EdyukGUI::swapHeaderSource()
{
	QWidget *w = activeWindow();
	qmdiClient *c = dynamic_cast<qmdiClient*>(w);
	
	if ( !c )
		return;
	
	QString fn = c->fileName();
	
	QFileInfo info(fn);
	QDir d(info.absolutePath());
	
	static QStringList
		headers = QStringList()
			<< "h"
			<< "hpp"
			<< "hxx",
		sources = QStringList()
			<< "c"
			<< "cc"
			<< "cpp"
			<< "cxx"
			;
	
	QString equiv, tmp, bn = info.baseName(), suf = info.suffix();
	
	if ( headers.contains(suf) )
	{
		foreach ( QString s, sources )
		{
			tmp = bn + "." + s;
			
			if ( d.exists(tmp) )
			{
				equiv = d.absoluteFilePath(tmp);
				break;
			}
		}
	} else if ( sources.contains(suf) ) {
		foreach ( QString s, headers )
		{
			tmp = bn + "." + s;
			
			if ( d.exists(tmp) )
			{
				equiv = d.absoluteFilePath(tmp);
				break;
			}
		}
	} else {
		QStringList alter;
		QFileInfoList entries = d.entryInfoList(QDir::Files | QDir::Readable);
		
		foreach ( const QFileInfo& i, entries )
		{
			if ( i.baseName() == bn && i.suffix() != suf )
				alter << i.fileName();
		}
		
		if ( alter.count() == 1 )
		{
			equiv = d.absoluteFilePath(alter.at(0));
		} else {
			
		}
	}
	
	if ( QFile::exists(equiv) )
	{
		w = fileOpen(equiv);
	}
}

void EdyukGUI::styleChanged(QAction *ac)
{
	if ( !ac )
		return;
	
	QActionGroup *g = ac->actionGroup();
	
	if ( !g )
		return;
	
	QAction *a = g->checkedAction();
	
	if ( !a )
		return;
	
	QString name = a->text();
	
	QSettingsClient(pServer, "gui").setValue("style", name);
	QStyle *style = QStyleFactory::create(name);
	
	if ( style )
	{
		QApplication::setStyle(style);
		QApplication::setPalette(style->standardPalette());
	}
}

void EdyukGUI::perspectiveChanged(qmdiPerspective *p)
{
	Q_UNUSED(p)
}

void EdyukGUI::perspectiveAboutToChange(qmdiPerspective *p)
{
	Q_UNUSED(p)
}

void EdyukGUI::addPerspective(qmdiPerspective *p)
{
	if ( !p )
		return;
	
	qmdiMainWindow::addPerspective(p);
	
	QSettingsClient s(pServer, "gui/" + p->name());
	perspecStates.last() = s.value("state").toByteArray();
	
	toolbars["Perspectives"]->addAction(actions.last());
	
	EDYUK_SHORTCUT(actions.last(), "View/Perspectives", "");
}

void EdyukGUI::removePerspective(qmdiPerspective *p)
{
	if ( !p )
		return;
	
	int index = perspec.indexOf(p);
	
	if ( index == -1 )
		return;	
	
	QSettingsClient s(pServer, "gui/" + p->name());
	s.setValue("state", perspecStates[index]);
	
	DEV_SHORTCUT->unregisterAction(actions[index]);
	
	qmdiMainWindow::removePerspective(p);
}

void EdyukGUI::mergeExtraClient(qmdiClient *c, bool on)
{
	if ( !c )
		return;
	
	if ( on )
		mergeClient(c);
	else
		unmergeClient(c);
	
	updateGUI();
}

void EdyukGUI::insertExtraDockWidget(QWidget *w, bool on)
{
	if ( !w )
		return;
	
	if ( on )
		m_logDock->addExtraWidget(w);
	else
		m_logDock->removeExtraWidget(w);
	
}

/*!
	\brief Add a code parser to the collection
	\note The parser is added to the collection of the code model
	managed by the managerDock()
*/
void EdyukGUI::addCodeParser(QCodeParser *p)
{
	m_managerDock->addCodeParser(p);
}

/*!
	\brief Add a project parser to the collection
	\note The parser is added to the collection of the project model
	managed by the managerDock()
*/
void EdyukGUI::addProjectParser(QProjectParser *p)
{
	m_managerDock->addProjectParser(p);
}

/*!
	\brief Add a completion engine to the collection
	\note the engine is added to the internal language factory of the
	client factory (which is a QEditorFactory...)
*/
void EdyukGUI::addCompletionEngine(QCodeCompletionEngine *e)
{
	if ( !e )
		return;
	
	e->setCodeModel(m_managerDock->codeModel());
	
	QEditorFactory *f = qobject_cast<QEditorFactory*>(clientFactory());
	
	if ( !f || !f->languageFactory() )
		return;
	
	f->languageFactory()->addCompletionEngine(e);
}

/*!
	\brief Add a language definition to the collection
	\note the definition is added to the internal language factory of the
	client factory (which is a QEditorFactory...)
*/
void EdyukGUI::addLanguageDefinition(QLanguageDefinition *l)
{
	if ( !l )
		return;
	
	QEditorFactory *f = qobject_cast<QEditorFactory*>(clientFactory());
	
	if ( !f || !f->languageFactory() )
		return;
	
	f->languageFactory()->addLanguageDefinition(l);
}

/*!
	\return The currently active project or an empty string
*/
QString EdyukGUI::activeProject() const
{
	return m_managerDock->activeProject();
}

/*!
	\return The list of opened projects
*/
QStringList EdyukGUI::openedProjects() const
{
	return m_managerDock->openedProjects();
}

/*!
	\return The list of files owned by a given project
	\param project the file path of the project to search files in
	
	\note This function will NEVER open a project...
*/
QStringList EdyukGUI::files(const QString& project) const
{
	return m_managerDock->files(project);
}

/*!
	\return The (opened) project owning a given file
	\param file filename to look for inside projects
*/
QString EdyukGUI::ownerProject(const QString& file) const
{
	return m_managerDock->ownerProject(file);
}

void EdyukGUI::message(const QString& s)
{
	if ( QSystemTrayIcon::supportsMessages() && !isVisible() )
	{
		m_trayIcon->showMessage("Edyuk", s);
	} else {
		if ( !isVisible() )
			show();
		
		raise();
	}
}

/*!
	\brief Create a new file (from templates)
*/
void EdyukGUI::fileNew()
{
	EdyukCreateDialog dlg(COMPONENT(templateManager));
	
	dlg.setFilter(EdyukCreateDialog::All);
	dlg.exec();
}

static QString last;

/*!
	\brief Pop up a file open dialog
*/
void EdyukGUI::fileOpen()
{
	#if 0
	QFileDialog dlg;
	
	dlg.setDirectory(last.isEmpty() ? QDir::current() : last);
	dlg.setFilters( filters().split(";;") );
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFiles);
	dlg.setWindowTitle(tr("Open file(s)..."));
	
	if ( dlg.exec() )
	{
		last = dlg.directory().path();
		
		foreach ( QString name, dlg.selectedFiles() )
			fileOpen(name);
	}
	#endif
	
	QStringList files = QFileDialog::getOpenFileNames(this, tr("Open file(s)..."), last);
	
	foreach ( QString name, files )
		fileOpen(name);
	
	if ( files.count() )
		last = QFileInfo(files.last()).absolutePath();
}

/*!
	\brief Create a new project
*/
void EdyukGUI::projectNew()
{
	EdyukCreateDialog dlg(COMPONENT(templateManager));
	
	dlg.setFilter(EdyukCreateDialog::Project);
	dlg.exec();
}

/*!
	\brief Pop up a project open dialog
*/
void EdyukGUI::projectOpen()
{
	QFileDialog dlg;
	
	dlg.setDirectory(last.isEmpty() ? QDir::current() : last);
	dlg.setFilters(m_managerDock->projectFilters());
	dlg.setAcceptMode(QFileDialog::AcceptOpen);
	dlg.setFileMode(QFileDialog::ExistingFiles);
	dlg.setWindowTitle(tr("Open project(s)..."));
	
	if ( dlg.exec() )
	{
		last = dlg.directory().path();
		
		foreach ( QString name, dlg.selectedFiles() )
			projectOpen(name);
	}
}

/*!
	\brief Toggle the "detailed view" mode
*/
void EdyukGUI::setProjectDetailed(bool y)
{
	m_managerDock->setProjectViewDetailLevel(y ? 10 : 0);
}

/*!
	\brief Open the given project
	\param name path of the project file to open
*/
bool EdyukGUI::projectOpen(const QString& name)
{
	return m_managerDock->openProject(name);
}

/*!
	\brief Save the active project
*/
void EdyukGUI::projectSave()
{
	projectSave(activeProject());
}

/*!
	\brief Save the given project
	\param name project to save
*/
void EdyukGUI::projectSave(const QString& name)
{
	m_managerDock->saveProject(name);
}

/*!
	\brief Close the active project
*/
void EdyukGUI::projectClose()
{
	projectClose(activeProject());
}

/*!
	\brief Close the given project
	\param name project to close
	\return whether closing succeeded
*/
bool EdyukGUI::projectClose(const QString& name)
{
	return m_managerDock->closeProject(name);
}

/*!
	\brief Add file(s) to the active project
*/
void EdyukGUI::projectAdd()
{
	projectAdd(activeProject());
}

/*!
	\brief Add file(s) to the given project
	\param project project to which the file(s) will be added
*/
void EdyukGUI::projectAdd(const QString& project)
{
	m_managerDock->projectAddFiles(project);
}

/*!
	\brief Add file(s) to the given project
	\param project project to which the file(s) will be added
*/
void EdyukGUI::projectAdd(const QString& project, const QStringList& files)
{
	m_managerDock->projectAddFiles(project, files);
}

/*!
	\brief Remove file(s) from the active project
*/
void EdyukGUI::projectRemove()
{
	projectRemove(activeProject());
}

/*!
	\brief Remove file(s) from the given project
	\param project project from which the file(s) will be removed
*/
void EdyukGUI::projectRemove(const QString& project)
{
	Q_UNUSED(project)
}

/*!
	\brief Create a new file and directly add it into the active project
*/
void EdyukGUI::projectNewFile()
{
	projectNewFile(activeProject());
}

/*!
	\brief Create a new file and directly add it into the given project
	\param project project to which the file will be added
*/
void EdyukGUI::projectNewFile(const QString& project)
{
	Q_UNUSED(project)
}

/*!
	\brief Pop up an option dialog for the active project
*/
void EdyukGUI::projectOptions()
{
	projectOptions(activeProject());
}

/*!
	\brief Pop up an option dialog for the given project
	\param project project to which the file will be added
*/
void EdyukGUI::projectOptions(const QString& name)
{
	m_managerDock->projectOptions(name);
}

/*!
	\brief Display an "About Edyuk" dialog
*/
void EdyukGUI::about()
{
	EdyukAboutDialog dlg(this);
	
	dlg.exec();
}

/*!
	\brief Display help on Edyuk (doen't work yet... there's no manual anyway...)
*/
void EdyukGUI::help()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(QCoreApplication::applicationDirPath() + "/doc/README.htm"));
}

QString EdyukGUI::filters()
{
	// TODO : fetch filters from language def loaded in the editor factory...
	return qmdiMainWindow::filters();
}

void EdyukGUI::projectsOpened(bool y)
{
	aProjectSave->setEnabled(y);
	aProjectClose->setEnabled(y);
	//aProjectNewFile->setEnabled(y);
	aProjectAdd->setEnabled(y);
	//aProjectRemove->setEnabled(y);
	aProjectOptions->setEnabled(y);
}

void EdyukGUI::refreshClassTree()
{
	m_managerDock->processFileChanges();
}

void EdyukGUI::notifyFileChange(const QStringList& l)
{
	m_managerDock->filesChanged(l);
}

void EdyukGUI::refreshTargetList()
{
	#if 0
	QProject *p = m_managerDock->projectView()->activeProject();
	
	QString s;
	QStringList t;
	
	if ( p )
	{
		s = p->name();
		t = p->query("TARGET_PATH").split(",");
	} else {
		
	}
	
	//qDebug("project %s", qPrintable(s));
	
	QDebuggingEngine::instance()->setTargets(s, t);
	QBuildEngine::instance()->setTargetList(t);
	
	#endif
	
	//qDebug("refreshing...");
	
	QDebuggingEngine::instance()->setTarget(QBuildEngine::instance()->activeTarget());
}

void EdyukGUI::buildTaskAboutToStart()
{
	QSettingsClient autosave(pServer, "autosave");
	int as_build = autosave.value("build", Edyuk::AlwaysAsk).toInt();
	
	QStringList mf = modifiedFiles();
	QStringList mp = m_managerDock->modifiedProjects();
	
	if ( as_build == Edyuk::AlwaysAsk )
	{
		if ( mf.isEmpty() && mp.isEmpty() )
			return;
		
		// TODO : Kate-like save selector
		
		QStringList overlap;
		
		foreach ( QString f, mf )
			if ( mp.contains(f) )
				overlap << f;
		
		QString msg = tr(
							"%1 files have been modified.\n"
							"Do you want to commit changes in these %2 project(s) and %3 source file(s)?"
						)
						.arg(mp.count() + mf.count())
						.arg(mp.count())
						.arg(mf.count());
		
		if ( overlap.count() )
		{
			msg += "\n\n";
			msg += tr("%1 overlaps (projects opened and modified as source files) :\n%2")
						.arg(overlap.count())
						.arg(overlap.join("\n"));
		}
		
		int ret = QMessageBox::question(
									0,
									tr("About to build"),
									msg,
									QMessageBox::Save | QMessageBox::Ignore
								);
		
		if ( ret == QMessageBox::Save )
			as_build = Edyuk::SaveWithoutAsking;
	}
	
	if ( as_build == Edyuk::SaveWithoutAsking )
	{
		saveFiles(mf);
		
		m_managerDock->saveAllProjects();
		//saveAll();
	}
}

void EdyukGUI::saveAll()
{
	qmdiMainWindow::saveAll();
	
	m_managerDock->saveAllProjects();
}

void EdyukGUI::activeProjectChanged(QProject *p)
{
	QString s, b;
	
	if ( p )
	{
		s = p->name();
		b = p->backend();
	}
	
	QBuildEngine::instance()->setActiveSource(s, b);
	
	QDebuggingEngine::instance()->setSource(s);
	QDebuggingEngine::instance()->setTarget(QBuildEngine::instance()->activeTarget());
}

void EdyukGUI::execTargetChanged(const QString& target)
{
	QDebuggingEngine::instance()->setTarget(target);
}

void EdyukGUI::buildModeChanged(const QString& mode)
{
	#if 0
	QProject *p = m_managerDock->projectView()->activeProject();
	
	if ( !p )
		return QBuildEngine::instance()
							->setActiveTarget(
									QString(),
									QString(),
									QStringList()
								);
	
	QString prop("TARGET_PATH");
	
	if ( mode == "release" )
		prop += "_RELEASE";
	else if ( mode == "debug" )
		prop += "_DEBUG";
	
	QBuildEngine::instance()
					->setActiveTarget(
							p->name(),
							p->backend(),
							p->query(prop).split(",")
						);
	
	QDebuggingEngine::instance()->setActiveTarget(QBuildEngine::instance()->activeTarget());
	#endif
}

void EdyukGUI::toolsChanged(QActionGroup *g)
{
	menus["&Tools"]->clear();
	
	menus["&Tools"]->addAction(aOptions);
	//menus["&Tools"]->addAction(aPlugins);
	menus["&Tools"]->addSeparator();
	menus["&Tools"]->addAction(aShortcuts);
	menus["&Tools"]->addAction(aTools);
	menus["&Tools"]->addSeparator();
	menus["&Tools"]->addAction(aLanguage);
	menus["&Tools"]->addSeparator();
	
	foreach ( QAction *a, g->actions() )
		menus["&Tools"]->addAction(a);
	
	updateGUI();
}

void EdyukGUI::trayIconActivated(QSystemTrayIcon::ActivationReason r)
{
	if ( r == QSystemTrayIcon::Trigger )
	{
		if ( isVisible() )
		{
			hide();
		} else {
			show();
			raise();
		}
	}
}

void EdyukGUI::closeEvent(QCloseEvent *e)
{
	QSettingsClient tray(pServer, "tray");
	
	if (
			m_trayIcon
		&&
			isVisible()
		&&
			!QCoreApplication::closingDown()
		&&
			tray.value("enabled", EDYUK_TRAY_DEFAULT).toBool()
		)
	{
		if ( tray.value("warn", true).toBool() )
		{
			int ret = QMessageBox::information(
										0,
										tr("Minimizing to Tray"),
										tr(
											"Edyuk is still running but minimized to system tray.\n"
											"This allow you to load files much faster later on.\n\n"
											"You can disable this feature in Edyuk settings.\n\n"
											"Click on the ignore button to hide this message in the future."
										),
										QMessageBox::Ok | QMessageBox::Ignore
								);
			
			if ( ret == QMessageBox::Ignore )
				tray.setValue("warn", false);
		}
		
		e->ignore();
		hide();
		
		return;
	}
	
	// save all data
	QSettingsClient autosave(pServer, "autosave");
	int as_exit = autosave.value("exit", Edyuk::AlwaysAsk).toInt();
	
	if ( as_exit == Edyuk::SaveWithoutAsking )
	{
		saveAll();
	} else if ( as_exit == Edyuk::DiscardChanges ) {
		m_managerDock->closeAllProjects();
		closeAll(true);
	}
	
	if ( !tryClose(0) )
	{
		e->ignore();
		return;
	}
	
	e->accept();
}

void EdyukGUI::forceClose()
{
	if ( !tryClose(0) )
		return;
	
	delete m_trayIcon;
	m_trayIcon = 0;
	
	pServer = 0;
	
	close();
}

bool EdyukGUI::tryClose(QSessionManager *mgr)
{
	if ( !pServer )
		return true;
	
	QBuildEngine::instance()->abort();
	QDebuggingEngine::instance()->terminateSession();
	
	QString cf = currentFile();
	QStringList ofl = openedFiles();
	
	QString cp = activeProject();
	QStringList opl = openedProjects();
	
	if ( !mgr || mgr->allowsInteraction() )
	{
		if ( m_managerDock->tryCloseAllProjects() )
		{
			return false;
		}
		
		#ifdef _EDYUK_DEBUG_
		qDebug() << "closing all ..." ;
		#endif
		
		if ( !closeAll(false) )
		{
			return false;
		}
		
		if ( mgr )
			mgr->release();
	}
	
	m_managerDock->closeAllProjects();
	
	QLineMarksInfoCenter::instance()->saveMarks(
							Edyuk::settingsPath()
							+ QDir::separator()
							+ "marks.dump");
	
	#ifdef _EDYUK_DEBUG_
	qDebug() << "current file : " << cf;
	qDebug() << "opened files : " << ofl.join("\n\t");
	qDebug() << "current project : " << cp;
	qDebug() << "opened projects : " << opl.join("\n\t");
	#endif
	
	QSettingsClient c(pServer, "opened");
	
	c.setValue("file", cf);
	c.setValue("files", ofl);
	
	c.setValue("project", cp);
	c.setValue("projects", opl);
	
	QSettingsClient s(pServer, "gui");
	
	s.setValue("winstate", (int)windowState());
	s.setValue("title", windowTitle());
	s.setValue("width", width());
	s.setValue("height", height());
	s.setValue("pos", pos());
	
	QString name;
	qmdiPerspective *p = perspective();
	
	if ( p )
	{
		name = p->name();
		perspecStates[ perspec.indexOf( perspective() ) ] = saveState();
	} else
		name = QString();
	
	s.setValue("perspective", name);
	
	for ( int i = 0; i < perspec.count(); ++i )
		s.setValue(perspec[i]->name() + "/state", perspecStates[i]);
	
	return true;
}

/*!	@} */
