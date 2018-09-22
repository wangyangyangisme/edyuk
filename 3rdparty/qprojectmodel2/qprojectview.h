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

#ifndef _QPROJECT_VIEW_H_
#define _QPROJECT_VIEW_H_

#include "qpm-config.h"

/*!
	\file qprojectview.h
	\brief Definition of the QProjectView class.
*/

#include <QTreeView>

#include <QPointer>
#include <QBasicTimer>

class QAction;
class QActionGroup;

class QProject;
class QProjectNode;
class QProjectModel;

class QSortFilterProxyModel;

class QPM_EXPORT QProjectView : public QTreeView
{
	Q_OBJECT
	
	public:
		enum ProjectActivationMode
		{
			Default,
			ActivateSubProjects,
			ActivateOnClick
		};
		
		QProjectView(QWidget *p = 0);
		virtual ~QProjectView();
		
		virtual void setModel(QAbstractItemModel *m);
		
		QProject* activeProject() const;
		
		int projectActivationMode() const;
		
		int actionDelay() const;
		QWidget* actionBar() const;
		
	public slots:
		void setActiveProject(QProject *p);
		void setActiveProject(const QString& p);
		
		void setProjectActivationMode(int mode);
		
		void setActionDelay(int d);
		
	signals:
		void clicked(QProjectNode *n);
		
		void fileActivated(const QString& fn);
		
		void activeProjectChanged(QProject *p);
		void activeProjectChanged(const QString& s);
		
	protected:
		virtual void mousePressEvent(QMouseEvent *e);
		virtual void mouseReleaseEvent(QMouseEvent *e);
		virtual void mouseDoubleClickEvent(QMouseEvent *e);
		
		virtual void mouseMoveEvent(QMouseEvent *e);
		virtual bool eventFilter(QObject *o, QEvent *e);
		
		QModelIndex source(const QModelIndex& idx) const;
		QModelIndex view  (const QModelIndex& idx) const;
		
	private slots:
		void projectAdded(QProject *p);
		void projectRemoved(QProject *p);
		void reloadingProject(QProject *p);
		void projectReloaded(QProject *old, QProject *p);
		
		void triggered(QAction *a);
		void clicked(const QModelIndex& index);
		void activated(const QModelIndex& index);
		void selected(const QModelIndex& current, const QModelIndex& previous);
		
	private slots:
		void expandActive();
		void activateProject(QProject *p);
		
	private:
		int m_reload;
		int m_delay, m_activationMode;
		
		QProject *m_active;
		QPointer<QProjectModel> m_model;
		
		QWidget *m_actionBar;
		QActionGroup *m_group;
		QBasicTimer m_autoHide;
		QList<QAction*> m_actions;
		QSortFilterProxyModel *m_proxy;
};

#endif // !_QPROJECT_VIEW_H_
