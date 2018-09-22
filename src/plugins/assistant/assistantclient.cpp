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

#include "assistantclient.h"

#include "assistant.h"
#include "qmdiserver.h"

#include "edyukgui.h"
#include "edyukapplication.h"
#include "qshortcutmanager.h"

#include "plugin.h"

#include "ui_search.h"

#include <QUrl>
#include <QMenu>
#include <QLabel>
#include <QAction>
#include <QComboBox>
#include <QFileInfo>
#include <QStatusBar>
#include <QGridLayout>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextBrowser>
#include <QTextDocument>
#include <QDesktopServices>
#include <QContextMenuEvent>

#if QT_VERSION >= 0x040400
#include <QHelpEngine>
#endif

class AssistantSearchPanel : public QWidget, private Ui::Search
{
	Q_OBJECT
	
	public:
		AssistantSearchPanel(QTextBrowser *b, QWidget *p = 0)
		 : QWidget(p ? p : b), m_browser(b)
		{
			setupUi(this);
		}
		
	protected:
		virtual void showEvent(QShowEvent *e)
		{
			Q_UNUSED(e)
			
			leFind->setFocus();
		}
		
	private slots:
		void on_bNext_clicked()
		{
			search();
		}
		
		void on_bPrevious_clicked()
		{
			search(true);
		}
		
		void on_leFind_textEdited()
		{
			//search();
			if ( AssistantPlugin::configKey<bool>("qmdiPerspective/AssistantPerspective/liveSearch", false) )
			{
				QTextCursor c = m_browser->textCursor();
				c.setPosition(c.anchor());
				m_browser->setTextCursor(c);
				
				search();
			}
		}
		
		void on_leFind_returnPressed()
		{
			search();
		}
		
		void on_cbWords_toggled(bool y)
		{
			Q_UNUSED(y)
			
			m_prev = QTextCursor();
		}
		
		void on_cbCase_toggled(bool y)
		{
			Q_UNUSED(y)
			
			m_prev = QTextCursor();
		}
		
		void on_cbRegExp_toggled(bool y)
		{
			Q_UNUSED(y)
			
			m_prev = QTextCursor();
		}
		
		void on_cbCursor_toggled(bool y)
		{
			Q_UNUSED(y)
			
			m_prev = QTextCursor();
		}
		
	private:
		void search(bool backward = false)
		{
			QTextDocument::FindFlags f;
			QString txt = leFind->text();
			QTextDocument *d = m_browser->document();
			
			if ( backward )
				f |= QTextDocument::FindBackward;
			
			if ( cbWords->isChecked() )
				f |= QTextDocument::FindWholeWords;
			
			if ( cbCase->isChecked() )
				f |= QTextDocument::FindCaseSensitively;
			
			QTextCursor c =
								cbCursor->isChecked()
							?
								m_browser->textCursor()
							:
								(
									m_prev.isNull()
								?
									(
										backward
									?
										QTextCursor(d->end())
									:
										QTextCursor(d->begin())
									)
								:
									m_prev
								);
			
			if ( cbRegExp->isChecked() )
				c = d->find(QRegExp(txt), c, f);
			else
				c = d->find(txt, c, f);
			
			if ( c.isNull() )
			{
				leFind->setStyleSheet("color :  white; background-color : #FF6666;");
			} else {
				m_browser->setTextCursor(c);
				leFind->setStyleSheet("");
			}
			
			m_prev = c;
		}
		
		QTextCursor m_prev;
		QTextBrowser *m_browser;
};

class AssistantBrowser : public QTextBrowser
{
	Q_OBJECT
	
	public:
		#if QT_VERSION >= 0x040400
		AssistantBrowser(QHelpEngine *e, QWidget *p = 0)
		 : QTextBrowser(p), m_engine(e)
		#else
		AssistantBrowser(QWidget *p = 0)
		 : QTextBrowser(p)
		#endif
		{
			
		}
		
		virtual void setSource(const QUrl& u)
		{
			QUrl url(u);
			
			if ( url.scheme().isEmpty() )
				#if QT_VERSION >= 0x040400
				url.setScheme("qthelp");
				#else
				url.setScheme("file");
				#endif
			
			QString scheme = url.scheme();
			
			if (
					scheme == QLatin1String("ftp")
				||
					scheme == QLatin1String("http")
				||
					scheme == QLatin1String("https")
				||
					scheme == QLatin1String("svn")
				||
					scheme == QLatin1String("sftp")
				||
					scheme == QLatin1String("rsync")
				||
					scheme == QLatin1String("mailto")
				||
					url.toString().endsWith(QLatin1String(".pdf"))
				)
			{
				QDesktopServices::openUrl(u);
				return;
			} else if ( scheme == "qthelp" ) {
				#if QT_VERSION >= 0x040400
				QTextBrowser::setSource(m_engine->findFile(u));
				#else
				qWarning("Qt 4.4 help format unsupported : rebuild Edyuk with Qt 4.4");
				#endif
				return;
			} else if ( !url.isValid() ) {
				
				return;
			}
			
			QTextBrowser::setSource(url);
		}
		
		QVariant loadResource(int type, const QUrl &name)
		{
			#if QT_VERSION >= 0x040400
			QByteArray ba;
			
			if ( type < 4 )
			{
				ba = m_engine->fileData(name);
			}
			
			if ( ba.count() )
				return ba;
			#endif
			
			return QTextBrowser::loadResource(type, name);
		}
		
	private:
		#if QT_VERSION >= 0x040400
		QHelpEngine *m_engine;
		#endif
};

#if QT_VERSION >= 0x040400
AssistantClient::AssistantClient(QHelpEngine *e, QWidget *p)
 : qmdiWidget(p), m_engine(e)
{
#else
AssistantClient::AssistantClient(Assistant *a, QWidget *p)
 : qmdiWidget(p), pOwner(a)
{
#endif
	pUrl = new QComboBox(this);
	pUrl->setEditable(true);
	pUrl->setAutoCompletion(true);
	pUrl->setDuplicatesEnabled(false);
	
	#if QT_VERSION >= 0x040400
	pBrowser = new AssistantBrowser(e, this);
	#else
	pBrowser = new AssistantBrowser(this);
	pBrowser->setSearchPaths( QStringList( Assistant::docPath() ) );
	#endif
	pBrowser->setContextMenuPolicy(Qt::NoContextMenu);
	
	pLabel = new QLabel(tr("Address : "), this);
	
	m_search = new AssistantSearchPanel(pBrowser, this);
	
	QGridLayout *grid = new QGridLayout(this);
	grid->setMargin(5);
	grid->setSpacing(0);
	
	grid->addWidget(pLabel	,  0, 1,  1,  6);
	grid->addWidget(pUrl	,  0, 6,  1, 58);
	grid->addWidget(pBrowser,  1, 0, 46, 64);
	grid->addWidget(m_search, 47, 0,  1, 64);
	
	setLayout(grid);
	
	pMenu = new QMenu;
	
	aForward = new QAction(QIcon(":/forward.png"), tr("Forward"), this);
	aForward->setEnabled(false);
	EDYUK_SHORTCUT(aForward, tr("Qt Assistant"), "");
	connect(pBrowser, SIGNAL( forwardAvailable(bool) ),
			aForward, SLOT  ( setEnabled(bool) ) );
	connect(aForward, SIGNAL( triggered() ),
			pBrowser, SLOT  ( forward() ) );
	
	aBackward = new QAction(QIcon(":/backward.png"), tr("Backward"), this);
	aForward->setEnabled(false);
	EDYUK_SHORTCUT(aBackward, tr("Qt Assistant"), "");
	connect(pBrowser , SIGNAL( backwardAvailable(bool) ),
			aBackward, SLOT  ( setEnabled(bool) ) );
	connect(aBackward, SIGNAL( triggered() ),
			pBrowser , SLOT  ( backward() ) );
	
	aReload = new QAction(QIcon(":/reload.png"), tr("Reload"), this);
	EDYUK_SHORTCUT(aReload, tr("Qt Assistant"), "");
	connect(aReload	, SIGNAL( triggered() ),
			pBrowser, SLOT  ( reload() ) );
	
	aHome = new QAction(QIcon(":/home.png"), tr("Home"), this);
	EDYUK_SHORTCUT(aHome, tr("Qt Assistant"), "");
	connect(aHome	, SIGNAL( triggered() ),
			pBrowser, SLOT  ( home() ) );
	
	aFind = new QAction(QIcon(":/find.png"), tr("Search"), this);
	EDYUK_SHORTCUT(aFind, tr("Qt Assistant"), "CTRL+F");
	connect(aFind	, SIGNAL( triggered() ),
			m_search, SLOT  ( show() ) );
	
	aOpenLink = new QAction(tr("Open link"), this);
	connect(aOpenLink	, SIGNAL( triggered() ),
			this		, SLOT  ( openLink() ) );
	
	aOpenInTab = new QAction(tr("Open in new tab"), this);
	connect(aOpenInTab	, SIGNAL( triggered() ),
			this		, SLOT  ( openInNewTab() ) );
	
	menus["&Edit"]->addAction(aBackward);
	menus["&Edit"]->addAction(aForward);
	menus["&Edit"]->addSeparator();
	menus["&Edit"]->addAction(aFind);
	menus["&Edit"]->addSeparator();
	menus["&Edit"]->addAction(aReload);
	menus["&Edit"]->addAction(aHome);
	
	toolbars["Edit"]->addAction(aBackward);
	toolbars["Edit"]->addAction(aForward);
	toolbars["Edit"]->addSeparator();
	toolbars["Edit"]->addAction(aFind);
	toolbars["Edit"]->addSeparator();
	toolbars["Edit"]->addAction(aReload);
	toolbars["Edit"]->addAction(aHome);
	
	retranslate();
	
	connect(pBrowser, SIGNAL( sourceChanged(const QUrl&) ),
			this	, SLOT  ( sourceChanged(const QUrl&) ) );
	
	connect(pUrl, SIGNAL( currentIndexChanged(const QString&) ),
			this, SLOT  ( currentIndexChanged(const QString&) ) );
	
	connect(pBrowser, SIGNAL( highlighted(const QString&) ),
			this	, SLOT  ( highlighted(const QString&) ) );
	
	setContentModified(false);
}

AssistantClient::~AssistantClient()
{
	#if QT_VERSION < 0x040400
	assistant()->removeClient(this);
	#endif
}

void AssistantClient::openLink(const QString& lnk)
{
	pBrowser->setSource(lnk);
}

void AssistantClient::openInNewTab(const QString& lnk)
{
	#if QT_VERSION >= 0x040400
	AssistantClient *a = new AssistantClient(m_engine);
	#else
	AssistantClient *a = assistant()->createClient();
	#endif
	
	a->openLink(lnk);
	
	if ( server() )
		server()->addClient(a);
}

void AssistantClient::retranslate()
{
	pLabel->setText(tr("Address : "));
	
	aForward->setText(tr("Forward"));
	aBackward->setText(tr("Backward"));
	aReload->setText(tr("Reload"));
	aHome->setText(tr("Home"));
	aFind->setText(tr("Search"));
	
	aOpenLink->setText(tr("Open link"));
	aOpenInTab->setText(tr("Open in new tab"));
	
	toolbars.setTranslation("Edit", tr("Edit"));
}

void AssistantClient::contextMenuEvent(QContextMenuEvent *e)
{
	e->accept();
	
	anchor = pBrowser->anchorAt( pBrowser->mapFromParent( e->pos() ) );
	
	pMenu->clear();
	
	if ( !anchor.isEmpty() )
	{
		if ( anchor.at(0) == '#' )
		{
			QString src = pBrowser->source().toString();
			int hash = src.indexOf('#');
			
			anchor.prepend( (hash > -1) ? src.left(hash) : src );
		}
		
		pMenu->addAction(aOpenLink);
		pMenu->addAction(aOpenInTab);
		pMenu->addSeparator();
	}
	
	pMenu->addAction(aBackward);
	pMenu->addAction(aForward);
	pMenu->addSeparator();
	pMenu->addAction(aReload);
	pMenu->addAction(aHome);
	pMenu->addSeparator();
	pMenu->addActions( pBrowser->createStandardContextMenu()->actions() );
	
	pMenu->exec(e->globalPos());
}

void AssistantClient::openLink()
{
	if ( anchor.count() )
		openLink(anchor);
}

void AssistantClient::openInNewTab()
{
	if ( anchor.count() )
		openInNewTab(anchor);
}

void AssistantClient::highlighted(const QString& link)
{
	COMPONENT(gui)->statusBar()->showMessage(link);
}

void AssistantClient::sourceChanged(const QUrl& url)
{
	pUrl->setEditText(url.toString());
	
	setTitle(	tr("Assistant : %1").arg(
				QFileInfo(url.toString()).completeBaseName() )
			);
}

void AssistantClient::currentIndexChanged(const QString& s)
{
	openLink(s);
}

#include "assistantclient.moc"
