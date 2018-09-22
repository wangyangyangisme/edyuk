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

#include "edyukmanagerdock.h"

/*!
	\file edyukmanagerdock.cpp
	\brief Implementation of the EdyukManagerDock class.
*/

#include "qwidgetstack.h"

#include "qcodeview.h"
#include "qcodenode.h"
#include "qcodemodel.h"
#include "qcodeparser.h"
#include "qcodeloader.h"
#include "qcodeproxymodel.h"
#include "qsourcecodewatcher.h"

#include "qproject.h"
#include "qprojectview.h"
#include "qprojectmodel.h"
#include "qprojectloader.h"
#include "qprojectproxymodel.h"

#include "qeditor.h"
#include "qdocumentsearch.h"

#include "qmdimainwindow.h"

#include "qbuildengine.h"
#include "qdebuggingengine.h"

#include <QMenu>
#include <QComboBox>
#include <QFileInfo>
#include <QListWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMessageBox>

/*!
	\class EdyukManagerDock
	\brief Specialized dock widget
 
	It provide generic access to common project management, code browsing
	and a convenient way to travel through opened files.
*/

/*!
	\brief ctor
*/
EdyukManagerDock::EdyukManagerDock(qmdiMainWindow *p, QSettingsClient s)
 : QDockWidget(p), m_parent(p), m_blockTryRecursion(false)
{
	setWindowTitle(tr("Workspace"));
	
	m_stack = new QWidgetStack(s.value("manager-display", QWidgetStack::ToolBox).toInt());
	
	QWidget *w = new QWidget(this);
	QVBoxLayout *l = new QVBoxLayout(w);
	l->setMargin(0);
	l->setSpacing(0);
	
	m_projectModel = new QProjectModel(this);
	m_projectModel->setProjectLoader(new QProjectLoader(this));
	
	m_projectProxyModel = new QProjectProxyModel(this);
	m_projectProxyModel->setSourceModel(m_projectModel);
	
	m_projectSelection = new QComboBox(w);
	m_projectView = new QProjectView(w);
	m_projectView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_projectView->setModel(m_projectProxyModel);
	m_projectView->header()->hide();
	
	l->addWidget(m_projectSelection);
	l->addWidget(m_projectView);
	l->addWidget(m_projectView->actionBar());
	m_projectView->actionBar()->setSizePolicy(	QSizePolicy::Expanding,
												QSizePolicy::Minimum);
	
	connect(m_projectView	, SIGNAL( fileActivated(QString) ),
			this			, SLOT  ( fileActivated(QString) ) );
	
	/*
	connect(m_projectView	, SIGNAL( activeProjectChanged(QProject*) ),
			p				, SLOT  ( activeProjectChanged(QProject*) ) );
	*/
	
	connect(m_projectView	, SIGNAL( activeProjectChanged(QString) ),
			this			, SLOT  ( emitActiveProjectChanged(QString) ) );
	
	connect(m_projectSelection	, SIGNAL( activated(QString) ),
			this				, SLOT  ( tryChangeActiveProject(QString) ) );
	
	connect(m_projectModel	, SIGNAL( projectAdded(QProject*) ),
			this			, SLOT  ( projectAdded(QProject*) ) );
	
	connect(m_projectModel	, SIGNAL( projectRemoved(QProject*) ),
			this			, SLOT  ( projectRemoved(QProject*) ) );
	
	connect(m_projectModel	, SIGNAL( projectReloaded(QProject*, QProject*) ),
			this			, SLOT  ( projectReloaded(QProject*, QProject*) ) );
	
	connect(m_projectModel	, SIGNAL( fileAdded(QString, QProject*) ),
			this			, SLOT  ( fileAdded(QString, QProject*) ) );
	
	connect(m_projectModel	, SIGNAL( fileRemoved(QString, QProject*) ),
			this			, SLOT  ( fileRemoved(QString, QProject*) ) );
	
	m_codeModel = new QCodeModel(this);
	m_codeModel->setCodeLoader(new QCodeLoader(this));
	
	m_codeProxy = new QCodeProxyModel(this);
	m_codeProxy->setSourceModel(m_codeModel);
	//m_codeProxy->setDynamicSortFilter(true);
	
	m_codeView = new QCodeView(this);
	m_codeView->setModel(m_codeProxy);
	m_codeView->setSortingEnabled(true);
	m_codeView->header()->hide();
	
	connect(m_codeView	, SIGNAL( actionRequested(QString, QStringList) ),
			this		, SLOT  ( actionRequested(QString, QStringList) ) );
	
	
	m_scratchpad = new QListWidget(this);
	m_scratchpad->setContextMenuPolicy(Qt::CustomContextMenu);
	
	connect(m_scratchpad, SIGNAL( customContextMenuRequested(QPoint) ),
			this		, SLOT  ( fileContextMenu(QPoint) ) );
	
	connect(p	, SIGNAL( fileOpened(QString) ),
			this, SLOT  ( fileOpened(QString) ) );
	
	connect(p	, SIGNAL( fileClosed(QString) ),
			this, SLOT  ( fileClosed(QString) ) );
	
	connect(m_scratchpad,
//			SIGNAL( currentItemChanged(QListWidgetItem*, QListWidgetItem*) ),
			SIGNAL( itemClicked(QListWidgetItem*) ),
			this		,
			SLOT  ( itemChanged(QListWidgetItem*) ) );
	
	m_stack->addWidget(tr("Opened &files"), m_scratchpad);
	m_stack->addWidget(tr("&Projects"), w);
	m_stack->addWidget(tr("&Class browser"), m_codeView);
	
	setWidget(m_stack);
}

/*!
	\brief dtor
*/
EdyukManagerDock::~EdyukManagerDock()
{
	
}

/*!
	\brief retranslate all strings
*/
void EdyukManagerDock::retranslate()
{
	setWindowTitle(tr("Workspace"));
	
	m_stack->setLabel(m_codeView, tr("&Class browser"));
	m_stack->setLabel(m_scratchpad, tr("Opened &files"));
	m_stack->setLabel(m_projectView, tr("&Projects"));
}

/*!
	\return The common code view
	\see codeModel()
*/
QCodeView* EdyukManagerDock::codeView() const
{
	return m_codeView;
}

/*!
	\return The common code model
	\see codeView()
*/
QCodeModel* EdyukManagerDock::codeModel() const
{
	return m_codeModel;
}

/*!
	\return The common project view
	\see projectModel()
*/
QProjectView* EdyukManagerDock::projectView() const
{
	return m_projectView;
}

/*!
	\return The common project model
	\see projectView()
*/
QProjectModel* EdyukManagerDock::projectModel() const
{
	return m_projectModel;
}

/*!

*/
int EdyukManagerDock::projectViewDetailLevel() const
{
	return m_projectProxyModel->detailLevel();
}

/*!

*/
void EdyukManagerDock::setProjectViewDetailLevel(int l)
{
	m_projectProxyModel->setDetailLevel(l);
}

/*!
	\brief Add a code parser to the collection
*/
void EdyukManagerDock::addCodeParser(QCodeParser *p)
{
	m_codeModel->codeLoader()->addParser(p);
}

/*!
	\brief Add a project parser to the collection
*/
void EdyukManagerDock::addProjectParser(QProjectParser *p)
{
	m_projectModel->projectLoader()->addParser(p);
}

/*!
	\return The currently active project or an empty string
*/
QString EdyukManagerDock::activeProject() const
{
	QProject *p = m_projectView->activeProject();
	
	return p ? p->name() : QString();
}

/*!
	\return The list of opened projects
*/
QStringList EdyukManagerDock::openedProjects() const
{
	QStringList l;
	QList<QProject*> lp = m_projectModel->projects();
	
	foreach ( QProject *p, lp )
	{
		l << p->name();
	}
	
	return l;
}

/*!
	\return The list of modified projects
*/
QStringList EdyukManagerDock::modifiedProjects() const
{
	QStringList l;
	
	#if 0
	/*
		"smart" algo : does not report modified subprojects
		of modified projects (i.e find first modification in
		each project tree and report it).
		
		This allow accurate determination of "save points"
		(projects to save to have the entire hierarchy saved)
	*/
	
	QStack<QProject*> lp;
	QList<QProject*> tp = m_projectModel->projects();
	
	foreach ( QProject *p, tp )
		lp.push(p);
	
	while ( lp.count() )
	{
		QProject *p = lp.pop();
		
		if ( p->isModified() )
		{
			l << p->name();
		} else {
			tp = p->subProjects();
			
			foreach ( QProject *p, tp )
				lp.push(p);
		}
	}
	#else
	QList<QProject*> tp = m_projectModel->projects(true);
	
	foreach ( QProject *p, tp )
	{
		if ( p->isModified() )
			l << p->name();
		
	}
	#endif
	
	return l;
}

/*!
	\return The list of files owned by a given project
	\param project the file path of the project to search files in
	
	\note This function will NEVER open a project...
*/
QStringList EdyukManagerDock::files(const QString& project) const
{
	QProject *p = m_projectModel->project(project);
	
	return p ? p->files() : QStringList();
}

/*!
	\return The (opened) project owning a given file
	\param file filename to look for inside projects
*/
QString EdyukManagerDock::ownerProject(const QString& file) const
{
	Q_UNUSED(file)
	
	return QString();
}

/*!
	\return A list of filters supported by the parser
	This function is an helper for smart open dialogs.
*/
QStringList EdyukManagerDock::projectFilters() const
{
	return m_projectModel->projectLoader()->projectFilters();
}

/*!
	\brief Open a project
	\param filename path of the project file to load
*/
bool EdyukManagerDock::openProject(const QString& filename)
{
	return m_projectModel->openProject(filename);
}

/*!
	\brief Open a project
	\param filename path of the project file to load
*/
void EdyukManagerDock::saveProject(const QString& filename)
{
	return m_projectModel->saveProject(filename);
}

/*!
	\brief Open a project
	\param filename path of the project file to load
*/
bool EdyukManagerDock::closeProject(const QString& filename)
{
	return m_projectModel->closeProject(filename);
}

/*!
	\brief Popup a project settings dialog
*/
void EdyukManagerDock::projectOptions(const QString& filename)
{
	QProject *p = m_projectModel->project(filename);
	
	if ( !p )
	{
		//qWarning("failed to lookup %s", qPrintable(filename));
		return;
	}
	
	p->settings();
}

/*!
	\brief Add file(s) to a project
*/
void EdyukManagerDock::projectAddFiles(const QString& filename)
{
	QProject *p = m_projectModel->project(filename);
	
	if ( !p )
	{
		//qWarning("failed to lookup %s", qPrintable(filename));
		return;
	}
	
	p->actionTriggered(QProjectModel::tr("Add file(s)"));
}

/*!
	\brief Add file(s) to a project
*/
void EdyukManagerDock::projectAddFiles(const QString& filename,
										const QStringList& files)
{
	QProject *p = m_projectModel->project(filename);
	
	if ( !p )
	{
		//qWarning("failed to lookup %s", qPrintable(filename));
		return;
	}
	
	foreach ( const QString& f, files )
		p->addFile(f);
	
}

/*!
	\brief Save all opened projects
*/
void EdyukManagerDock::saveAllProjects() const
{
	m_projectModel->saveAll();
}

/*!
	\brief Send pseudo closure request to project model to make
	sure all project the user cares about are saved
	\see closeAllProjects()
*/
bool EdyukManagerDock::tryCloseAllProjects() const
{
	return m_projectModel->tryCommitAll();
}

/*!
	\brief Close all opened projects without a moment's hesitation
	\see tryCloseAllProjects()
*/
void EdyukManagerDock::closeAllProjects() const
{
	m_projectModel->closeAll(true);
}

/*!
	\internal
*/
void EdyukManagerDock::actionRequested(const QString& act,
										const QStringList& args)
{
	//qDebug("action %s", qPrintable(act));
	
	if ( act == "open" )
	{
		if ( !args.count() )
			return;
		
		QString fn = args.at(0);
		
		QWidget *w = m_parent ? m_parent->fileOpen(fn) : 0;
		
		if ( !w || (w == ((QWidget*)-1)) )
			return;
		
		int idx;
		QEditor *e = qobject_cast<QEditor*>(w);
		
		if ( !e )
			return;
		
		idx = args.indexOf("-l");
		
		if ( (idx != -1) && (args.count() > (idx + 1)) )
		{
			QDocument *d = e->document();
			QDocumentCursor c(d, args.at(idx + 1).toInt());
			
			e->setCursor(c);
			e->setFocus();
			return;
		}
		
		idx = args.indexOf("-rx");
		
		if ( (idx != -1) && (args.count() > (idx + 1)) )
		{
			// find a pattern inside some text
			QString pattern = args.at(idx + 1);
			
			//qDebug("looking for rx pattern : \"%s\"", qPrintable(pattern));
			
			QDocumentSearch s(e, pattern, QDocumentSearch::RegExp | QDocumentSearch::Silent);
			
			s.next(false);
			
			if ( !s.cursor().isNull() && s.cursor().hasSelection() )
			{
				QDocumentCursor c = s.cursor();
				c.clearSelection();
				e->setCursor(c);
				e->setFocus();
				return;
			} else {
				//qDebug("rx not found...");
			}
		}
		
		idx = args.indexOf("-s");
		
		if ( (idx != -1) && (args.count() > (idx + 1)) )
		{
			// find a pattern inside some text
			QString pattern = args.at(idx + 1);
			
			//qDebug("looking for str pattern : \"%s\"", qPrintable(pattern));
			
			QDocumentSearch s(e, pattern, QDocumentSearch::WholeWords | QDocumentSearch::CaseSensitive | QDocumentSearch::Silent);
			
			s.next(false);
			
			if ( !s.cursor().isNull() && s.cursor().hasSelection() )
			{
				QDocumentCursor c = s.cursor();
				c.clearSelection();
				e->setCursor(c);
				e->setFocus();
				return;
			} else {
				//qDebug("string not found...");
			}
		}
		
	} else {
		qWarning("Unhandled action requested by managed models : %s",
				qPrintable(act));
	}
}

/*!
	\internal
*/
void EdyukManagerDock::fileOpened(const QString& fn)
{
	//qDebug("dock:opened.");
	int last = 0;
	QString lbl = QFileInfo(fn).fileName();
	QRegExp duplicatePattern(QString("%1(?: (\\d+))?").arg(lbl));
	
	for ( int i = 0; i < m_scratchpad->count(); ++i )
	{
		if ( duplicatePattern.exactMatch(m_scratchpad->item(i)->text()) )
		{
			if ( duplicatePattern.cap(1).count() )
				last = qMax(last, duplicatePattern.cap(1).toInt());
			
		}
	}
	
	if ( last )
	{
		lbl += " (";
		lbl += QString::number(last);
		lbl += ")";
	}
	
	QListWidgetItem *i = new QListWidgetItem(lbl);
	i->setToolTip(fn);
	
	m_scratchpad->addItem(i);
}

/*!
	\internal
*/
void EdyukManagerDock::fileClosed(const QString& fn)
{
	//qDebug("dock:closed.");
	
	for ( int i = 0; i < m_scratchpad->count(); ++i )
	{
		if ( m_scratchpad->item(i)->toolTip() == fn )
		{
			delete m_scratchpad->takeItem(i);
			--i;
		}
	}
}

/*!
	\internal
*/
void EdyukManagerDock::itemChanged(QListWidgetItem *i)
{
	if ( !i || !m_parent )
		return;
	
	m_parent->fileOpen(i->toolTip());
}

/*!
	\internal
*/
void EdyukManagerDock::fileActivated(const QString& fn)
{
	if ( !m_parent || !QFile::exists(fn) )
		return;
	
	m_parent->fileOpen(fn);
}

/*!
	
*/
void EdyukManagerDock::fileContextMenu(const QPoint& p)
{
	QListWidgetItem *i = m_scratchpad->itemAt(p);
	
	if ( !i )
		return;
	
	QMenu m;
	QAction *a = m.addAction(tr("Close"));
	
	if ( a == m.exec(m_scratchpad->mapToGlobal(p)) )
	{
		QWidget *w = m_parent->window(i->toolTip());
		
		if ( w )
			w->close();
		
	}
}

/*!
	\internal
*/
void EdyukManagerDock::projectAdded(QProject *p)
{
	if ( m_projectModel->projectCount() == 1 )
		emit projectsOpened(true);
	
	emit projectOpened(p->name());
	
	m_stack->setCurrentIndex(1);
	
	//qDebug("adding project %s", qPrintable(p->name()));
	
	m_codeModel->addGroup(p->name(), p->files(QProject::Recursive));
	
	m_projectSelection->addItem(p->name());
	m_projectSelection->setCurrentIndex(m_projectSelection->count() - 1);
}

/*!
	\internal
*/
void EdyukManagerDock::projectRemoved(QProject *p)
{
	if ( !m_projectModel->projectCount() )
		emit projectsOpened(false);
	
	emit projectClosed(p->name());
	
	m_codeModel->removeGroup(p->name());
	
	int idx = m_projectSelection->findText(p->name());
	
	if ( idx != -1 )
		m_projectSelection->removeItem(idx);
}

/*!
	\internal
*/
void EdyukManagerDock::projectReloaded(QProject *p, QProject *n)
{
	
}

/*!
	\brief Update class tree according to updated files (if needed)
	
	This process is meant to allow class tree update when the build
	process generates new files which contain relevant information
	(e.g uic-generated headers for Qt projects).
*/
void EdyukManagerDock::processFileChanges()
{
	QList<QProject*> l = m_projectModel->projects();
	
	foreach ( QProject *p, l )
	{
		QStringList files = p->files(QProject::Recursive);
		
		foreach ( QString f, m_pendingFilesChange )
		{
			if ( !files.contains(f) )
				continue;
			
			m_codeModel->updateGroup(p->name(), f);
		}
	}
}

/*!

*/
int EdyukManagerDock::displayMode() const
{
	return m_stack->mode();
}

/*!

*/
void EdyukManagerDock::setDisplayMode(int mode)
{
	m_stack->setMode(mode);
}

/*!
	\brief Add a file to the pending change list
	
	\see processFileChanges()
*/
void EdyukManagerDock::fileChanged(const QString& fn)
{
	m_pendingFilesChange << fn;
}

/*!
	\brief Add several files to the pending change list
	
	\see processFileChanges()
*/
void EdyukManagerDock::filesChanged(const QStringList& fn)
{
	m_pendingFilesChange << fn;
}

/*!
	\internal
*/
void EdyukManagerDock::tryChangeActiveProject(const QString& filename)
{
	if ( m_blockTryRecursion )
		return;
	
	if (
			QBuildEngine::instance()->taskRunning()
		||
			QDebuggingEngine::instance()->isInDebuggingSession()
		)
	{
		m_blockTryRecursion = true;
		
		QProject *p = m_projectView->activeProject();
		m_projectSelection->setCurrentIndex(m_projectSelection->findText(p ? p->name() : QString()));
		
		QMessageBox::information(0,
								tr("Action forbidden"),
								tr(
									"You cannot change the current project during a debugging session "
									"or while a build task is running."
								)
							);
		
		m_blockTryRecursion = false;
		
		return;
	}
	
	m_projectView->setActiveProject(filename);
	
	emit activeProjectChanged(filename);
	
	if ( m_parent )
	{
		QMetaObject::invokeMethod(
								m_parent,
								"activeProjectChanged",
								Q_ARG(QProject*, m_projectView->activeProject())
							);
	}
}

/*!
	\internal
*/
void EdyukManagerDock::emitActiveProjectChanged(const QString& filename)
{
	int idx = m_projectSelection->findText(filename);
	
	if ( idx == m_projectSelection->currentIndex() )
		return;
	
	if (
			QBuildEngine::instance()->taskRunning()
		||
			QDebuggingEngine::instance()->isInDebuggingSession()
		)
	{
		QString curProject = m_projectSelection->currentText();
		m_projectView->setActiveProject(curProject);
		
		QMessageBox::information(0,
								tr("Action forbidden"),
								tr(
									"You cannot change the current project during a debugging session "
									"or while a build task is running."
								)
							);
		
		return;
	}
	
	if ( idx != -1 )
		m_projectSelection->setCurrentIndex(idx);
	
	emit activeProjectChanged(filename);
	
	if ( m_parent )
	{
		QMetaObject::invokeMethod(
								m_parent,
								"activeProjectChanged",
								Q_ARG(QProject*, m_projectView->activeProject())
							);
	}
}

/*!
	\internal
*/
void EdyukManagerDock::fileAdded(const QString& fn, QProject *p)
{
	if ( !m_codeModel->topLevelNodes().count() )
		return;
	
	//qDebug("updating %s in %s. [add]", qPrintable(fn), qPrintable(p->name()));
	
	m_codeModel->updateGroup(p->name(), fn);
}

/*!
	\internal
*/
void EdyukManagerDock::fileRemoved(const QString& fn, QProject *p)
{
	if ( !m_codeModel->topLevelNodes().count() )
		return;
	
	QString proname = p->name();
	//qDebug("updating %s in %s. [remove]", qPrintable(fn), qPrintable(proname));
	
	//m_codeModel->updateGroup(p->name(), fn);
	
	
	QCodeNode *project;
	QByteArray filecxt = fn.toLocal8Bit();
	
	foreach ( project, m_codeModel->topLevelNodes() )
	{
		if ( !project )
			continue;
		
		QString n = QString::fromLocal8Bit(project->role(QCodeNode::Context));
		
		if ( n == proname )
		{
			//qDebug("updating --- %s", qPrintable(n));
			
			QStack<QCodeNode*> nodes;
			nodes.push(project);
			
			while ( nodes.count() )
			{
				QCodeNode *node = nodes.pop();
				int t = node->type();
				
				if (
						(t == QCodeNode::Group)
					||
						(t == QCodeNode::Language)
					||
						(t == QCodeNode::Namespace)
					)
				{
					foreach ( QCodeNode *child, node->children )
					{
						if ( child )
							nodes.push(child);
					}
					
					continue;
				}
				
				//qDebug("node %s [%s]", node->role(QCodeNode::Name).constData(), node->role(QCodeNode::Context).constData());
				
				if ( node->role(QCodeNode::Context) == filecxt )
				{
					//qDebug("removed node %s", node->role(QCodeNode::Name).constData());
					
					node->clear();
					
					if ( node != project )
					{
						//node->detach();
						delete node;
						//node->deleteLater();
					}
				}
			}
		}
	}
}
