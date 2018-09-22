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

#ifndef _QMDI_MAIN_WINDOW_H_
#define _QMDI_MAIN_WINDOW_H_

#include "qmdi.h"

/*!
	\file qmdimainwindow.h
	\brief Definition of the qmdiMainWindow class.
*/

#include "qmdihost.h"
#include <QMainWindow>

#include <QPointer>
#include <QByteArray>
#include <QStringList>

class QMenu;
class QAction;
class QActionGroup;
class QStackedWidget;

class qmdiWorkspace;
class qmdiStatusBar;
class qmdiPerspective;
class qmdiClientFactory;

class QMDI_API qmdiMainWindow : public QMainWindow, public qmdiHost
{
	Q_OBJECT
	
	public:
		enum FocusStealing
		{
			NoFocus,
			Focus
		};
		
		qmdiMainWindow(QWidget *p = 0);
		virtual ~qmdiMainWindow();
		
		qmdiStatusBar* status() const;
		
		qmdiClientFactory* clientFactory() const;
		void setClientFactory(qmdiClientFactory *f);
		
		qmdiWorkspace* workspace() const;
		
		QStringList perspectiveNames() const;
		QList<qmdiPerspective*> perspectives() const;
		
		qmdiPerspective* perspective() const;
		void setPerspective(const QString& n);
		void setPerspective(qmdiPerspective *p);
		
		virtual QString filters() const;
		
		virtual qmdiClient* createEmptyClient();
		
		virtual void addPerspective(qmdiPerspective *p);
		virtual void removePerspective(qmdiPerspective *p);
		
		QString currentFile() const;
		QStringList openedFiles() const;
		QStringList modifiedFiles() const;
		
		QWidget* window(const QString& fn) const;
		bool isOpen(const QString& fn, bool focus);
		
		QWidget* activeWindow() const;
		QWidgetList windowList() const;
		void setActiveWindow(QWidget *w);
		
		void addWidget(QWidget *w);
		
		bool checkModified();
		
		bool perspectiveLocked() const;
		
		virtual void clientOpened(qmdiClient *c);
		virtual void clientClosed(qmdiClient *c);
		virtual void currentClientChanged(qmdiClient *c);
		
	public slots:
		void updateGUI();
		void lockPerspective(bool y);
		virtual void retranslate();
		
		bool closeAll();
		bool closeAll(bool ignore);
		virtual void saveAll();
		virtual void saveFile(const QString& s);
		virtual void saveFiles(const QStringList& l);
		
		virtual QWidget* fileOpen(const QString& name);
		
		void focusCurrentClient();
		
		virtual void forceClose();
		
	signals:
		void fileOpened(const QString& s);
		void fileClosed(const QString& s);
		
		void currentFileChanged(const QString &s);
		
		void currentPerspectiveChanged(qmdiPerspective *p);
		void currentPerspectiveAboutToChange(qmdiPerspective *p);
		
	private slots:
		void setPerspective();
		void perspectiveChanged(QAction *a);
		
	private:
		qmdiStatusBar *pStatus;
		qmdiWorkspace *pWorkspace;
		QPointer<qmdiPerspective> pCur;
		QPointer<qmdiPerspective> pNext;
		
		QMenu *pPerspecMenu;
		QActionGroup *pPerspecActions;
		
	protected:
		bool bLocked;
		
		QList<QAction*> actions;
		QList<qmdiPerspective*> perspec;
		QList<QByteArray> perspecStates;
		
		qmdiClientFactory *pFactory;
		
		QAction *aNew, *aOpen, *aSave, *aSaveAs, *aSaveAll,
				*aClose, *aCloseAll, *aPrint, *aExit,
				*aPerspective, *aNone, *aTile, *aCascade;
		
};

#endif // _QMDI_MAIN_WINDOW_H_
