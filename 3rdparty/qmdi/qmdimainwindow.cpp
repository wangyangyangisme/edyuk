/***************************************************************************
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

#include "qmdimainwindow.h"

/*!
	\file qmdimainwindow.cpp
	\brief Implementation of the qmdiMainWindow class.
*/

#include "qmdiworkspace.h"
#include "qmdistatusbar.h"
#include "qmdiperspective.h"

#include "qmdiclientfactory.h"

#include <QDir>
#include <QMenu>
#include <QFile>
#include <QTimer>
#include <QAction>
#include <QDockWidget>
#include <QToolButton>
#include <QActionGroup>
#include <QStackedWidget>
#include <QApplication>

/*!
	\ingroup mdi
	
	@{
	
	\class qmdiMainWindow
	\brief An extended main window.
	
	qmdiMainWindow provides a complete perspective management system, basic
	actions to take advantages of workspaces and some convinience wrappers
	for qmdiHost and (current) qmdiWorkspace.
	
	\see qmdiWorkspace
	\see qmdiStatusBar
	\see qmdiPerspective
*/

/*!
	\brief Constructor
*/
qmdiMainWindow::qmdiMainWindow(QWidget *p)
 : QMainWindow(p), bLocked(false)
{
	aNew = new QAction(QIcon(":/new.png"), tr("&New..."), this);
	aOpen = new QAction(QIcon(":/open.png"), tr("&Open file..."), this);
	aSave = new QAction(QIcon(":/save.png"), tr("&Save"), this);
	aSaveAs = new QAction(QIcon(":/saveas.png"), tr("Save &as..."), this);
	aSaveAll = new QAction(QIcon(":/saveall.png"), tr("Sa&ve all"), this);
	aClose = new QAction(QIcon(":/close.png"), tr("&Close"), this);
	aCloseAll = new QAction(QIcon(":/closeall.png"), tr("C&lose all"), this);
	aPrint = new QAction(QIcon(":/print.png"), tr("&Print"), this);
	
	aExit = new QAction(QIcon(":/exit.png"), tr("E&xit"), this);
	aExit->setMenuRole(QAction::QuitRole);
	connect(aExit	, SIGNAL( triggered() ),
	//		qApp	, SLOT  ( quit() ) );
			this	, SLOT  ( forceClose() ) );
	
	aTile = new QAction(tr("&Tile"), this);
	aTile->setEnabled(false);
	
	aCascade = new QAction(tr("&Cascade"), this);
	aCascade->setEnabled(false);
	
	pStatus = new qmdiStatusBar(this);
	setStatusBar(pStatus);
	
	pFactory = 0;
	
	pWorkspace = new qmdiWorkspace(this);
	pWorkspace->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	aPrint->setEnabled(false);
	connect(pWorkspace	, SIGNAL( currentPrintable(bool) ),
			aPrint		, SLOT  ( setEnabled(bool) ) );
	
	connect(aPrint		, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( printCurrent() ) );
	
	aTile->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( widgetsOpened(bool) ),
			aTile		, SLOT  ( setEnabled(bool) ) );
	
	connect(aTile		, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( tile() ) );
	
	aCascade->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( widgetsOpened(bool) ),
			aCascade	, SLOT  ( setEnabled(bool) ) );
	
	connect(aCascade	, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( cascade() ) );
	
	aSave->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( currentModified(bool) ),
			aSave		, SLOT  ( setEnabled(bool) ) );
	
	connect(aSave		, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( saveCurrent() ) );
	
	aSaveAs->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( widgetsOpened(bool) ),
			aSaveAs		, SLOT  ( setEnabled(bool) ) );
	
	connect(aSaveAs		, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( saveCurrentAs() ) );
	
	aSaveAll->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( widgetsOpened(bool) ),
			aSaveAll	, SLOT  ( setEnabled(bool) ) );
	
	connect(aSaveAll	, SIGNAL( triggered() ),
			this		, SLOT  ( saveAll() ) );
	
	aClose->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( widgetsOpened(bool) ),
			aClose		, SLOT  ( setEnabled(bool) ) );
	
	connect(aClose		, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( closeCurrent() ) );
	
	aCloseAll->setEnabled(false);
	
	connect(pWorkspace	, SIGNAL( widgetsOpened(bool) ),
			aCloseAll	, SLOT  ( setEnabled(bool) ) );
	
	connect(aCloseAll	, SIGNAL( triggered() ),
			pWorkspace	, SLOT  ( closeAll() ) );
	
	setCentralWidget(pWorkspace);
	
	aNone = new QAction(tr("&None"), this);
	aNone->setCheckable(true);
	aNone->setChecked(true);
	
	pPerspecActions = new QActionGroup(this);
	pPerspecActions->setExclusive(true);
	pPerspecActions->addAction(aNone);
	
	connect(pPerspecActions	, SIGNAL( triggered(QAction*) ),
			this			, SLOT  ( perspectiveChanged(QAction*) ) );
	
	pPerspecMenu = new QMenu(this);
	pPerspecMenu->setTitle(tr("Perspectives"));
	pPerspecMenu->addAction(aNone);
	pPerspecMenu->addSeparator();
	
	aPerspective = pPerspecMenu->menuAction();
	aPerspective->setMenuRole(QAction::PreferencesRole);
}

/*!
	\brief Destructor
*/
qmdiMainWindow::~qmdiMainWindow()
{
	pCur = 0;
	
	foreach ( qmdiPerspective *p, perspec )
		p->disconnect();
	
	//qDeleteAll(perspec);
	perspec.clear();
	
	menus.clear();
	toolbars.clear();
	
	updateGUI();
}

void qmdiMainWindow::forceClose()
{
	close();
}

/*!
	\brief Retranslates the default actions and the perspectives
*/
void qmdiMainWindow::retranslate()
{
	foreach ( qmdiPerspective *p, perspec )
	{
		//qDebug("tr(mainwin->perspec)");
		p->retranslate();
	}
	
	pWorkspace->retranslate();
	
	aNew->setText(tr("&New..."));
	aOpen->setText(tr("&Open file..."));
	aSave->setText(tr("&Save"));
	aSaveAs->setText(tr("Save &as..."));
	aSaveAll->setText(tr("Sa&ve all"));
	aClose->setText(tr("&Close"));
	aCloseAll->setText(tr("C&lose all"));
	aPrint->setText(tr("&Print"));
	aExit->setText(tr("E&xit"));
	aTile->setText(tr("&Tile"));
	aCascade->setText(tr("&Cascade"));
}

/*!
	\return the status bar
*/
qmdiStatusBar* qmdiMainWindow::status() const
{
	return pStatus;
}

/*!
	\return the workspace
*/
qmdiWorkspace* qmdiMainWindow::workspace() const
{
	return pWorkspace;
}

/*!
	\return wether perspective is locked and may not be changed
*/
bool qmdiMainWindow::perspectiveLocked() const
{
	return bLocked;
}

/*!
	\brief (un)locks perspective to the current one.
	\param true locks, false, unlocks.
	
	Neither user actions nor any qmdiMainWindow calls will change it
	perspective when locked. 
*/
void qmdiMainWindow::lockPerspective(bool y)
{
	bLocked = y;
}

/*!
	\return Client factory passed to qmdiPerspective::createEmptyClient().
	
	\see setClientFactory()
*/
qmdiClientFactory* qmdiMainWindow::clientFactory() const
{
	return pFactory;
}

/*!
	\brief Sets a client factory
	
	Client factory is usedas a client source. This basically means that the
	clientCreated() signal of qmdiClientFactory is intercepted and proper
	clients are added to underlying perspectives.
*/
void qmdiMainWindow::setClientFactory(qmdiClientFactory *f)
{
	pFactory = f;
}

/*!
	\return The active perspective
	
	\warning Can be 0.
*/
qmdiPerspective* qmdiMainWindow::perspective() const
{
	return pCur;
}

/*!
	\return A list of available perspective names
*/
QStringList qmdiMainWindow::perspectiveNames() const
{
	QStringList l;
	
	foreach ( qmdiPerspective *p, perspec )
		l << p->name();
	
	return l;
}

/*!
	\return A list of available perspectives
*/
QList<qmdiPerspective*> qmdiMainWindow::perspectives() const
{
	return perspec;
}

/*!
	\overload
	\param n Name of the perspective to set.
	
	\see setPerspective(qmdiPerspective*)
*/
void qmdiMainWindow::setPerspective(const QString& n)
{
	foreach ( qmdiPerspective *p, perspec )
		if ( p->name() == n )
			return setPerspective(p);
	
}

/*!
	\brief Sets the current perspective to \a p
*/
void qmdiMainWindow::setPerspective(qmdiPerspective *p)
{
	if ( bLocked )
		return;
	
	pNext = p;
	
	QTimer::singleShot(10, this, SLOT( setPerspective() ) );
}

/*!
	\internal
*/
void qmdiMainWindow::setPerspective()
{
	qmdiPerspective *p = pNext;
	
	if ( (p == pCur) || bLocked )
	{
		//qDebug("p == pCur : %i | bLocked : %i", p == pCur, bLocked);
		return;
	}
	
	bool bPrev = updatesEnabled();
	
	update();
	
	if ( bPrev )
		setUpdatesEnabled(false);
	
	Qt::DockWidgetArea	topLeft = corner(Qt::TopLeftCorner),
						topRight = corner(Qt::TopRightCorner),
						bottomLeft = corner(Qt::BottomLeftCorner),
						bottomRight = corner(Qt::BottomRightCorner);
	
	if ( pCur )
	{
		perspecStates[ perspec.indexOf(pCur) ] = saveState();
		
		//setWorkspace(0);
		
		// custom fine-tunned hiding...
		pCur->hideEvent();
		
		for ( int i = 0; i < pCur->m_docks.count(); i++ )
		{
			QDockWidget *dw = pCur->m_docks.at(i);
			
			pCur->m_areas[i] = dockWidgetArea(dw);
			pStatus->removeButton(pCur->m_tools.at(i));
			removeDockWidget(dw);
		}
		
		unmergeClient(pCur);
		updateGUI();
	}
	
	#ifdef _EDYUK_DEBUG_
	qDebug("Perspective changed...");
	#endif
	
	if ( bPrev )
	{
		setUpdatesEnabled(true);
		repaint();
		setUpdatesEnabled(false);
	}
	
	emit currentPerspectiveAboutToChange(pCur);
	pCur = p;
	emit currentPerspectiveChanged(pCur);
	
	if ( pCur )
	{
		QAction *a = actions[ perspec.indexOf(pCur) ];
		
		if ( a && !a->isChecked() )
			a->setChecked(true);
		
		QByteArray state = perspecStates[ perspec.indexOf(pCur) ];
		
		for ( int i = 0; i < pCur->m_docks.count(); i++ )
		{
			QDockWidget *dw = pCur->m_docks.at(i);
			
			addDockWidget(pCur->m_areas.at(i), dw);
			pStatus->addButton(pCur->m_tools.at(i));
			
			if ( state.isEmpty() && !i )
			{
				dw->show();
			}
		}
		
		// custom fine-tunned showing...
		pCur->showEvent();
		
		mergeClient(pCur);
		updateGUI();
		restoreState(state);
		
		pStatus->showMessage("Perspective is : " + pCur->name());
	} else {
		//updateGUI();
		aNone->setChecked(true);
	}
	
	setCorner(Qt::TopLeftCorner, topLeft);
	setCorner(Qt::TopRightCorner, topRight);
	setCorner(Qt::BottomLeftCorner, bottomLeft);
	setCorner(Qt::BottomRightCorner, bottomRight);
	
	show();
	update();
	setUpdatesEnabled(bPrev);
	update();
}

/*!
	\brief Start managing a perspective
*/
void qmdiMainWindow::addPerspective(qmdiPerspective *p)
{
	if ( !p || perspec.contains(p) )
		return;
	
	#ifdef _EDYUK_DEBUG_
	qDebug("adding perspective");
	#endif
	
	p->setMainWindow(this);
	
	QAction *a = new QAction(p->icon(), p->name(), this);
	a->setCheckable(true);
	
	pPerspecMenu->addAction(a);
	pPerspecActions->addAction(a);
	
	actions << a;
	perspec << p;
	perspecStates << QByteArray();
}

/*!
	\brief Unmanages a perspective
*/
void qmdiMainWindow::removePerspective(qmdiPerspective *p)
{
	int i = perspec.indexOf(p);
	
	if ( i == -1 )
		return;
	
	p->setParent(0);
	p->setMainWindow(0);
	
	actions.removeAt(i);
	perspec.removeAt(i);
	perspecStates.removeAt(i);
	
	QAction *a = actions.takeAt(i);
	pPerspecMenu->removeAction(a);
	pPerspecActions->removeAction(a);
	
	delete a;
}

/*!
	\return the name of the active file of the active workspace
*/
QString qmdiMainWindow::currentFile() const
{
	if ( pWorkspace )
		return pWorkspace->file();
	
	return QString();
}

/*!
	\return File names of opened files
*/
QStringList qmdiMainWindow::openedFiles() const
{
	return pWorkspace->files();
}

/*!
	\return List of modified files
*/
QStringList qmdiMainWindow::modifiedFiles() const
{
	QStringList l;
	QWidgetList wl = pWorkspace->windowList();
	
	foreach ( QWidget *w, wl )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( !c )
			continue;
		
		if ( c->isContentModified() )
			l << c->fileName();
	}
	
	return l;
}

/*!
	\brief Open a new file and add it to the workspace
	\param name path of the file to open
	\return A pointer to the opened file, if possible
	
	In order to open a file, qmdiMainWindow iterates over its perspectives and tries
	to open the file with each of them. Then, if none shown interest, it relies on
	its clientFactory()
	
	\note If the file is already opened fileOpen() does exactly the same as isOpen()
	
	\warning The pointer returned can be NULL for various reasons :
	<ul>
		<li>The file \a name does not exist
		<li>One of the perspective caused it to be open as a project
		<li>One of the perspective caused it to be open in an external application
	<ul>
*/
QWidget* qmdiMainWindow::fileOpen(const QString& name)
{
	QWidget *w = 0;
	QFileInfo info(name);
	
	QString fp = info.absoluteFilePath();
	
	if ( isOpen(fp, Focus) )
		return window(fp);
	
	if ( !QFile::exists(fp) )
	{
		qWarning("File %s does not exist...", qPrintable(name));
		return 0;
	}
	
	qmdiClient *c = 0;
	
	foreach ( qmdiPerspective *p, perspectives() )
	{
		if ( p->canOpen(fp) )
		{
			c = p->open(fp);
			
			if ( (void*)c == (void*)-1 )
			{
				// external app or weirder action
				//qDebug("external : %s", qPrintable(name));
				
				return 0;
			} else if ( c ) {
				// regular file opening
				//qDebug("regular : %s", qPrintable(name));
				
				break;
			} else {
				// opening failed / no opening done...
				//qDebug("failed : %s", qPrintable(name));
				
			}
		}
	}
	
	if ( !c && clientFactory() )
	{
		c = clientFactory()->createClient(fp);
	}
	
	if ( !c )
		return 0;
	
	w = dynamic_cast<QWidget*>(c);
	
	addWidget(w);
	
	return w;
}

/*!
	\return the first window (among windowList()) named \a fn or zero if none found
*/
QWidget* qmdiMainWindow::window(const QString& fn) const
{
	QString cfn = QDir::cleanPath(fn);
	
	foreach ( QWidget *w, windowList() )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( c && (QDir::cleanPath(c->fileName()) == cfn) )
			return w;
	}
	
	return 0;
}

/*!
	\return whether a window hodling file \a fn is already opened
	\param focus Whether setting found window (if any) as active one.
	
	\see window(const QString&)
	\see setActiveWindow(QWidget *w)
*/
bool qmdiMainWindow::isOpen(const QString& fn, bool focus)
{
	QWidget *w = window(fn);
	
	if ( w && focus )
		setActiveWindow(w);
	
	return w;
}

/*!
	\return The currently active window
	
	\see currentFile()
*/
QWidget* qmdiMainWindow::activeWindow() const
{
	return pWorkspace->activeWindow();
}

/*!
	\return A list of all opened windows
	
	\see openedFiles()
*/
QWidgetList qmdiMainWindow::windowList() const
{
	return pWorkspace->windowList();
}

/*!
	\brief Sets the active window
	\param w widget to set as active
	
	\note this function may change the current perspective
	
	\see setPerspective(qmdiPerspective*)
*/
void qmdiMainWindow::setActiveWindow(QWidget *w)
{
	if ( pWorkspace->windowList().contains(w) )
		return pWorkspace->setActiveWindow(w);
	
}

/*!
	\brief Adds a widget to the main window
	\param w widget to add
	
	\note This function may change the current perspective
	
	\warning \a w must inherit from qmdiClient!
*/
void qmdiMainWindow::addWidget(QWidget *w)
{
	pWorkspace->addWidget(w);
}

/*!
	\brief Save a file
*/
void qmdiMainWindow::saveFile(const QString& s)
{
	QList<QWidget*> wl = windowList();
	
	foreach ( QWidget *w, wl )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( c && c->fileName() == s )
			c->save();
	}
}

/*!
	\brief Save some files
*/
void qmdiMainWindow::saveFiles(const QStringList& l)
{
	QList<QWidget*> wl = windowList();
	
	foreach ( QWidget *w, wl )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( c && l.contains(c->fileName()) )
			c->save();
	}
}

/*!
	\brief Attempt to save all opened files.
	
	\see openedFiles()
	\see qmdiWorkspace::saveAll()
*/
void qmdiMainWindow::saveAll()
{
	pWorkspace->saveAll();
}

/*!
	\brief Pretends a close event
	\return Whether the (pretended) close event has been canceled
	
	The user is asked, through the qmdiServer::maybeSave() function,
	to commit unsaved changes to widgets. An option is offered to interrupt
	closure
	
	\see qmdiWorkspace::checkModified()
*/
bool qmdiMainWindow::checkModified()
{
	return pWorkspace->checkModified();
}

/*!
	\brief Close all opened files
	
	simply call workspace()->closeAll()
*/
bool qmdiMainWindow::closeAll()
{
	return pWorkspace->closeAll();
}

/*!
	\brief Close all opened files
	\param ignore whether to skip the modification check
	\return True if closing was successfull, false if one file
	rejected the close event.
	
	\note Unlike qmdiWorkspace::closeAll() this function DO call checkModified()
	
	\see openedFiles()
	\see checkModified()
*/
bool qmdiMainWindow::closeAll(bool ignore)
{
	if ( !ignore )
		if ( checkModified() )
			return false;
	
	pWorkspace->closeAll(true);
	
	return true;
}

/*!
	\brief Updates menus and toolbars
	
	\see qmdiHost::updateGUI(QMainWindow*)
*/
void qmdiMainWindow::updateGUI()
{
	qmdiHost::updateGUI(this);
}

/*!
	\internal
*/
void qmdiMainWindow::perspectiveChanged(QAction *a)
{
	if ( a == aNone )
		return setPerspective(0);
	
	setPerspective( perspec[ actions.indexOf(a) ] );
}

/*!
	\reimp
	\brief Give file filters supported by the application
	\return a valid set of filters (suitable for QFileDialog...)
	
	This re-implementation fetches filters from all know perspectives.
	
	\see perspectives()
	\see qmdiPerspective::filters()
*/
QString qmdiMainWindow::filters() const
{
	QString all(tr("All files ( * )"));
	
	QStringList l;
	
	foreach ( qmdiPerspective *p, perspec )
	{
		if ( p )
		{
			QStringList sl = p->filters();
			
			foreach ( const QString& f, sl )
				if ( !l.contains(f) )
					l << f;
			
		}
	}
	
	if ( !l.contains(all) )
		l << all;
	
	return l.join(";;");
}

/*!
	\brief Creates an empty client to add in the current workspace
	
	If a perspective is active, the creation is forwarded to
	qmdiPerspective::createEmptyClient(), otherwise a NULL pointer is returned.
*/
qmdiClient* qmdiMainWindow::createEmptyClient()
{
	if ( perspective() )
		return perspective()->createEmptyClient(pFactory);
	
	return pFactory ? pFactory->createClient(QString()) : 0;
}

/*!
	\reimp
	\brief Emit fileOpened() signal for given client
	\param c New client just opened (or moved to a workspace)
 */
void qmdiMainWindow::clientOpened(qmdiClient *c)
{
	if ( !c )
		return;
	
	emit fileOpened(c->fileName());
}

/*!
	\reimp
	\brief Emit fileClose() signal for given client
	\param c Old client about to be closed (or moved from workspace)
 */
void qmdiMainWindow::clientClosed(qmdiClient *c)
{
	if ( !c )
		return;
	
	emit fileClosed(c->fileName());
}

/*!
	\reimp
	\brief Adjust the perspective according to a given client
	\param c client requesting adjustement
*/
void qmdiMainWindow::currentClientChanged(qmdiClient *c)
{
	emit currentFileChanged(c ? c->fileName() : QString());
	
	if ( !c )
		return;
	
	QList<qmdiPerspective*> likely;
	qmdiPerspective::Affinity current, top = qmdiPerspective::None;
	
	foreach ( qmdiPerspective *p, perspec )
	{
		current = p->affinity(c);
		
		if ( current == qmdiPerspective::Exclusive )
		{
			// TODO : check for multi-exclusivity ???
			
			likely.clear();
			likely << p;
			
			break;
		} else if ( current != qmdiPerspective::None ) {
			if ( current > top )
			{
				top = current;
				likely.clear();
			}
			
			likely << p;
		}
	}
	
	if ( likely.isEmpty() )
	{
		// no match... no perspective
		setPerspective(0);
	} else if ( likely.count() == 1 ) {
		// perfect match :)
		setPerspective(likely.at(0));
	} else {
		// multi match...
		qDebug("Ambiguous fallback...");
		setPerspective(likely.at(0));
	}
}

/*!
	\brief Set the focus to current client
*/
void qmdiMainWindow::focusCurrentClient()
{
	if ( pWorkspace->activeWindow() )
		pWorkspace->activeWindow()->setFocus();
	
}

/*! @} */
