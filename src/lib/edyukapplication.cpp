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

#include "edyukapplication.h"

#include "version.h"

#include "edyukgui.h"
#include "edyuklogdock.h"
#include "edyuksettings.h"
#include "edyuktranslator.h"
#include "edyuktoolsmanager.h"
#include "edyuktemplatemanager.h"

#include "qbuildengine.h"
#include "qdebuggingengine.h"

#include "qpluginconfig.h"
#include "qpluginmanager.h"

#include "qshortcutmanager.h"

#include "qsnippetmanager.h"

#include "qeditor.h"
#include "qeditorfactory.h"
#include "qcodecompletionengine.h"
#include "qlinemarksinfocenter.h"

#ifdef _EDYUK_DEBUG_
	#define Q_INIT_SPLASH(s)
	#define Q_SPLASH(s) qDebug(s);
	#define Q_HOOK_SPLASH(w)
	#define Q_DELETE_SPLASH
#else
	#define Q_INIT_SPLASH(s)												\
		pSplash = new QSplashScreen(	QPixmap(s),				\
											Qt::WindowStaysOnTopHint);		\
		pSplash->show();
	
	#define Q_SPLASH(s)														\
		pSplash->showMessage(s, Qt::AlignLeft | Qt::AlignBottom, Qt::white);\
		processEvents();
	
	#define Q_HOOK_SPLASH(w) pSplash->finish(w);
	
	#define Q_DELETE_SPLASH pSplash->deleteLater(); pSplash = 0;
#endif

#include <QDir>
#include <QFile>
#include <QIcon>
#include <QEvent>
#include <QTimer>
#include <QTextEdit>
#include <QTextStream>
#include <QTextCursor>
#include <QTextDocument>
#include <QSplashScreen>
#include <QSessionManager>

/*!
	\var clog
	Text stream that points to edyuk log file.
*/

QTextStream clog;
QFile *pEdyukLogFile = 0;
QSplashScreen *pSplash = 0;

void EdyukMessageHandler(QtMsgType t, const char *msg)
{
	static QString _buffer;
	
	static const char *type[] =
	{
		"",
		"Edyuk::Warning  : ",
		"Edyuk::Critical : ",
		"Edyuk::Fatal    : "
	};
	
	if ( !qstrncmp(msg, "status:", 7) && pSplash )
	{
		pSplash->showMessage(QString::fromLocal8Bit(msg + 7), Qt::AlignRight | Qt::AlignBottom, Qt::white);
		qApp->processEvents();
	}
	
	//fprintf(stderr, "%s%s\n", type[t], msg);
	fprintf(stdout, "%s%s\n", type[t], msg);
	fflush(stdout);
	clog << type[t] << msg << endl;
	
	
	if ( EdyukApplication::Instance()->loggerReady() )
	{
		EdyukApplication::Instance()->log(_buffer + msg);
		_buffer.clear();
	} else {
		//_buffer += type[t];
		_buffer += msg;
		_buffer += '\n';
	}
	
	// some KDE parts appear to send critical messages just for fun
	// (Sonnet designer plugin for instance panics when no dictionnary
	// is found...
	if ( (t & 0x02) && !QByteArray(msg).contains("kde") )
	{
		//fprintf(stderr, "aborting...\n");
		fprintf(stdout, "aborting...\n");
		
		#if defined(Q_OS_UNIX) && defined(_EDYUK_DEBUG_)
		abort(); // trap; generates core dump
		#else
		exit(-1); // goodbye cruel world
		#endif
	}
}

/*
	Plugin handlers
*/

void BuilderHandler(void *p)
{
	QBuildEngine::instance()->addBuilder(static_cast<QBuilder*>(p));
}

void DebuggerHandler(void *p)
{
	QDebuggingEngine::instance()->addDebugger(static_cast<QDebugger*>(p));
}

void ClientHandler(void *p)
{
	// TODO : manage a list of client and enable/disable them at will
	COMPONENT(gui)->mergeExtraClient(static_cast<qmdiClient*>(p), true);
}

void PerspectiveHandler(void *p)
{
	COMPONENT(gui)->addPerspective(static_cast<qmdiPerspective*>(p));
}

void InputBindingHandler(void *p)
{
	QEditor::registerInputBinding((QEditorInputBindingInterface*)p);
}

void CompletionEngineHandler(void *p)
{
	QCodeCompletionEngine *eng = static_cast<QCodeCompletionEngine*>(p);
	
	//EdyukApplication::Instance()->registerEngineTrigger(eng);
	
	EdyukApplication::Instance()->connect(	eng, SIGNAL( cloned(QCodeCompletionEngine*) ),
											SLOT( registerEngineTrigger(QCodeCompletionEngine*) ) );
	
	COMPONENT(gui)->addCompletionEngine(eng);
}

void LanguageDefinitionHandler(void *p)
{
	COMPONENT(gui)->addLanguageDefinition(static_cast<QLanguageDefinition*>(p));
}

class QCodeParser;

void CodeParserHandler(void *p)
{
	COMPONENT(gui)->addCodeParser(static_cast<QCodeParser*>(p));
}

class QProjectParser;

void ProjectParserHandler(void *p)
{
	COMPONENT(gui)->addProjectParser(static_cast<QProjectParser*>(p));
}

/*!
	\class EdyukApplication
	\brief The core class of Edyuk
	
	\see EdyukGUI
*/

/*!
	\brief ctor
*/
EdyukApplication::EdyukApplication(int& argc, char **argv)
 : QSingleApplication(argc, argv),
	pGUI(0),
	pSettings(0), pTranslator(0), pToolsManager(0),
	pShortcutManager(0), pTemplateManager(0)
{
	//	Initialize resource
	Q_INIT_RESOURCE(Edyuk);
	
	// Get back app settings (must be done before anything else...)
	// 
	// handle special cli argument : -settings location for enhanced flexibility/mobility
	QString settingsLoc;

	for ( int i = 0; i < argc; ++i )
		if ( !qstrcmp(argv[i], "-settings") && (i + i) < argc )
			settingsLoc = QString::fromLocal8Bit(argv[++i]);
	
	if ( settingsLoc.count() )
	{
		QFileInfo info(settingsLoc);

		if ( info.isRelative() )
			settingsLoc = Edyuk::makeAbsolute(info.path(), applicationFilePath());
		else
			settingsLoc = info.absolutePath();
		
		bool ok = QDir::root().mkpath(settingsLoc);
		
		//qDebug("creating dir : %s, %i", qPrintable(settingsLoc), ok);
		
		settingsLoc += "/";
		m_path = settingsLoc;
		
		if ( info.fileName().count() )
			settingsLoc += info.fileName();
		else
			settingsLoc += "edyuk.conf";
	}
	
	pSettings = settingsLoc.count() ? new EdyukSettings(settingsLoc) : new EdyukSettings;
	
	// single app stuff
	if ( !isInstanceAllowed() && pSettings->value("gui/instances").toInt() == Edyuk::InstanceSingle )
	{
		QStringList l = arguments();
		l.removeAt(0);
		
		sendRequest(l);
		return;
	}
	
	#if defined(Q_WS_MAC)
	// check for bundling
	QString bundleContents;
	
	if ( applicationDirPath().section('/', -4, -3) == "edyuk.app/Contents" )
		bundleContents = applicationDirPath().section('/', 0, -3);
	#endif
	
	// Initialize text stream recieiving Edyuk log
	pEdyukLogFile = new QFile( Edyuk::settingsPath() + "run.log" );
	
	if ( !pEdyukLogFile->open(QFile::WriteOnly | QFile::Text) )
		qWarning("Unable to write log file : %s", qPrintable(Edyuk::settingsPath() + "run.log"));
	
	clog.setDevice(pEdyukLogFile);
	
	qRegisterMetaType<QTextCursor>("QTextCursor");
	
	// Install the message handler
	qInstallMsgHandler(EdyukMessageHandler);
	
	// Setup splash screen
	Q_INIT_SPLASH(":/splash.jpg")
	
	// Initialize data pathes
	
	Edyuk::addDataPath(QApplication::applicationDirPath());
	
	QCE::addDataPath(
			QApplication::applicationDirPath()
			+ QDir::separator()
			+ "qxs"
		);
	
	#if defined(Q_WS_X11)
	// extra plugin path for "smooth" *Nix install
	Edyuk::addDataPath("/usr/share/edyuk");
	QCE::addDataPath("/usr/share/edyuk/qxs");
	#elif defined(Q_WS_MAC)
	// extra plugin path for "bundle" Mac install
	if ( bundleContents.count() )
	{
		Edyuk::addDataPath(bundleContents + "/Resources");
		QCE::addDataPath(bundleContents + "/Resources/qxs");
	}
	#endif
	
	Edyuk::addDataPath(Edyuk::settingsPath());
	QCE::addDataPath(Edyuk::settingsPath() + "qxs");
	
	// Setup plugin system
	
	QPluginConfig::setStorageLocation(Edyuk::settingsPath() + "plugins");
	
	QPluginManager::instance()->setBlacklist(pSettings->value("plugins/blacklist").toStringList());
	
	QPluginManager::instance()->addHandler("QBuilder", BuilderHandler);
	QPluginManager::instance()->addHandler("QDebugger", DebuggerHandler);
	
	QPluginManager::instance()->addHandler("QCodeParser", CodeParserHandler);
	QPluginManager::instance()->addHandler("QProjectParser", ProjectParserHandler);
	
	QPluginManager::instance()->addHandler("QEditorInputBindingInterface", InputBindingHandler);
	QPluginManager::instance()->addHandler("QCodeCompletionEngine", CompletionEngineHandler);
	QPluginManager::instance()->addHandler("QLanguageDefinition", LanguageDefinitionHandler);
	
	QPluginManager::instance()->addHandler("qmdiClient", ClientHandler);
	QPluginManager::instance()->addHandler("qmdiPerspective", PerspectiveHandler);
	
	// Setup QCodeEdit settings
	Q_SPLASH("Initializing editing framework...")
	
	QLineMarksInfoCenter::instance()
		->loadMarkTypes(QCE::fetchDataFile("marks.qxm"));
	
	QLineMarksInfoCenter::instance()
		->loadMarks(Edyuk::settingsPath() + "marks.dump");
	
	m_snippetManager = new QSnippetManager;
	
	Q_SPLASH("Initializing code snippets...")
	
	// first time snippet setup (copy sys-wide snip to local snip)
	
	QDir().mkpath(Edyuk::settingsPath() + "snippets");
	
	foreach ( QString dp, Edyuk::dataPathes() )
	{
		if ( dp == Edyuk::settingsPath() )
			continue;
		
		QDir data(Edyuk::settingsPath() + "snippets");
		QDir base(dp + QDir::separator() + "snippets");
		
		QStringList l = base.entryList(QDir::Files | QDir::Readable);
		
		foreach ( QString f, l )
		{
			if ( QFileInfo(f).suffix() != "qcs" )
				continue;
			
			if ( !data.exists(f) )
				QFile::copy(base.filePath(f), data.filePath(f));
		}
	}
	
	m_snippetManager->loadSnippetsFromDirectory(
							Edyuk::settingsPath()
							+ "snippets"
						);
	
	// Setup shortcut manager (must be done before plugins lookup...)
	Q_SPLASH("Initializing shortcuts...")
	QShortcutManager::setSettingsPath(Edyuk::settingsPath());
	pShortcutManager = new QShortcutManager;
	
	// Setup tools manager
	Q_SPLASH("Initializing tools...")
	pToolsManager = new EdyukToolsManager;
	
	// Setup Locale Language
	Q_SPLASH("Initializing langage...")
	pTranslator  = new EdyukTranslator(pSettings);
	connect(pTranslator		, SIGNAL( languageChanged(const QString&) ),
			pShortcutManager, SLOT  ( languageChanged(const QString&) ) );
	
	// setup template manager
	Q_SPLASH("Initializing template manager...")
	pTemplateManager = new EdyukTemplateManager;
	
	// Setup GUI
	Q_SPLASH("Initializing User Interface...")
	
	//  * Setup main window
	setWindowIcon(QIcon(":/edyuk.svg"));
	
	pGUI = new EdyukGUI(pSettings);
	pGUI->setUpdatesEnabled(false);
	pGUI->setRecentAction(pSettings->recent());
	pGUI->setLanguageAction(pTranslator->action());
	
	pSettings->setSnippetManager(m_snippetManager);
	pSettings->loadFormatSchemes(qobject_cast<QEditorFactory*>(pGUI->clientFactory())->languageFactory());
	
	connect(pGUI,
			SIGNAL( fileOpened(QString) ),
			pSettings,
			SLOT  ( addRecentFile(QString) ) );
	
	connect(pGUI,
			SIGNAL( projectOpened(QString) ),
			pSettings,
			SLOT  ( addRecentProject(QString) ) );
	
	connect(pSettings,
			SIGNAL( recentFile(QString) ),
			pGUI,
			SLOT  ( fileOpen(QString) ) );
	
	connect(pSettings,
			SIGNAL( recentProject(QString) ),
			pGUI,
			SLOT  ( projectOpen(QString) ) );
	
	connect(pToolsManager	, SIGNAL( toolsChanged(QActionGroup*) ),
			pGUI			, SLOT  ( toolsChanged(QActionGroup*) ) );
	
	
	connect(QBuildEngine::instance(),
			SIGNAL( mergingRequested(qmdiClient*, bool) ),
			pGUI,
			SLOT  ( mergeExtraClient(qmdiClient*, bool) ) );
	
	connect(QBuildEngine::instance(),
			SIGNAL( targetListUpdateRequested() ),
			pGUI,
			SLOT  ( refreshTargetList() ) );
	
	connect(QBuildEngine::instance(),
			SIGNAL( filesChanged(QStringList) ),
			pGUI,
			SLOT  ( notifyFileChange(QStringList) ) );
	
	connect(QBuildEngine::instance(),
			SIGNAL( taskFinished() ),
			pGUI,
			SLOT  ( refreshClassTree() ) );
	
	connect(QBuildEngine::instance(),
			SIGNAL( taskAboutToStart() ),
			pGUI,
			SLOT  ( buildTaskAboutToStart() ) );
	
	connect(QDebuggingEngine::instance(),
			SIGNAL( mergingRequested(qmdiClient*, bool) ),
			pGUI,
			SLOT  ( mergeExtraClient(qmdiClient*, bool) ) );
	
	connect(QDebuggingEngine::instance(),
			SIGNAL( widgetInsertionRequested(QWidget*, bool) ),
			pGUI,
			SLOT  ( insertExtraDockWidget(QWidget*, bool) ) );
	
	pShortcutManager->applyAll();
	pToolsManager->updateActions();
	
	// plugin loading
	Q_SPLASH("Initializing plugins...")
	
	// default plugin path for "sandbox" install
	QPluginManager::instance()->addPluginPath(applicationDirPath() + "/plugins");
	
	#if defined(Q_WS_X11)
	// extra plugin path for "smooth" *Nix install
	QPluginManager::instance()->addPluginPath("/usr/share/edyuk/plugins");
	#elif defined(Q_WS_MAC)
	// extra plugin path for "bundle" Mac install
	if ( bundleContents.count() )
		QPluginManager::instance()->addPluginPath(bundleContents + "/PlugIns");
	#endif
	
	//QBuildEngine::instance()->rebuildCache();
	
	Q_SPLASH("Setting default perspective.")
	pGUI->setDefaultPerspective();
	
	Q_SPLASH("Edyuk initialized.")
	Q_HOOK_SPLASH(pGUI)
	
	pGUI->updateGUI();
	pGUI->setUpdatesEnabled(true);
	
	pTranslator->setDefaultLanguage();
	
	pGUI->show();
	
	QTimer::singleShot(0, this, SLOT( reopen() ));
	
	Q_DELETE_SPLASH
	
	qDebug(" ");
}

EdyukApplication::~EdyukApplication()
{
	//	Restore default message handler
	qInstallMsgHandler(0);
	
	#ifdef _EDYUK_DEBUG_
	qDebug("attempting to clean garbage...");
	#endif
	
	pSettings->setValue("plugins/blacklist", QPluginManager::instance()->blacklist());
	
	if ( pGUI )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean gui...");
		#endif
		delete pGUI;
	}
	
	if ( pTemplateManager )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean template manager...");
		#endif
		delete pTemplateManager;
	}
	
	if ( pTranslator )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean translator...");
		#endif
		delete pTranslator;
	}
	
	if ( pToolsManager )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean tools manager...");
		#endif
		delete pToolsManager;
	}
	
	if ( pShortcutManager )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean shortcut manager...");
		#endif
		delete pShortcutManager;
	}
	
	if ( m_snippetManager )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean snippet manager...");
		#endif
		m_snippetManager->saveSnippetsToDirectory(
							Edyuk::settingsPath()
							+ "snippets"
						);
		
		delete m_snippetManager;
	}
	
	if ( pSettings )
	{
		#ifdef _EDYUK_DEBUG_
		qDebug("attempting to clean settings...");
		#endif
		delete pSettings;
	}
	
	#ifdef _EDYUK_DEBUG_
	qDebug("Edyuk garbage successfully cleaned!");
	#endif
	
	// unload plugins before app get destroyed...
	QPluginManager::instance()->clear();
	
	QLineMarksInfoCenter::destroy();
	
	delete pEdyukLogFile;
}

int EdyukApplication::exec()
{
	if ( !isInstanceAllowed() && pSettings->value("gui/instances").toInt() == Edyuk::InstanceSingle )
		return -1;
	
	return QApplication::exec();
}

void EdyukApplication::request(const QString& s)
{
	qDebug("Unhandled CLI request : %s", qPrintable(s));
}

void EdyukApplication::request(const QStringList& l)
{
	qDebug("CLI request : \n\t%s", qPrintable(l.join("\n\t")));
	
	for ( int i = 0; i < l.count(); i++ )
	{
		QString s = l.at(i);
		
		if ( s.startsWith("-") )
		{
			
		} else {
			pGUI->fileOpen(s);
		}
	}
}

bool EdyukApplication::event(QEvent *e)
{
	if ( !e )
		return false;
	
	switch ( e->type() )
	{
		case Edyuk::RunTimeTranslation :
			
			pSettings->retranslate();
			pToolsManager->retranslate();
			//pPluginManager->retranslate();
			
			QBuildEngine::instance()->retranslate();
			QDebuggingEngine::instance()->retranslate();
			
			if ( pGUI )
				pGUI->retranslate();
			
			return QApplication::event(e);
			
		case QEvent::FileOpen :
			
			break;
			
		case QEvent::Close :
			pGUI->hide();
			
		default:
			return QApplication::event(e);
	}
	
	return true;
}

QString EdyukApplication::currentFile() const
{
	return pGUI ? pGUI->currentFile() : QString();
}

QString EdyukApplication::currentProject() const
{
	return pGUI ? pGUI->activeProject() : QString();
}

QStringList EdyukApplication::openedFiles() const
{
	return pGUI ? pGUI->openedFiles() : QStringList();
}

QStringList EdyukApplication::openedProjects() const
{
	return pGUI ? pGUI->openedProjects() : QStringList();
}

void EdyukApplication::reopen()
{
	pGUI->lockPerspective(true);
	
	QStringList args = arguments();
	args.removeAt(0);
	
	int i = 0;
	
	while ( i < args.count() )
	{
		const QString& a = args.at(i);

		if ( a.isEmpty() )
		{
			args.removeAt(i);
		} else if ( a.at(0).unicode() == '-' ) {
			args.removeAt(i);
			
			if ( i < args.count() )
				args.removeAt(i);
		} else {
			++i;
		}
	}
	
	if ( args.count() )
	{
		foreach ( const QString& s , args )
		{
			pGUI->fileOpen(s);
			//pGUI->message(tr("Opened file :\n%1").arg(s));
		}
	} else {
		QString s;
		QSettingsClient reopen(pSettings, "opened");
		const int file_mode = reopen.value("file_mode", Edyuk::ReopenAll).toInt();
		
		QStringList l = reopen.value("files").toStringList();
		
		switch ( file_mode )
		{
			case Edyuk::ReopenNone :
				break;
				
			case Edyuk::ReopenAll :
				foreach ( QString s, l )
					pGUI->fileOpen(s);
				
				// Force bringing focus to the former "current file" 
				//break;
				
			case Edyuk::ReopenCurrent :
				s = reopen.value("file").toString();
				
				if ( s.count() )
					pGUI->fileOpen(s);
				
				break;
		}
		
		const int project_mode = reopen.value("project_mode", Edyuk::ReopenAll).toInt();
		
		l = reopen.value("projects").toStringList();
		
		switch ( project_mode )
		{
			case Edyuk::ReopenNone :
				break;
				
			case Edyuk::ReopenAll :
				foreach ( QString s, l )
					pGUI->projectOpen(s);
				
				break;
				
			case Edyuk::ReopenCurrent :
				s = reopen.value("project").toString();
				
				if ( s.count() )
					pGUI->projectOpen(s);
				
				break;
		}
	}
	
	pGUI->lockPerspective(false);
}

void EdyukApplication::commitData(QSessionManager& manager)
{
	bool ret = pGUI->tryClose(&manager);
	
	if ( !ret )
		manager.cancel();
	
}

void EdyukApplication::saveState(QSessionManager& manager)
{
	;
}

void EdyukApplication::registerEngineTrigger(QCodeCompletionEngine *eng)
{
	//qDebug("registering trigger");
	EDYUK_SHORTCUT(
					eng->triggerAction(),
					"Edit",
					tr("Ctrl+Space")
				);
}

int EdyukApplication::version()
{
	return EDYUK_VERSION;
}

QString EdyukApplication::versionString()
{
	return EDYUK_VERSION_STR;
}

bool EdyukApplication::loggerReady() const
{
	return pGUI ? pGUI->isVisible() : false;
}

void EdyukApplication::log(const QString& s)
{
	pGUI->m_logDock->edyukLog(s);
}

EdyukApplication* EdyukApplication::Instance()
{
	return qobject_cast<EdyukApplication*>(qApp);
}

EdyukGUI* EdyukApplication::gui() const
{
	return pGUI;
}

EdyukTemplateManager* EdyukApplication::templateManager() const
{
	return pTemplateManager;
}

EdyukTranslator* EdyukApplication::translator() const
{
	return pTranslator;
}

EdyukToolsManager* EdyukApplication::toolsManager() const
{
	return pToolsManager;
}

QShortcutManager* EdyukApplication::shortcutManager() const
{
	return pShortcutManager;
}

QSnippetManager* EdyukApplication::snippetManager() const
{
	return m_snippetManager;
}

static QStringList __edyuk_data_path;

QStringList Edyuk::dataPathes()
{
	return __edyuk_data_path;
}

QString Edyuk::dataFile(const QString& file)
{
	if ( QFileInfo(file).isAbsolute() )
		return file;
	
	foreach ( QString dp, __edyuk_data_path )
	{
		QDir d(dp);
		
		if ( d.exists(file) )
			return d.absoluteFilePath(file);
	}
	
	return file;
}

void Edyuk::addDataPath(const QString& p)
{
	__edyuk_data_path << p;
}

QString Edyuk::settingsPath()
{
	QString& path = EdyukApplication::Instance()->m_path;

	if ( path.count() )
		return path;
	
	/*
		It seems that this function is called by a static constructor somewhere
		(i.e. before main() so we shall provide a correct defaulting)
	*/
	QString app = "edyuk";

	/*
	QString app = QApplication::applicationName();
	
	if ( app.isEmpty() )
	{
		app = "edyuk-";
		app += EDYUK_VERSION_STR;
	}
	*/
	
	if ( path.isEmpty() )
	{
		path = QDir::homePath() + QDir::separator() + "." + app + QDir::separator();
		//qDebug("path set to : %s", qPrintable(path));
	}
	
	if ( !QDir::home().exists("." + app) )
	{
		QDir::home().mkdir("." + app);
	}
	
	return path;
}

static void strip(QString& s)
{
	QStringList l = s.split("/", QString::KeepEmptyParts);
	
	for ( int i = 0; i < l.count(); ++i )
	{
		
		if ( l.at(i) == "." )
		{
			l.removeAt(i);
			--i;
		}
		
		if ( l.at(i) == ".." )
		{
			l.removeAt(i);
			--i;
			
			if ( i >= 0 )
				l.removeAt(i);
		}
	}
	
	s = l.join("/");
}

QString Edyuk::makeAbsolute(const QString& rel, const QString& abs)
{
	QStringList ref = QFileInfo(abs).path().replace("\\", "/").split("/"),
				mov = QString(rel).replace("\\", "/").split("/");
	
	foreach ( QString s, mov )
	{
		if ( s == "." )
			continue;
		
		if ( s == ".." )
		{
			if ( ref.count() )
				ref.removeLast();
		} else
			ref << s;
	}
	
	return ref.join("/");
}

QString Edyuk::makeRelative(const QString& to, const QString& f)
{
	if ( f.isEmpty() )
		return QString();
	
	if ( !QFileInfo(f).isAbsolute() )
		return f;
	
	QString sTo, sF;
	QFileInfo infoTo(to.isEmpty() ? "." : to), infoF(f);
	
	infoF.makeAbsolute();
	sF = infoF.absoluteFilePath().replace("\\", "/");
	
	infoTo.makeAbsolute();
	sTo = infoTo.isFile() ? infoTo.absolutePath() : infoTo.absoluteFilePath();
	sTo.replace("\\", "/");
	
	strip(sF);
	strip(sTo);
	
	if ( sF.startsWith(sTo) )
		return sF.remove(0, sTo.count() + 1);
	
	if ( sTo.startsWith(sF) )
	{
		sTo.remove(0, sF.count() + 1);
		sF.clear();
		
		goto MakeList;
	}
	
	#ifdef Q_WS_WIN
	if ( sF.left(3) != sTo.left(3) )
		return sF;
	
	sF.remove(0, 3);
	sTo.remove(0, 3);
	#else
	sF.remove(0, 1);
	sTo.remove(0, 1);
	#endif
	
	MakeList:
	
	QStringList lF = sF.split("/", QString::SkipEmptyParts),
				lTo = sTo.split("/", QString::SkipEmptyParts);
	
	while ( lF.count() && lTo.count() )
	{
		if ( lF.first() != lTo.first() )
			break;
		
		lF.removeAt(0);
		lTo.removeAt(0);
	}
	
	foreach (QString s, lTo)
		lF.prepend("..");
	
	return lF.join("/");
}

QStringList Edyuk::splitArguments(const QString& s)
{
	QStringList l;
	int i = 0, j = 0;
	bool quoted = false;
	
	while ( i <= s.length() )
	{
		if ( (i == s.length()) || (s.at(i).isSpace() && !quoted) )
		{
			if ( i != j )
				l << s.mid(j, i - j);
			
			j = ++i;
			quoted = (i < s.length()) ? s.at(i) == '\"' : false;
		} else if ( s.at(i) == '\\' ) {
			++i;
		} else if ( s.at(i) == '\"' ) {
			quoted = false;
		}
		
		++i;
	}
	
	return l;
}
