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

#include "assistantperspective.h"

#include "plugin.h"

#include "assistant.h"
#include "assistantclient.h"

#include "edyukgui.h"
#include "edyukapplication.h"

#include "qrcedit.h"

#include "qeditor.h"
#include "qcodeedit.h"
#include "qdocument.h"
#include "qdocumentline.h"
#include "qlinemarksinfocenter.h"
#include "qeditorfactory.h"
#include "qlanguagefactory.h"

#include <QDir>
#include <QMenu>
#include <QIcon>
#include <QLabel>
#include <QFrame>
#include <QTimer>
#include <QAction>
#include <QProcess>
#include <QFileInfo>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QTabWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QListWidget>
#include <QDockWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QApplication>
#include <QLibraryInfo>
#include <QInputDialog>

#include <QDockWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#if QT_VERSION >= 0x040400
#include <QHelpEngine>
#include <QHelpIndexWidget>
#include <QHelpSearchEngine>
#include <QHelpContentWidget>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#endif

AssistantPerspective::AssistantPerspective()
 :	qmdiPerspective(0)
{
	m_integration = AssistantPlugin::configKey<bool>("qmdiPerspective/AssistantPerspective/integration", true);
	
	if ( m_integration )
	{
	#if QT_VERSION < 0x040400
	initialized[0] = initialized[1] = initialized[2] = false;
	
	pAssistant = new Assistant(mainWindow(), this);
	#else
	m_engine = new QHelpEngine(Edyuk::settingsPath() + "assistant.qhc", this);
	
	QStringList l = m_engine->registeredDocumentations();
	QFileInfoList fl = QDir(QLibraryInfo::location(QLibraryInfo::DocumentationPath) + "/qch")
							.entryInfoList(QDir::Files | QDir::Readable);
	
	foreach ( const QFileInfo& fi, fl )
	{
		if ( l.contains(fi.filePath()) || fi.suffix() != "qch" )
			continue;
		
		m_engine->registerDocumentation(fi.filePath());
	}
	
	#endif
	
	pDockAssistant = new QDockWidget(tr("Qt Assistant"));
	QGridLayout *grid;
	
	#if QT_VERSION >= 0x040400
	assistant = new QTabWidget(pDockAssistant);
	
	m_engine->contentWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
	
	pIndex = new QWidget(assistant);
	grid = new QGridLayout(pIndex);
	
	lIndexSearch = new QLabel(pIndex);
	lIndexSearch->setText(tr("Filter :"));
	grid->addWidget(lIndexSearch, 0, 0, 1, 3); 
	
	leIndexSearch = new QLineEdit(pIndex);
	connect(leIndexSearch	, SIGNAL( editingFinished() ),
			this			, SLOT  ( indexSearch() ) );
	grid->addWidget(leIndexSearch, 1, 0, 1, 3);
	
	m_engine->indexWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
	grid->addWidget(m_engine->indexWidget(), 2, 0, 8, 3);
	
	pSearch = new QWidget;
	QVBoxLayout *v = new QVBoxLayout(pSearch);
	v->addWidget(m_engine->searchEngine()->queryWidget());
	v->addWidget(m_engine->searchEngine()->resultWidget());
	
	m_engine->searchEngine()->resultWidget()->setContextMenuPolicy(Qt::CustomContextMenu);
	
	
	assistant->addTab(m_engine->contentWidget(), tr("&Contents"));
	assistant->addTab(pIndex, tr("&Index"));
	assistant->addTab(pSearch, tr("&Search"));
	
	connect(m_engine->contentWidget()	, SIGNAL( linkActivated(QUrl) ),
			this						, SLOT  ( linkActivated(QUrl) ) );
	
	connect(m_engine->contentWidget()	, SIGNAL( customContextMenuRequested(QPoint) ),
			this						, SLOT  ( contentContextMenu(QPoint) ) );
	
	connect(m_engine->indexWidget()	, SIGNAL( linkActivated(QUrl, QString) ),
			this					, SLOT  ( linkActivated(QUrl) ) );
	
	connect(m_engine->indexWidget()	, SIGNAL( linksActivated(QMap<QString, QUrl>, QString) ),
			this					, SLOT  ( linksActivated(QMap<QString, QUrl>) ) );
	
	connect(m_engine->indexWidget()	, SIGNAL( customContextMenuRequested(QPoint) ),
			this					, SLOT  ( indexContextMenu(QPoint) ) );
	
	connect(m_engine->searchEngine()->queryWidget()	, SIGNAL( search() ),
			this									, SLOT  ( fullSearch() ) );
	
	connect(m_engine->searchEngine()->resultWidget(), SIGNAL( requestShowLink(QUrl) ),
			this									, SLOT  ( linkActivated(QUrl) ) );
	
	connect(m_engine->searchEngine()->resultWidget(), SIGNAL( customContextMenuRequested(QPoint) ),
			this									, SLOT  ( searchContextMenu(QPoint) ) );
	
	pDockAssistant->setWidget(assistant);
	#else
	QWidget *w = new QWidget(pDockAssistant);
	
	assistant = new QTabWidget(w);
	
	pContents = new QTreeWidget;
	pContents->installEventFilter(this);
	pContents->setHeaderLabels(QStringList("Content by section"));
	pContents->setContextMenuPolicy(Qt::CustomContextMenu);
	
	pIndex = new QWidget;
	pIndex->installEventFilter(this);
	
	grid = new QGridLayout;
	
	lIndexSearch = new QLabel(pIndex);
	grid->addWidget(lIndexSearch, 0, 0); 
	
	leIndexSearch = new QLineEdit(pIndex);
	connect(leIndexSearch, SIGNAL( editingFinished() ),
			this		, SLOT  ( indexSearch() ) );
	grid->addWidget(leIndexSearch, 1, 0, 1, 3);
	
	pViewIndex = new QListView(pIndex);
	pViewIndex->setContextMenuPolicy(Qt::CustomContextMenu);
	grid->addWidget(pViewIndex, 2, 0, 8, 3);
	
	pIndex->setLayout(grid);
	
	
	pSearch = new QWidget;
	pSearch->installEventFilter(this);
	
	grid = new QGridLayout;
	
	lFullSearch = new QLabel(pSearch);
	grid->addWidget(lFullSearch, 0, 0); 
	
	leFullSearch = new QLineEdit(pSearch);
	connect(leFullSearch, SIGNAL( editingFinished() ),
			this		, SLOT  ( fullSearch() ) );
	grid->addWidget(leFullSearch, 1, 0, 1, 3); 
	
	bFullSearch = new QPushButton(tr("&Search"), pSearch);
	grid->addWidget(bFullSearch, 2, 2);
	connect(bFullSearch	, SIGNAL( clicked() ),
			this		, SLOT  ( fullSearch() ) );
	
	pViewSearch = new QListWidget(pSearch);
	connect(pViewSearch	, SIGNAL( itemDoubleClicked(QListWidgetItem*) ),
			this		, SLOT  ( itemDoubleClicked(QListWidgetItem*) ) );
	
	connect(pViewSearch	, SIGNAL( customContextMenuRequested(QPoint) ),
			this		, SLOT  ( searchContextMenu(QPoint) ) );
	
	pViewSearch->setContextMenuPolicy(Qt::CustomContextMenu);
	grid->addWidget(pViewSearch, 3, 0, 9, 3);
	
	
	pSearch->setLayout(grid);
	
	assistant->addTab(pContents, tr("&Contents"));
	assistant->addTab(pIndex, tr("&Index"));
	assistant->addTab(pSearch, tr("&Search"));
	
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->setMargin(0);
	vbox->setSpacing(0);
	
	QHBoxLayout *hbox = new QHBoxLayout;
	
	lProfile = new QLabel(w);
	hbox->addWidget(lProfile);
	
	pProfile = new QComboBox(w);
	pProfile->addItems( pAssistant->profiles() );
	connect(pProfile, SIGNAL( currentIndexChanged(QString) ),
			this	, SLOT  ( setProfile(QString) ) );
	
	hbox->addWidget(pProfile);
	
	vbox->addLayout(hbox);
	vbox->addWidget(assistant);
	
	pPrepare = new QFrame(w);
	pPrepare->setFrameShape(QFrame::StyledPanel);
	pPrepare->setFrameShadow(QFrame::Raised);
	
	hbox = new QHBoxLayout;
	
	lPrepare = new QLabel(pPrepare);
	lPrepare->setText(tr("Preparing..."));
	hbox->addWidget(lPrepare);
	
	pProgress = new QProgressBar(pPrepare);
	hbox->addWidget(pProgress);
	
	pPrepare->setLayout(hbox);
	
	vbox->addWidget(pPrepare);
	
	w->setLayout(vbox);
	pDockAssistant->setWidget(w);
	
	pPrepare->hide();
	#endif
	
	pDockAssistant->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	
	addDockWidget(pDockAssistant, "assistant panel", Qt::RightDockWidgetArea);
	}
	
	retranslate();
}

AssistantPerspective::~AssistantPerspective()
{
	
}

QIcon AssistantPerspective::icon() const
{
	return QIcon(":/assistant.png");
}

QString AssistantPerspective::name() const
{
	return "Qt 4 Assistant";
}

void AssistantPerspective::retranslate()
{
	if ( m_integration )
	{
	#if QT_VERSION >= 0x040400
	
	#else
	assistant->setTabText( assistant->indexOf(pContents), tr("&Contents"));
	assistant->setTabText( assistant->indexOf(pIndex), tr("&Index"));
	assistant->setTabText( assistant->indexOf(pSearch), tr("&Search"));
	
	lPrepare->setText(tr("Preparing..."));
	lProfile->setText(tr("Select profile :"));
	lIndexSearch->setText(tr("Text to find :"));
	lFullSearch->setText(tr("Text to find :"));
	
	bFullSearch->setText(tr("&Search"));
	#endif
	
	//pDockBrowser->setWindowTitle(tr("Assistant Browser"));
	pDockAssistant->setWindowTitle(tr("Qt Assistant"));
	}
	
	qmdiPerspective::retranslate();
}

QStringList AssistantPerspective::filters() const
{
	QStringList l;
	
	l
		<< tr("C++ source files ( *.cpp *.c *.cxx *.cc )")
		<< tr("C++ header files ( *.h *.hpp *.hxx )")
		<< tr("Qt resource files ( *.qrc )")
		<< tr("Qt Linguist translations ( *.ts )")
		;
	
	return l;
}

qmdiPerspective::Affinity AssistantPerspective::affinity(qmdiClient *c) const
{
	static QStringList ext = QStringList()
		<< "cpp" << "c" << "cxx" << "cc"
		<< "hpp" << "h" << "hxx"
		<< "qrc"
		<< "ts";
	
	if ( dynamic_cast<QRCEdit*>(c) )
		return qmdiPerspective::Exclusive;
	
	if ( ext.contains(QFileInfo(c->fileName()).suffix()) )
		return qmdiPerspective::High;
	
	// we're a "gather them all" perspective...
	return qmdiPerspective::Low;
}

qmdiClient* AssistantPerspective::open(const QString& filename)
{
	if ( filename.endsWith(".qrc") )
	{
		QRCEdit *e = new QRCEdit(filename);
		e->setPerspective(this);
		
		return e;
	} else if ( filename.endsWith(".ts") ) {
		QProcess::startDetached("linguist", QStringList(filename));
		
		return (qmdiClient*)-1;
	}
	
	return 0;
}

bool AssistantPerspective::canOpen(const QString& filename) const
{
	return filename.endsWith(".qrc") || filename.endsWith(".ts");
}

qmdiClient* AssistantPerspective::createEmptyClient(qmdiClientFactory *factory)
{
	return qmdiPerspective::createEmptyClient(factory);
}

void AssistantPerspective::setMainWindow(qmdiMainWindow *w)
{
	if ( !w )
		return;
	
	#if QT_VERSION < 0x040400
	if ( m_integration )
	{
		pAssistant->setMainWindow(w);
		
		QTimer::singleShot(1000, this, SLOT( setDefaultProfile() ));
	}
	#endif
	
	qmdiPerspective::setMainWindow(w);
}

bool AssistantPerspective::eventFilter(QObject *o, QEvent *e)
{
	#if QT_VERSION < 0x040400
	if ( m_integration )
	{
		if ( (e->type() == QEvent::Show) || (e->type() == QEvent::FocusIn) )
		{
			if ( (o == pContents) && !initialized[0] )
			{
				initialized[0] = true;
				pAssistant->setupContent(pContents);
			} else if ( (o == pIndex) && !initialized[1] ) {
				initialized[1] = true;
				pAssistant->setupIndex(pViewIndex);
			} else if ( (o == pSearch) && !initialized[2] ) {
				initialized[2] = true;
				pAssistant->setupSearch();
			}
		}
	}
	#endif
	
	return qmdiPerspective::eventFilter(o, e);
}

void AssistantPerspective::setDefaultProfile()
{
	#if QT_VERSION < 0x040400
	if ( !m_integration || !pProfile->count() )
		return;
	
	setProfile(pProfile->currentText());
	#endif
}

void AssistantPerspective::setProfile(const QString& p)
{
	#if QT_VERSION < 0x040400
	if ( !m_integration || !pAssistant || !p.count() )
		return;
	
	pProfile->setEnabled(false);
	
	pAssistant->setProfile(p);
	
	initialized[0] = initialized[1] = initialized[2] = false;
	
	if ( pContents->isVisible() )
	{
		pContents->clear();
		initialized[0] = pAssistant->setupContent(pContents);
	} else if ( pIndex->isVisible() ) {
		initialized[1] = pAssistant->setupIndex(pViewIndex);
	} else if ( pSearch->isVisible() ) {
		initialized[2] = pAssistant->setupSearch();
	}
	
	pProfile->setEnabled(true);
	#endif
}

void AssistantPerspective::fullSearch()
{
	#if QT_VERSION < 0x040400
	if ( !m_integration || !pAssistant )
		return;
	
	pViewSearch->clear();
	
	foreach ( QString d, pAssistant->search( leFullSearch->text() ) )
	{
		QListWidgetItem *i = new QListWidgetItem( pAssistant->title(d) );
		i->setData(Assistant::LinkRole, d);
		
		pViewSearch->addItem(i);
	}
	#else
	if ( m_integration )
	{
		QList<QHelpSearchQuery> query = m_engine->searchEngine()->queryWidget()->query();
		m_engine->searchEngine()->search(query);
	}
	#endif
}

#if QT_VERSION >= 0x040400
bool AssistantPerspective::contextMenu(const QPoint& pos, bool& newtab)
{
	QMenu m;
	QAction *aOpenInCur = m.addAction(tr("&Open"));
	QAction *aOpenInNew = m.addAction(tr("Open in &new tab"));
	
	QAction *a = m.exec(pos);
	
	newtab = a == aOpenInNew;
	
	return a;
}

void AssistantPerspective::openLink(const QUrl& url, bool newtab)
{
	AssistantClient *a;
	
	a = qobject_cast<AssistantClient*>(mainWindow()->activeWindow());
	
	if ( !a || newtab )
	{
		a = new AssistantClient(m_engine);
		mainWindow()->addWidget(a);
	}
	
	a->openLink(url.toString());
}
#endif

void AssistantPerspective::contentContextMenu(const QPoint& p)
{
	#if QT_VERSION >= 0x040400
	if ( !m_integration )
		return;
	
	QUrl url;
	QModelIndex idx = m_engine->contentWidget()->indexAt(p);
	
	if ( !idx.isValid() )
		return;
	
	QHelpContentItem *i = m_engine->contentModel()->contentItemAt(idx);
	
	if ( i )
		url = i->url();
	
	if ( !url.isValid() )
		return;
	
	bool inNew = false;
	
	if ( contextMenu(m_engine->contentWidget()->mapToGlobal(p), inNew) )
		openLink(url, inNew);
	#endif
}

void AssistantPerspective::indexContextMenu(const QPoint& p)
{
	#if QT_VERSION >= 0x040400
	/*
	QUrl url;
	QModelIndex idx = m_engine->contentWidget()->indexAt(p);
	QHelpContentItem *i = m_engine->contentModel()->contentItemAt(idx);
	
	if ( i )
		url = i->url();
	
	if ( !url.isValid() )
		return;
	
	bool inNew = false;
	contextMenu(p, inNew);
	
	openLink(url, inNew);
	*/
	#endif
}

void AssistantPerspective::searchContextMenu(const QPoint& p)
{
	if ( !m_integration )
		return;
	
	#if QT_VERSION < 0x040400
	QListWidgetItem *i = pViewSearch->itemAt(p);
	
	if ( !i )
		return;
	
	pAssistant->contextMenu(p, pViewSearch, i->data(Assistant::LinkRole).toString());
	#else
	QUrl url = m_engine->searchEngine()->resultWidget()->linkAt(p);
	
	if ( !url.isValid() )
		return;
	
	bool inNew = false;
	
	if ( contextMenu(m_engine->searchEngine()->resultWidget()->mapToGlobal(p), inNew) )
		openLink(url, inNew);
	#endif
}

void AssistantPerspective::itemDoubleClicked(QListWidgetItem *i)
{
	if ( !m_integration )
		return;
	
	#if QT_VERSION < 0x040400
	if ( !i )
		return;
	
	QString link = i->data(Assistant::LinkRole).toString();
	
	if ( link.isEmpty() )
		return;
	
	pAssistant->openLinkToFile(link);
	#endif
}

void AssistantPerspective::indexSearch()
{
	if ( !m_integration )
		return;
	
	#if QT_VERSION < 0x040400
	if ( pAssistant )
		pAssistant->searchIndex( leIndexSearch->text() );
	#else
	QString filter = leIndexSearch->text();
	
	if ( filter.contains(QLatin1Char('*')) )
		m_engine->indexWidget()->filterIndices(filter, filter);
	else
		m_engine->indexWidget()->filterIndices(filter, QString());
	#endif
}

void AssistantPerspective::linkActivated(const QUrl& url)
{
	#if QT_VERSION >= 0x040400
	openLink(url, false);
	#endif
}

void AssistantPerspective::linksActivated(const QMap<QString, QUrl>& urls)
{
	#if QT_VERSION >= 0x040400
	bool ok = false;
	QStringList lbls = urls.keys();
	QString s = QInputDialog::getItem(
								0,
								tr("Multiple links"),
								tr("Choose the most appropriate : "),
								lbls,
								0,
								false,
								&ok
							);
	
	if ( ok && s.count() )
		openLink(urls.values().at(lbls.indexOf(s)), false);
	
	#endif
}
