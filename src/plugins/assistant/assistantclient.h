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

#ifndef _ASSISTANT_CLIENT_H_
#define _ASSISTANT_CLIENT_H_

#include "plugin.h"

#include <QString>
#include "qmdiwidget.h"

class Assistant;

class QUrl;
class QMenu;
class QLabel;
class QAction;
class QComboBox;
class QTextBrowser;

class AssistantSearchPanel;

#if QT_VERSION >= 0x040400
class QHelpEngine;
#endif

class AssistantClient : public qmdiWidget
{
	Q_OBJECT
	
	public:
		#if QT_VERSION >= 0x040400
		AssistantClient(QHelpEngine *e, QWidget *w = 0);
		#else
		AssistantClient(Assistant *p, QWidget *w = 0);
		#endif
		virtual ~AssistantClient();
		
		virtual void retranslate();
		
		#if QT_VERSION < 0x040400
		inline Assistant* assistant() { return pOwner; }
		#endif
		
		static QString docPath();
		
	public slots:
		void openLink(const QString& lnk);
		void openInNewTab(const QString& lnk);
		
	protected:
		virtual void contextMenuEvent(QContextMenuEvent *e);
		
	private slots:
		void openLink();
		void openInNewTab();
		
		void highlighted(const QString& link);
		void sourceChanged(const QUrl& url);
		void currentIndexChanged(const QString& s);
		
	private:
		QMenu *pMenu;
		
		#if QT_VERSION < 0x040400
		Assistant *pOwner;
		#else
		QHelpEngine *m_engine;
		#endif
		
		QAction *aForward, *aBackward, *aReload, *aHome,
				*aOpenLink, *aOpenInTab,
				*aFind;
		
		QLabel *pLabel;
		QComboBox *pUrl;
		QTextBrowser *pBrowser;
		AssistantSearchPanel *m_search;
		
		QString anchor;
};

#endif // !_ASSISTANT_CLIENT_H_
