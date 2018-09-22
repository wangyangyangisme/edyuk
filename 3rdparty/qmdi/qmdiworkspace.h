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

#ifndef _QMDI_WORKSPACE_H_
#define _QMDI_WORKSPACE_H_

#include "qmdi.h"

#include <QWidget>
#include "qmdiserver.h"

/*!
	\file qmdiworkspace.h
	\brief Definition of the qmdiWorkspace class.
	
	\see qmdiWorkspace
*/

#include <QList>
#include <QPoint>
#include <QPointer>

class qmdiHost;

class QMenu;
class QTabBar;
class QMdiArea;
class QToolButton;
class QMainWindow;
class QMdiSubWindow;

class QMDI_API qmdiWorkspace : public QWidget, public qmdiServer
{
	Q_OBJECT
	
	friend class qmdiPerspective;
	
	public:
		qmdiWorkspace(QMainWindow *p = 0, qmdiHost *h = 0);
		virtual ~qmdiWorkspace();
		
		QStringList files() const;
		QWidgetList windowList() const;
		
		QString file() const;
		QWidget* activeWindow() const;
		
	public slots:
		virtual void saveAll();
		virtual void saveCurrent();
		virtual void saveCurrentAs();
		
		virtual bool closeAll(bool bypassMod = false);
		virtual bool closeCurrent(bool bypassMod = false);
		
		virtual bool checkModified();
		
		virtual void printCurrent();
		
		void tile();
		void cascade();
		
		void addWidget(QWidget *e);
		virtual void addClient(qmdiClient *c);
		virtual void saveClientAs(qmdiClient *c);
		
		void setActiveWindow(QWidget *w, bool force = false);
		
		virtual void retranslate();
		
		virtual bool maybeSave(qmdiClient *c);
		
	signals:
		void indexChanged(int i);
		void indexChanged(QWidget *w);
		
		void widgetsOpened(bool y);
		void currentModified(bool y);
		void currentPrintable(bool y);
		
		void widgetAdded(QWidget* w);
		void widgetClosed(QWidget *w);
	
	protected slots:
		void fileNew();
		
		void currentChanged(int i);
		void subWindowActivated(QMdiSubWindow *w);
		
		void emitCurrentModified(bool m);
		void currentTitleChanged(const QString& t);
		
	protected:
		virtual void clientDeleted(QObject *o);
		
		virtual bool eventFilter(QObject *o, QEvent *e);
		
		virtual void showEvent(QShowEvent *e);
		virtual void hideEvent(QHideEvent *e);
		virtual void contextMenuEvent(QContextMenuEvent *e);
		
		virtual void dragEnterEvent(QDragEnterEvent *e);
		virtual void dragMoveEvent(QDragMoveEvent *e);
		virtual void dropEvent(QDropEvent *e);
		
		int tabAt(const QPoint& p);
		
	private:
		void mergeMDI(QWidget *w);
		
		QMenu *m_menu;
		QTabBar *m_tab;
		QMdiArea *m_area;
		QMainWindow *m_main;
		QToolButton *m_new, *m_close;
		
		QPointer<QWidget> m_current;
		
		QList<QWidget*> m_widgets;
		QList<QMdiSubWindow*> m_wrappers;
		
		// fix
		bool m_locked, m_maximizeOnShow;
		
		// d'n'd
		QPoint m_dragPos;
		bool m_mightDrag, m_dragging;
};

#endif
