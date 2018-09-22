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

#include "qprojectview.h"

/*!
	\file qprojectview.cpp
	\brief Implementation of the QProjectView class.
*/

#include "qproject.h"
#include "qprojectmodel.h"
#include "qprojectproxymodel.h"

#include <QUrl>
#include <QTimer>
#include <QEvent>
#include <QAction>
#include <QTimerEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QToolButton>
#include <QActionGroup>
#include <QItemDelegate>
#include <QDesktopServices>

/*!
	\class QProjectView
	\brief A specialized tree view
	
	QProjectView is more than a simple QTreeView. It notifies
	activation of project nodes but also keep track of the
	active project - \see activeProject() - and allow activation
	criteria to be fine-tunned.
*/

class QProjectViewDelegate : public QItemDelegate
{
	public:
		QProjectViewDelegate(QProjectView *v)
		 : QItemDelegate(v), m_view(v)
		{
			
		}
		
		virtual void paint(QPainter *painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
		{
			QProject * p = 0;
			const QProjectModel *m = qobject_cast<const QProjectModel*>(index.model());
			const QProjectProxyModel *pm = qobject_cast<const QProjectProxyModel*>(index.model());
			
			if ( m )
			{
				p = dynamic_cast<QProject*>(m->node(index));
			} else if ( pm ) {
				p = dynamic_cast<QProject*>(pm->node(index));
			}
			
			if ( p && isActive(p) )
			{
				QStyleOptionViewItem custom(option);
				custom.font.setBold(true);
				QItemDelegate::paint(painter, custom, index);
			} else {
				QItemDelegate::paint(painter, option, index);
			}
		}
		
		bool isActive(QProject *p) const
		{
			if ( !p )
				return false;
			
			return p == m_view->activeProject();
		}
		
	private:
		QProjectView *m_view;
};

/*!
	\brief ctor
*/
QProjectView::QProjectView(QWidget *p)
 : QTreeView(p), m_delay(5000), m_activationMode(Default), m_active(0), m_proxy(0)
{
	setDragEnabled(true);
	setAcceptDrops(true);
	setMouseTracking(true);
	setContextMenuPolicy(Qt::ActionsContextMenu);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	
	setItemDelegate(new QProjectViewDelegate(this));
	
	m_group = new QActionGroup(this);
	
	m_actionBar = new QWidget(this);
	m_actionBar->setLayout(new QVBoxLayout);
	m_actionBar->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	m_actionBar->layout()->setMargin(2);
	m_actionBar->layout()->setSpacing(1);
	m_actionBar->installEventFilter(this);
	m_actionBar->hide();
	
	connect(m_group	, SIGNAL( triggered(QAction*) ),
			this	, SLOT  ( triggered(QAction*) ) );
	
	connect(this, SIGNAL( activated(QModelIndex) ),
			this, SLOT  ( activated(QModelIndex) ) );
	
	connect(this, SIGNAL( clicked(QModelIndex) ),
			this, SLOT  ( clicked(QModelIndex) ) );
	
}

/*!
	\brief dtor
*/
QProjectView::~QProjectView()
{
	
}

QModelIndex QProjectView::source(const QModelIndex& idx) const
{
	return m_proxy ? m_proxy->mapToSource(idx) : idx;
}

QModelIndex QProjectView::view  (const QModelIndex& idx) const
{
	return m_proxy ? m_proxy->mapFromSource(idx) : idx;
}

/*!
	\return The action delay
	The action delay determines the time during which the action
	bar will remain visible once mouse cursor has left it.
*/
int QProjectView::actionDelay() const
{
	return m_delay;
}

/*!
	\brief Set the action delay
	\see actionDelay()
*/
void QProjectView::setActionDelay(int d)
{
	m_delay = d;
}

/*!
	\return the action bar of the view used to interact with the content of
	the model
*/
QWidget* QProjectView::actionBar() const
{
	return m_actionBar;
}

/*!
	\return the current project activation mode
	
	\see ProjectActivationMode
*/
int QProjectView::projectActivationMode() const
{
	return m_activationMode;
}

/*!
	\brief Set the project activation mode
	
	\see projectActivationMode()
*/
void QProjectView::setProjectActivationMode(int mode)
{
	m_activationMode = mode;
}

/*!
	\return The currently active project
	
	\note Can be null
	
	\see setActiveProject(QProject *p)
	\see setActiveProject(const QString& p)
*/
QProject* QProjectView::activeProject() const
{
	return m_active;
}

/*!
	\brief Set the active project
	\param p project to set as active
	
	\note Does nothing if the given project is not in the underlying model
	
	\see setActiveProject(const QString& p)
*/
void QProjectView::setActiveProject(QProject *p)
{
	if ( !m_model )
		return;
	
	activateProject(p);
	
	setCurrentIndex(view(m_model->index(p)));
}

/*!
	\overload
	\see setActiveProject(QProject *p)
*/
void QProjectView::setActiveProject(const QString& p)
{
	if ( !m_model )
		return;
	
	setActiveProject(m_model->project(p));
}

/*!
	\internal
*/
void QProjectView::reloadingProject(QProject *p)
{
	// TODO : check for subprojects?
	if ( m_active != p )
		return;
	
	m_active = 0;
	m_reload = m_model->projects().indexOf(p);
}

/*!
	\brief Set a model to the view
	\param m model to set
	\warning m MUST inherit from QProjectModel or a qFatal() will be issued
*/
void QProjectView::setModel(QAbstractItemModel *m)
{
	if ( m_model )
	{
		disconnect(	m_model	, SIGNAL( projectAdded(QProject*) ),
					this	, SLOT  ( projectAdded(QProject*) ) );
		
		disconnect(	m_model	, SIGNAL( projectRemoved(QProject*) ),
					this	, SLOT  ( projectRemoved(QProject*) ) );
		
		disconnect(	m_model	, SIGNAL( reloadingProject(QProject*) ),
					this	, SLOT  ( reloadingProject(QProject*) ) );
		
		disconnect(	m_model	, SIGNAL( projectReloaded(QProject*, QProject*) ),
					this	, SLOT  ( projectReloaded(QProject*, QProject*) ) );
		
		disconnect(	m_model	, SIGNAL( fileActivated(QString) ),
					this	, SIGNAL( fileActivated(QString) ) );
		
		disconnect(	m_model	, SIGNAL( requestActivation(QProject*) ),
					this	, SLOT  ( activateProject(QProject*) ) );
		
		disconnect(	selectionModel(),
					SIGNAL( currentChanged(QModelIndex, QModelIndex) ),
					this			,
					SLOT  ( selected(QModelIndex, QModelIndex) ) );
		
		if ( m_proxy )
		{
			disconnect(	m_proxy	, SIGNAL( requestEdit(QModelIndex) ),
						this	, SLOT  ( edit(QModelIndex) ) );
		} else {
			disconnect(	m_model	, SIGNAL( requestEdit(QModelIndex) ),
						this	, SLOT  ( edit(QModelIndex) ) );
		}
	}
	
	m_proxy = 0;
	m_model = qobject_cast<QProjectModel*>(m);
	
	if ( !m_model )
	{
		m_proxy = qobject_cast<QProjectProxyModel*>(m);
		
		if ( !m_proxy )
		{
			qFatal("[%s:%i] %s : invalid model", __FILE__, __LINE__, __FUNCTION__);
		}
		
		m_model = qobject_cast<QProjectModel*>(m_proxy->sourceModel());
		
		if ( !m_model )
		{
			qFatal("[%s:%i] %s : invalid model", __FILE__, __LINE__, __FUNCTION__);
		}
	}
	
	connect(m_model	, SIGNAL( fileActivated(QString) ),
			this	, SIGNAL( fileActivated(QString) ) );
	
	connect(m_model	, SIGNAL( projectAdded(QProject*) ),
			this	, SLOT  ( projectAdded(QProject*) ) );
	
	connect(m_model	, SIGNAL( projectRemoved(QProject*) ),
			this	, SLOT  ( projectRemoved(QProject*) ) );
	
	connect(m_model	, SIGNAL( reloadingProject(QProject*) ),
			this	, SLOT  ( reloadingProject(QProject*) ) );
	
	connect(m_model	, SIGNAL( projectReloaded(QProject*, QProject*) ),
			this	, SLOT  ( projectReloaded(QProject*, QProject*) ) );
	
	connect(m_model	, SIGNAL( requestActivation(QProject*) ),
			this	, SLOT  ( activateProject(QProject*) ) );
	
	//m_model->setDetailed(true);
	
	QTreeView::setModel(m);
	
	connect(m	, SIGNAL( requestEdit(QModelIndex) ),
			this, SLOT  ( edit(QModelIndex) ) );
	
	connect(selectionModel(), SIGNAL( currentChanged(QModelIndex, QModelIndex) ),
			this			, SLOT  ( selected(QModelIndex, QModelIndex) ) );
	
}

/*!
	\internal
*/
void QProjectView::activateProject(QProject *p)
{
	if ( p == m_active )
		return;
	
	m_active = p;
	
	emit activeProjectChanged(p);
	emit activeProjectChanged(p ? p->name() : QString());
	
	QTimer::singleShot(50, this, SLOT( expandActive() ) );
}

/*!
	\internal
*/
void QProjectView::expandActive()
{
	QModelIndex idx = m_model->index(m_active);
	expand(m_proxy ? m_proxy->mapFromSource(idx) : idx);
}

/*!
	\internal
*/
void QProjectView::projectAdded(QProject *p)
{
	if ( p && !p->parent() )
		setActiveProject(p);
	
}

/*!
	\internal
*/
void QProjectView::projectRemoved(QProject *p)
{
	if (
			!p
		||
			!m_model
		||
			(
				(p != m_active)
			&&
				!p->subProjects(QProject::Recursive).contains(m_active)
			)
		)
		return;
	
	QList<QProject*> projects = m_model->projects();
	
	if ( projects.isEmpty() )
	{
		setActiveProject(0);
	} else {
		setActiveProject(projects.at(0));
	}
}

/*!
	\internal
*/
void QProjectView::projectReloaded(QProject *old, QProject *p)
{
	if ( m_reload == -1 )
	{
		QList<QProject*> projects = m_model->projects();
		
		if ( projects.isEmpty() )
		{
			setActiveProject(0);
		} else {
			setActiveProject(projects.at(0));
		}
		
		return;
	}
	
	m_active = p;
	setCurrentIndex(view(m_model->index(p)));
	
	m_reload = -1;
}

void QProjectView::mousePressEvent(QMouseEvent *e)
{
// 	QModelIndex idx = source(indexAt(e->pos()));
// 	bool noChange = m_model->node(idx) == m_selected;
	
	QTreeView::mousePressEvent(e);
}

void QProjectView::mouseReleaseEvent(QMouseEvent *e)
{
	return QTreeView::mouseReleaseEvent(e);
}

void QProjectView::mouseDoubleClickEvent(QMouseEvent *e)
{
	QTreeView::mouseDoubleClickEvent(e);
}

/*!
	\internal
 */
void QProjectView::mouseMoveEvent(QMouseEvent *e)
{
	if ( (geometry().bottomLeft() - e->pos()).manhattanLength() < 30 )
	{
		//m_actionBar->show();
	}
	
	QTreeView::mouseMoveEvent(e);
}

/*!
	\internal
*/
bool QProjectView::eventFilter(QObject *o, QEvent *e)
{
	if ( o == m_actionBar )
	{
		switch ( e->type() )
		{
			case QEvent::Show :
				m_autoHide.start(actionDelay(), m_actionBar);
				break;
				
			case QEvent::Hide :
				m_autoHide.stop();
				break;
				
			case QEvent::Enter :
				m_autoHide.stop();
				break;
				
			case QEvent::Leave :
				m_autoHide.start(actionDelay(), m_actionBar);
				break;
				
			case QEvent::Timer :
			{
				QTimerEvent *te = static_cast<QTimerEvent*>(e);
				
				if ( te->timerId() == m_autoHide.timerId() )
				{
					m_actionBar->hide();
				}
				
				break;
			}
				
			default:
				
				break;
		}
	}
	
	return QTreeView::eventFilter(o, e);
}

/*!
	\internal
*/
void QProjectView::triggered(QAction *a)
{
	QProjectNode *m_selected = m_model->node(source(selectionModel()->currentIndex()));
	
	if ( !m_selected || !a || !m_actions.contains(a) )
		return;
	
	m_selected->actionTriggered(a->text());
}

/*!
	\internal
*/
void QProjectView::clicked(const QModelIndex& index)
{
	
}

/*!
	\internal
 */
void QProjectView::activated(const QModelIndex& index)
{
	QProjectNode *n = m_model->node(source(index));
	
	// perform default action (opening in most cases...)
	if ( m_actions.count() )
	{
		//qDebug("preforming default action : %s", qPrintable(m_actions.at(0)->text()));
		n->actionTriggered(m_actions.at(0)->text());
	} else if ( n->type() == QProjectNode::File ) {
		emit fileActivated(m_active
						?
							m_active->absoluteFilePath(n->name())
						:
							n->name()
						);
	}
}

/*!
	\internal
*/
void QProjectView::selected(const QModelIndex& idx, const QModelIndex& previous)
{
	Q_UNUSED(previous)
	
	if ( !m_model )
		return;
	
	QProjectNode *n = m_model->node(source(idx));
	
	QProject *active = 
			n
		?
			(
				(n->type() == QProjectNode::Project)
			?
				dynamic_cast<QProject*>(n)
			:
				n->project()
			)
		:
			0
		;
	
	// remove actions
	bool bs = m_actions.isEmpty() || m_actionBar->isVisible();
	
	if ( m_actions.count() )
	{
		for ( int i = 0; i < m_actions.count(); ++i )
		{
			QAction *a = m_actions.at(i);
			
			removeAction(a);
			m_group->removeAction(a);
			delete a;
		}
		
		m_actions.clear();
	}
	//
	
	if ( n )
	{
		//m_selected = n;
		
		// add new actions
		QLayout *l = m_actionBar->layout();
		
		QProjectNode::ActionList al = n->actions();
		
		foreach ( QProjectNode::Action act, al )
		{
			QAction *a = new QAction(act.icon, act.label, this);
			
			
			m_group->addAction(a);
			m_actions << a;
			addAction(a);
		}
	}
	
	if ( m_activationMode & ActivateOnClick )
	{
		if ( active )
		{
			if ( !active->parent() || (m_activationMode & ActivateSubProjects) )
			{
				activateProject(active);
			} else {
				while ( active->parent() )
					active = active->project();
				
				activateProject(active);
			}
		} else {
			QList<QProject*> projects = m_model->projects();
			
			if ( projects.isEmpty() )
			{
				activateProject(0);
			} else {
				activateProject(projects.at(0));
			}
		}
	}
}
