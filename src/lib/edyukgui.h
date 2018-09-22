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

#ifndef _EDYUK_GUI_H_
#define _EDYUK_GUI_H_

#include "edyuk.h"

/*!
	\file edyukgui.h
	\brief Definition of the EdyukGUI class
	
	\see EdyukGUI
*/

/*!
	\defgroup gui GUI layer
*/

#include "qmdimainwindow.h"
#include <QSystemTrayIcon>

class QAction;
class QCloseEvent;
class QActionGroup;
class QSettingsServer;
class QSystemTrayIcon;
class QSessionManager;

class QProject;
class QCodeParser;
class QProjectParser;
class QCodeCompletionEngine;
class QLanguageDefinition;

class EdyukLogDock;
class EdyukManagerDock;

class EDYUK_EXPORT EdyukGUI : public qmdiMainWindow
{
	friend class EdyukApplication;
	
	Q_OBJECT
	
	public:
		EdyukGUI(QSettingsServer *s);
		virtual ~EdyukGUI();
		
		virtual QString filters();
		
		virtual void addPerspective(qmdiPerspective *p);
		virtual void removePerspective(qmdiPerspective *p);
		
		QString activeProject() const;
		QStringList openedProjects() const;
		
		QStringList files(const QString& project) const;
		QString ownerProject(const QString& file) const;
		
		void addCodeParser(QCodeParser *p);
		void addProjectParser(QProjectParser *p);
		void addCompletionEngine(QCodeCompletionEngine *e);
		void addLanguageDefinition(QLanguageDefinition *l);
		
		virtual void saveAll();
		
	public slots:
		virtual void retranslate();
		
		void setRecentAction(QAction *a);
		void setLanguageAction(QAction *a);
		
		void mergeExtraClient(qmdiClient *c, bool on);
		void insertExtraDockWidget(QWidget *w, bool on);
		
		void setDefaultPerspective();
		void setPerspectives(QList<qmdiPerspective*> l);
		
		void fileNew();
		void fileOpen();
		
		// dirty hack to avoid resolution operators everywhere...
		inline QWidget* fileOpen(const QString& f)
		{ return qmdiMainWindow::fileOpen(f); }
		
		void projectNew();
		
		void projectOpen();
		bool projectOpen(const QString& name);
		
		void projectSave();
		void projectSave(const QString& name);
		
		void projectClose();
		bool projectClose(const QString& name);
		
		void projectAdd();
		void projectAdd(const QString& name);
		void projectAdd(const QString& name, const QStringList& files);
		
		void projectRemove();
		void projectRemove(const QString& name);
		
		void projectNewFile();
		void projectNewFile(const QString& name);
		
		void projectOptions();
		void projectOptions(const QString& name);
		
		void about();
		void help();
		
		void message(const QString& s);
		
		bool tryClose(QSessionManager *mgr);
		
	protected:
		void setupMenu();
		void setupActions();
		
		virtual void forceClose();
		
		virtual void closeEvent(QCloseEvent *e);
		
	signals:
		void projectOpened(const QString& filename);
		
	protected slots:
		void styleChanged(QAction *a);
		
		void projectsOpened(bool y);
		void setProjectDetailed(bool y);
		
		void toolsChanged(QActionGroup *g);
		
		void swapHeaderSource();
		
		void perspectiveChanged(qmdiPerspective *p);
		void perspectiveAboutToChange(qmdiPerspective *p);
		
		void refreshClassTree();
		void refreshTargetList();
		void buildTaskAboutToStart();
		void notifyFileChange(const QStringList& l);
		
		void activeProjectChanged(QProject *p);
		void buildModeChanged(const QString& mode);
		void execTargetChanged(const QString& target);
		
		void trayIconActivated(QSystemTrayIcon::ActivationReason r);
		
	private:
		void translateActions();
		
		QAction *aReopen, *aSwapHeaderSource,
				*aStyle,
				*aProjectDetailed,
					*aProjectOpen, *aProjectNew, *aProjectSave, *aProjectClose,
					*aProjectNewFile, *aProjectAdd, *aProjectRemove,
					*aProjectOptions,
				*aOptions, *aShortcuts, *aTools, *aLanguage,
				*aAboutEdyuk, *aEdyukHelp,
				*aFocusCurrent;
		
		QSystemTrayIcon *m_trayIcon;
		
		QSettingsServer *pServer;
		
		EdyukLogDock *m_logDock;
		EdyukManagerDock *m_managerDock;
		
		QList<QCodeCompletionEngine*> m_cce;
};

#endif // !_EDYUK_GUI_H_
