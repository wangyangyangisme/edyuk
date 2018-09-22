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

#include <qglobal.h>

#if QT_VERSION < 0x040400

#include "assistant.h"

#include "index.h"
#include "assistantclient.h"

#include "qmdimainwindow.h"

#include <QUrl>
#include <QDir>
#include <QMenu>
#include <QStack>
#include <QTimer>
#include <QAction>
#include <QFileInfo>
#include <QListView>
#include <QTreeWidget>
#include <QMessageBox>
#include <QInputDialog>
#include <QLibraryInfo>
#include <QApplication>

//#include "qunicodetables_p.h"


/*
	code borrowed from : 
	Qt Assistant, bundled with Qt/X11 4.1.3, OpenSource Edition,
	
	copyright notice :
	
** Copyright (C) 1992-2006 Trolltech AS. All rights reserved.
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
	
	note : according to this notice, some modifications have been done and the
	resulting code, as a derivative work is licensed under GPL 2.0
	
*/
bool caseInsensitiveLessThan(const QString &as, const QString &bs)
{
	const QChar *a = as.unicode();
	const QChar *b = bs.unicode();
	
	if (a == 0)
		return true;
	
	if (b == 0)
		return false;
	
	if ( a == b )
		return false;
	
	int l = qMin(as.length(), bs.length());
	
	while ( l-- && a->toLower() == b->toLower() )
        a++,b++;
	
	if ( l == -1 )
		return (as.length() < bs.length());
	
	return a->toLower() < b->toLower();
}

/**
 * \a real is kinda a hack for the smart search, need a way to match a regexp
 * to an item...
 * How would you say the best match for Q.*Wiget is QWidget?
 */
QModelIndex AssistantIndexModel::filter(const QString &s, const QString &real)
{
    QStringList list;

    int goodMatch = -1;
    int perfectMatch = -1;
    if (s.isEmpty())
        perfectMatch = 0;

    const QRegExp regExp(s);
    QMultiMap<QString, QString>::iterator it = link.begin();
    QString lastKey;
    for (; it != link.end(); ++it) {
        if (it.key() == lastKey)
            continue;
        lastKey = it.key();
        const QString key = it.key();
        if (key.contains(regExp) || key.contains(s, Qt::CaseInsensitive))
        {
            list.append(key);
            
            if (perfectMatch == -1 && (key.startsWith(real, Qt::CaseInsensitive)))
            {
                if (goodMatch == -1)
                    goodMatch = list.count() - 1;
                if (real.length() == key.length())
                    perfectMatch = list.count() - 1;
            }  else if (perfectMatch > -1 && s == key) {
                perfectMatch = list.count() - 1;
            }
        }
    }

    int bestMatch = perfectMatch;
    if (bestMatch == -1)
        bestMatch = goodMatch;

    bestMatch = qMax(0, bestMatch);
    
    // sort the new list
    QString match;
    if (bestMatch >= 0 && list.count() > bestMatch)
        match = list[bestMatch];
    qSort(list.begin(), list.end(), caseInsensitiveLessThan);
    setStringList(list);
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i) == match){
            bestMatch = i;
            break;
        }
    }
    return index(bestMatch, 0, QModelIndex());
}

/*
	End of code borrowed from Qt
*/

Assistant::Assistant(qmdiMainWindow *w, QObject *p)
 : QObject(p), pIndex(0), pIndexModel(0), pMain(w), bSetup(false), bInSetup(false)
{
	pMenu = new QMenu;
	
	aOpenInCur = new QAction(tr("Open in current tab"), this);
	aOpenInNew = new QAction(tr("Open in new tab"), this);
	
	pMenu->addAction(aOpenInCur);
	pMenu->addAction(aOpenInNew);
	
	setupProfiles();
}

Assistant::~Assistant()
{
	
	
}

QString Assistant::docPath()
{
	return QLibraryInfo::location(QLibraryInfo::DocumentationPath)
						.replace("\\", "/")
						+ "/html/";
}

QStringList Assistant::profiles() const
{
	return profilesTable.keys();
}

void Assistant::setupProfiles()
{
	QString dbPath = databasePath();
	QStringList dbFiles = QDir(dbPath).entryList();
	
	if ( dbFiles.isEmpty() )
		return;
	
	QStringList::iterator db;
	
	QRegExp profile("^(?:index|content)db40(?:\\.dict|\\.doc)?\\.(\\w+)$");
	
	for ( db = dbFiles.begin(); db != dbFiles.end(); db++ )
	{
		if ( !db->contains(profile) )
			continue;
		
		profilesTable[profile.cap(1)] << dbPath + "/" + *db;
	}
}

void Assistant::setProfile(const QString& pro)
{
	if ( pro.isEmpty() || !profilesTable.contains(pro) )
		return;
	
	if ( pIndex )
	{
		delete pIndex;
		pIndex = 0;
	}
	
	if ( pIndexModel )
	{
		delete pIndexModel;
		pIndexModel = 0;
	}
	
	keywords.clear();
	contentList.clear();
	
	m_profile = pro;
	m_base = QString("%1/%3db40%4.%2").arg(databasePath()).arg(pro);
}

bool Assistant::setupContent(QTreeWidget *tw)
{
	//TODO : use a custom model instead ?
	
	/** -- Code added by fmc -- **/
	if ( !tw )
		return false;
	
	QStringList dbFiles = profilesTable[m_profile];
	
	QString cont = m_base.arg("content").arg("");
	
	/*
		Setup content list for content tree
	*/
	if ( !dbFiles.contains(cont) )
	{
		QMessageBox::information(0,
								tr("Assistant"),
								tr(
									"Content database not found.\n"
									"Please run Qt Assistant to generate it.\n"
								)
							);
		
		return false;
	}
	
	QFile f(cont);
	
	if ( f.open(QFile::ReadOnly) )
	{
		QDataStream d(&f);
		
		/*
			Skip age data...
		*/
		quint32 ages;
		d >> ages;
		
		while ( !d.atEnd() )
		{
			QString key;
			QList<ContentItem> items;
			
			d >> key;
			d >> items;
			
			contentList += qMakePair(key, QList<ContentItem>(items));
		}
		
		f.close();
	} else {
		return false;
	}
	
	pContentTree = tw;
	
	connect(pContentTree, SIGNAL( itemClicked(QTreeWidgetItem*, int) ),
			this		, SLOT  ( openLinkToItem(QTreeWidgetItem*) ) );
	
	connect(pContentTree, SIGNAL( customContextMenuRequested(const QPoint&) ),
			this		, SLOT  ( contextMenu(const QPoint&) ) );
	
	/** -- !Code added by fmc -- **/
	
	QList< QPair<QString, ContentList> >::iterator it;
	
	for( it = contentList.begin(); it != contentList.end(); ++it )
	{
		QTreeWidgetItem *newEntry;
		
		//QApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 100);
		
		QTreeWidgetItem *contentEntry;
		QStack<QTreeWidgetItem*> stack;
		
		int depth = 0;
		bool root = false;
		
		QTreeWidgetItem *lastItem[64];
		
		for(int j = 0; j < 64; ++j)
			lastItem[j] = 0;
		
		ContentList lst = (*it).second;
		
		for (ContentList::ConstIterator it = lst.begin(); it != lst.end(); ++it)
		{
			ContentItem item = *it;
			
			if ( !item.depth )
			{
				newEntry = new QTreeWidgetItem(pContentTree, 0, Chapter);
				newEntry->setText(0, item.title);
				newEntry->setIcon(0, QIcon(":/book.png"));
				newEntry->setData(0, LinkRole, item.reference);
				stack.push(newEntry);
				depth = 1;
				root = true;
			} else {
				if( (item.depth > depth) && root )
				{
					depth = item.depth;
					stack.push(contentEntry);
				}
				
				if( item.depth == depth )
				{
					contentEntry = new QTreeWidgetItem(	stack.top(),
														lastItem[ depth ],
														Section);
					
					lastItem[ depth ] = contentEntry;
					contentEntry->setText(0, item.title);
					contentEntry->setData(0, LinkRole, item.reference);
				} else if( item.depth < depth ) {
					stack.pop();
					depth--;
					item = *(--it);
				}
			}
		}
	}
	
	contentList.clear();
	return true;
}

bool Assistant::setupIndex(QListView *lv)
{
	if ( !lv || pIndexModel )
		return false;
	
	QStringList dbFiles = profilesTable[m_profile];
	
	QString indx = m_base.arg("index").arg("");
	
	/*
		Setup alphabetic index
	*/
	if ( !dbFiles.contains(indx) )
	{
		QMessageBox::information(0,
								tr("Assistant"),
								tr(
									"Index database not found.\n"
									"Please run Qt Assistant to generate it.\n"
								)
							);
		
		return false;
	}
	
	QFile f(indx);
	
	if ( pIndexModel )
		delete pIndexModel;
	
	pIndexModel = new AssistantIndexModel(this);
	
	if ( f.open(QFile::ReadOnly) )
	{
		QDataStream d(&f);
		
		/*
			Skip age data...
		*/
		quint32 ages;
		d >> ages;
		
		d >> keywords;
		
		for (int i = 0; i < keywords.count(); ++i )
		{
			const IndexKeyword &idx = keywords.at(i);
			
			pIndexModel->addLink(idx.keyword, idx.link);
		}
		
		f.close();
	} else {
		return false;
	}
	
	pIndexTarget = lv;
	
	connect(pIndexModel	, SIGNAL( currentIndexChanged(const QModelIndex&) ),
			pIndexTarget, SLOT  ( setCurrentIndex(const QModelIndex&) ) );
	
	connect(pIndexTarget, SIGNAL( doubleClicked(const QModelIndex&) ),
			this		, SLOT  ( openLinkToIndex(const QModelIndex&) ) );
	
	connect(pIndexTarget, SIGNAL( customContextMenuRequested(const QPoint&) ),
			this		, SLOT  ( contextMenu(const QPoint&) ) );
	
	pIndexTarget->setModel(pIndexModel);
	
	pIndexModel->filter(QString(), QString());
	
	return true;
}

bool Assistant::setupSearch()
{
	if ( pIndex )
		return false;
	
	QStringList dbFiles = profilesTable[m_profile];
	
	QString docl = m_base.arg("index").arg(".doc"),
			dict = m_base.arg("index").arg(".dict");
	
	/*
		Setup index for full-text search
	*/
	if ( dbFiles.contains(dict) && dbFiles.contains(docl) )
	{
		pIndex = new Index(QStringList(), QDir::homePath());
		
		pIndex->setDocListFile(docl);
		pIndex->setDictionaryFile(dict);
		
		pIndex->readDict();
	} else {
		QMessageBox::information(0,
								tr("Assistant"),
								tr(
									"Search databases not found.\n"
									"Please run Qt Assistant to generate them.\n"
								)
							);
		
		return false;
	}
	
	return true;
}

QString Assistant::databasePath()
{
	return QDir::homePath() + QLatin1String("/.assistant");
}

QString Assistant::title(const QString& doc)
{
	return pIndex->getDocumentTitle(doc);
}

AssistantClient* Assistant::createClient()
{
	AssistantClient *a = new AssistantClient(this);
	clients << a;
	
	return a;
}

void Assistant::removeClient(AssistantClient *a)
{
	clients.removeAll(a);
}

void Assistant::openLinkToFile(const QString& f)
{
	if ( f.isEmpty() )
		return;
	
	
	QWidget *w = mainWindow() ? mainWindow()->activeWindow() : 0;
	AssistantClient *a = qobject_cast<AssistantClient*>(w);
	
	if ( !a )
	{
		a = createClient();
		
		if ( mainWindow() )
			mainWindow()->addWidget(a);
	}
	
	/*
		f must be a link somewher in Qt docs moreover AssistantClient(s) have
		searchPaths set to correct Qt docs location finally using stripped file
		names was the only way to get rid of a nasty bug under M$ Window$...
	*/
	a->openLink( QFileInfo(f).fileName() );
	//emit openLink( QFileInfo(f).fileName() );
}

void Assistant::openLinkToItem(QTreeWidgetItem *i)
{
	if ( !i )
		return;
	
	openLinkToFile( QUrl( i->data(0, LinkRole).toString() ).path() );
}

void Assistant::openLinkToIndex(const QModelIndex& i)
{
	if ( !i.isValid() )
		return;
	
	QString lnk;
	QStringList l = pIndexModel->links( i.row() );
	
	if ( l.count() == 1 )
		lnk = l.at(0);
	else
		lnk = QInputDialog::getItem(0, tr("Multiple links found"),
									tr("Select one : "), l);
	
	openLinkToFile(lnk);
}

void Assistant::contextMenu(const QPoint& p)
{
	QObject *c = sender();
	
	if ( !c )
		return;
	
	QAction *a = 0;
	
	QListView *lv = qobject_cast<QListView*>(c);
	QTreeWidget *tw = qobject_cast<QTreeWidget*>(c);
	
	if ( lv )
		a = pMenu->exec( lv->viewport()->mapToGlobal(p) );
	else if ( tw )
		a = pMenu->exec( tw->viewport()->mapToGlobal(p) );
	
	if ( !a )
		return;
	
	if ( a == aOpenInNew && mainWindow() )
		mainWindow()->addWidget(createClient());
	
	if ( tw )
		openLinkToItem( tw->currentItem() );
	else if ( lv )
		openLinkToIndex( lv->currentIndex() );
}

void Assistant::contextMenu(const QPoint& p, QWidget *w, const QString& dest)
{
	if ( !w )
		return;
	
	QAction *a = pMenu->exec(w->mapToGlobal(p));
	
	if ( !a )
		return;
	
	if ( a == aOpenInNew && mainWindow() )
		mainWindow()->addWidget(createClient());
	
	openLinkToFile(dest);
}


/*
	code borrowed from : 
	Qt Assistant, bundled with Qt/X11 4.1.3, OpenSource Edition,
	
	copyright notice :
	
** Copyright (C) 1992-2006 Trolltech AS. All rights reserved.
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License version 2.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of
** this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
** http://www.trolltech.com/products/qt/opensource.html
	
*/
QStringList Assistant::search(QString str)
{
	str = str.simplified();
	str = str.replace(QLatin1String("\'"), QLatin1String("\""));
	str = str.replace(QLatin1String("`"), QLatin1String("\""));
	
	QString buf = str;
	str = str.replace(QLatin1String("-"), QLatin1String(" "));
	str = str.replace(QRegExp(QLatin1String("\\s[\\S]?\\s")), QLatin1String(" "));
	
	QStringList terms, termSeq, seqWords;
	terms = str.split(QLatin1Char(' '));
	
	QStringList::iterator it = terms.begin();
	
	for (; it != terms.end(); ++it)
	{
		(*it) = (*it).simplified();
		(*it) = (*it).toLower();
		(*it) = (*it).replace(QLatin1String("\""), QLatin1String(""));
	}
	
	if (str.contains(QLatin1Char('\"')))
	{
		if ((str.count(QLatin1Char('\"')))%2 == 0)
		{
			QString s;
			int beg = 0;
			int end = 0;
			
			beg = str.indexOf(QLatin1Char('\"'), beg);
			
			while (beg != -1)
			{
				beg++;
				end = str.indexOf(QLatin1Char('\"'), beg);
				s = str.mid(beg, end - beg);
				s = s.toLower();
				s = s.simplified();
				
				if (s.contains(QLatin1Char('*')))
				{
					QMessageBox::warning(0, tr("Full Text Search"),
						tr("Using a wildcard within phrases is not allowed."));
					return QStringList();
				}
				
				seqWords += s.split(QLatin1Char(' '));
				termSeq << s;
				beg = str.indexOf(QLatin1Char('\"'), end + 1);
			}
		} else {
			QMessageBox::warning(0, tr("Full Text Search"),
				tr("The closing quotation mark is missing."));
			return QStringList();
		}
	}
	
	return pIndex ? pIndex->query(terms, termSeq, seqWords) : QStringList();
}

void Assistant::searchIndex(const QString &searchString)
{
	if ( !pIndexModel )
		return;	
	
	QRegExp atoz("[A-Z]");
	int matches = searchString.count(atoz);
	
	if (matches > 0 && !searchString.contains(".*"))
	{
		int start = 0;
		QString newSearch;
		
		for (; matches > 0; --matches)
		{
			int match = searchString.indexOf(atoz, start+1);
			
			if (match <= start)
				continue;
			
			newSearch += searchString.mid(start, match-start);
			newSearch += ".*";
			start = match;
		}
		
		newSearch += searchString.mid(start);
		
		pIndexModel->search(newSearch, searchString);
	} else
		pIndexModel->search(searchString, searchString);
}

/*
	End of code borrowed from Qt
*/
#endif
