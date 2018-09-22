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

#ifndef _ASSISTANT_PERSPECTIVE_H_
#define _ASSISTANT_PERSPECTIVE_H_

#include "plugin.h"

#include "qmdiperspective.h"

class Assistant;
class AssistantClient;

class QLabel;
class QFrame;
class QAction;
class QComboBox;
class QTextEdit;
class QLineEdit;
class QTreeView;
class QListView;
class QTabWidget;
class QTimerEvent;
class QPushButton;
class QTreeWidget;
class QListWidget;
class QDockWidget;
class QProgressBar;
class QListWidgetItem;

#if QT_VERSION >= 0x040400
class QHelpEngine;
#endif

class AssistantPerspective : public qmdiPerspective
{
	Q_OBJECT
	
	public:
		AssistantPerspective();
		virtual ~AssistantPerspective();
		
		virtual void retranslate();
		
		virtual QIcon icon() const;
		virtual QString name() const;
		
		virtual Affinity affinity(qmdiClient *c) const;
		
		virtual QStringList filters() const;
		virtual qmdiClient* open(const QString& filename);
		virtual bool canOpen(const QString& filename) const;
		
		virtual qmdiClient* createEmptyClient(qmdiClientFactory *f);
		
	protected:
		virtual void setMainWindow(qmdiMainWindow *w);
		virtual bool eventFilter(QObject *o, QEvent *e);
		
	private slots:
		void fullSearch();
		void indexSearch();
		
		void setDefaultProfile();
		void setProfile(const QString& p);
		
		void itemDoubleClicked(QListWidgetItem *i);
		
		void contentContextMenu(const QPoint& p);
		void indexContextMenu(const QPoint& p);
		void searchContextMenu(const QPoint& p);
		
		void linkActivated(const QUrl& url);
		void linksActivated(const QMap<QString, QUrl>& urls);
		
	private:
		bool m_integration;
		
		#if QT_VERSION >= 0x040400
		void openLink(const QUrl& url, bool newtab);
		bool contextMenu(const QPoint& pos, bool& newtab);
		
		QHelpEngine *m_engine;
		
		QLabel *lIndexSearch;
		QTabWidget *assistant;
		QLineEdit *leIndexSearch;
		QWidget *pIndex, *pSearch;
		QDockWidget *pDockAssistant;
		#else
		Assistant *pAssistant;
		bool initialized[3];
		
		QListView *pViewIndex;
		QListWidget *pViewSearch;
		QComboBox *pProfile;
		
		QPushButton *bFullSearch;
		
		QFrame *pPrepare;
		QLabel *lPrepare, *lProfile, *lIndexSearch, *lFullSearch;
		QProgressBar *pProgress;
		QWidget *pIndex, *pSearch;
		QTreeWidget *pContents;
		
		QLineEdit *leIndexSearch, *leFullSearch;
		
		QTabWidget *assistant;
		AssistantClient *browser;
		QDockWidget *pDockBrowser, *pDockAssistant;
		#endif
};

#endif // _CPP_QT_PERSPECTIVE_H_
