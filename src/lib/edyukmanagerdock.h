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

#ifndef _EDYUK_MANAGER_DOCK_H_
#define _EDYUK_MANAGER_DOCK_H_

#include "edyuk.h"

/*!
	\file edyukmanagerdock.h
	\brief Definition of the EdyukManagerDock class
	
	\see EdyukManagerDock
*/

#include <QDockWidget>

#include "qsettingsclient.h"

class QComboBox;
class QListWidget;
class QListWidgetItem;
class QSortFilterProxyModel;

class QWidgetStack;

class QCodeView;
class QCodeModel;
class QCodeParser;

class QProject;
class QProjectView;
class QProjectModel;
class QProjectParser;
class QProjectProxyModel;

class qmdiMainWindow;

class EDYUK_EXPORT EdyukManagerDock : public QDockWidget
{
	Q_OBJECT
	
	public:
		EdyukManagerDock(qmdiMainWindow *p, QSettingsClient s);
		virtual ~EdyukManagerDock();
		
		int displayMode() const;
		
		QCodeView* codeView() const;
		QCodeModel* codeModel() const;
		
		QProjectView* projectView() const;
		QProjectModel* projectModel() const;
		
		virtual void retranslate();
		
		void addCodeParser(QCodeParser *p);
		void addProjectParser(QProjectParser *p);
		
		QString activeProject() const;
		QStringList openedProjects() const;
		QStringList modifiedProjects() const;
		
		QStringList files(const QString& project) const;
		QString ownerProject(const QString& file) const;
		
		bool openProject(const QString& filename);
		void saveProject(const QString& filename);
		bool closeProject(const QString& filename);
		
		void projectOptions(const QString& filename);
		void projectAddFiles(const QString& filename);
		void projectAddFiles(const QString& name, const QStringList& files);
		
		void saveAllProjects() const;
		void closeAllProjects() const;
		bool tryCloseAllProjects() const;
		
		QStringList projectFilters() const;
		
		int projectViewDetailLevel() const;
		void setProjectViewDetailLevel(int l);
		
	public slots:
		void processFileChanges();
		
		void setDisplayMode(int mode);
		
		void fileChanged(const QString& fn);
		void filesChanged(const QStringList& fn);
		
	signals:
		void projectsOpened(bool y);
		void projectOpened(const QString& fn);
		void projectClosed(const QString& fn);
		void activeProjectChanged(const QString& fn);
		
	private slots:
		void actionRequested(const QString& act, const QStringList& args);
		
		void fileOpened(const QString& fn);
		void fileClosed(const QString& fn);
		void itemChanged(QListWidgetItem *i);
		void fileActivated(const QString& fn);
		
		void projectAdded(QProject *p);
		void projectRemoved(QProject *p);
		void projectReloaded(QProject *p, QProject *n);
		
		void fileAdded(const QString& fn, QProject *p);
		void fileRemoved(const QString& fn, QProject *p);
		
		void tryChangeActiveProject(const QString& filename);
		void emitActiveProjectChanged(const QString& filename);
		
		void fileContextMenu(const QPoint& p);
		
	private:
		QWidgetStack *m_stack;
		
		QListWidget *m_scratchpad;
		
		QCodeView *m_codeView;
		QCodeModel *m_codeModel;
		QSortFilterProxyModel *m_codeProxy;
		
		QProjectView *m_projectView;
		QProjectModel *m_projectModel;
		QProjectProxyModel *m_projectProxyModel;
		
		QComboBox *m_projectSelection;
		
		qmdiMainWindow *m_parent;
		
		bool m_blockTryRecursion;
		QStringList m_pendingFilesChange;
};

#endif // ! _EDYUK_MANAGER_DOCK_H_
