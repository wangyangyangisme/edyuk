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

#include "qmdiworkspace.h"

#include "qmdihost.h"
#include "qmdiclient.h"
#include "qmdiperspective.h"
#include "qmdimainwindow.h"

#include <QUrl>
#include <QMenu>
#include <QTabBar>
#include <QString>
#include <QMdiArea>
#include <QDropEvent>
#include <QMouseEvent>
#include <QToolButton>
#include <QMainWindow>
#include <QGridLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QMouseEvent>
#include <QApplication>
#include <QMdiSubWindow>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QContextMenuEvent>

/*!
	\class qmdiWorkspace
	\brief A specialized mdi server
	
	qmdiWorkspace manage a set of clients (which MUST be widgets) and allow the user
	to travel among them using both tabs and QWorkspace-like interface
	
	qmdiWorkspace is drop aware : dragging files from any other application (e.g. a
	file browser) will result in them being opened (if the URLs are valid, refers to
	local files and points to files supported by the application...)
	
	qmdiWorkspace provides DnD facilities for placing the tabs in any order you want.
*/

void fixExtension(QString& name, const QString& filter)
{
	if ( name.contains('.') )
		return;
	
	QString ext = filter.section(" *", 1, 1);
	
	if ( ext.endsWith(')') )
		ext.chop(1);
	
	name += ext;
}

/*!
	\brief ctor
*/
qmdiWorkspace::qmdiWorkspace(QMainWindow *p, qmdiHost *host)
 :	QWidget(p),
	qmdiServer( host ? host : dynamic_cast<qmdiHost*>(p) ),
	m_main(p), m_locked(false), m_maximizeOnShow(false),
	m_mightDrag(false), m_dragging(false)
{
	setAcceptDrops(true);
	
	m_menu = new QMenu(this);
	
	m_new = new QToolButton(this);
	m_new->setIcon(QIcon(":/addtab.png"));
	
	m_close = new QToolButton(this);
	m_close->setIcon(QIcon(":/closetab.png"));
	
	m_tab = new QTabBar(this);
	m_tab->setAcceptDrops(true);
	m_tab->installEventFilter(this);
	
	//pWS = new QWorkspace(this);
	//pWS->setScrollBarsEnabled(true);
	
	m_area = new QMdiArea(this);
	m_area->installEventFilter(this);
	
	QGridLayout *g = new QGridLayout(this);
	g->setMargin(0);
	g->setSpacing(0);
	
	g->addWidget(m_new,		0, 0, 1, 1);
	g->addWidget(m_tab,		0, 1, 1, 62);
	g->addWidget(m_close,	0, 63, 1, 1);
	
	g->addWidget(m_area,	1, 0, 47, 64);
	
	setLayout(g);
	
	connect(m_tab	, SIGNAL( currentChanged(int) ),
			this	, SLOT  ( currentChanged(int) ) );
	
	connect(m_area	, SIGNAL( subWindowActivated(QMdiSubWindow*) ),
			this	, SLOT  ( subWindowActivated(QMdiSubWindow*) ) );
	
	connect(m_new	, SIGNAL( clicked() ),
			this	, SLOT  ( fileNew() ) );
	
	connect(m_close	, SIGNAL( clicked() ),
			m_area	, SLOT  ( closeActiveSubWindow() ) );
	
	show();
}

/*!
	\brief dtor
	
	Remaining clients are closed WITHOUT any attempt to save their
	content...
*/
qmdiWorkspace::~qmdiWorkspace()
{
	closeAll(true);
}

/*!
	\brief Ask the host to create a new empty client and add it
*/
void qmdiWorkspace::fileNew()
{
	if ( !host() )
		return;
	
	addClient( host()->createEmptyClient() );
}

/*!
	\brief Dynamic translation handler
*/
void qmdiWorkspace::retranslate()
{
	QList<QWidget*> l = windowList();
	
	foreach ( QWidget *w, l )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( !c )
			continue;
		
		c->retranslate();
	}
}

/*!
	\brief Ask the user whether he wants to commit changes to a client
	\return false if the user canceled the action
	\param c client concerned
	
	\note This function automatically call save() on the given client if
	the user requested to.
*/
bool qmdiWorkspace::maybeSave(qmdiClient *c)
{
	int ret = QMessageBox::warning(this,
								tr("Edyuk"),
								tr("Save changes to file %1 ?").arg(c->name()),
								QMessageBox::Yes | QMessageBox::Default,
								QMessageBox::No,
								QMessageBox::Cancel | QMessageBox::Escape);
	
	switch ( ret )
	{
		case QMessageBox::Yes :
			c->save();
			
		case QMessageBox::No :
			break;
			
		default:
			return false;
	}
	
	return true;
}

/*!
	\brief Save all opened clients
*/
void qmdiWorkspace::saveAll()
{
	QWidgetList l = windowList();
	
	foreach ( QWidget *w, l )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( !c || !w )
			continue;
		
		c->save();
	}
}

/*!
	\brief Save the current client
*/
void qmdiWorkspace::saveCurrent()
{
	qmdiClient *c = dynamic_cast<qmdiClient*>((QWidget*)m_current);
	
	if ( c )
		c->save();
	
}

/*!
	\brief Save the current client under a new name
	
	\see saveAs(qmdiClient*)
*/
void qmdiWorkspace::saveCurrentAs()
{
	qmdiClient *c = dynamic_cast<qmdiClient*>((QWidget*)m_current);
	
	saveClientAs(c);
}

/*!
	\brief Save a client under a new name
	\param c Client to save
*/
void qmdiWorkspace::saveClientAs(qmdiClient *c)
{
	if ( !c )
		return;
	
	QString filters = host() ? host()->filters() : tr("All files ( * )");
	
	QString x, name = QFileDialog::getSaveFileName(dynamic_cast<QWidget*>(c),
									"Save file as...",
									c->fileName(), //QFileInfo(c->fileName()).filePath(),
									filters,
									&x);
	
	if ( name.isEmpty() )
		return;
	
	if ( QFileInfo(name).completeSuffix().isEmpty() )
		fixExtension(name, x);
	
	c->setFileName(name);
	c->save();
}

/*!
	\brief Pretend a close event
	\return Whether the (pretended) close event has been canceled
	
	The user is asked, through the qmdiServer::maybeSave() function,
	to commit unsaved changes to widgets. An option is offered to interrupt
	closure
*/
bool qmdiWorkspace::checkModified()
{
	foreach ( QWidget *w, windowList() )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( !c )
			continue;
		
		if ( c->isContentModified() )
			if ( !maybeSave(c) )
				return true;
	}
	
	return false;
}

/*!
	\brief Attempt to close all opened client
	\return True on success (this includes the absence of clients to close)
	\param bypassMod Whther modification state shall be bypassed
*/
bool qmdiWorkspace::closeAll(bool bypassMod)
{
	foreach ( QWidget *w, windowList() )
	{
		if ( !w )
			continue;
		
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( c )
		{
			if ( bypassMod )
				c->setContentModified(false);
		}
		
		if ( w->close() )
			emit widgetClosed(w);
	}
	
	return true;
}

/*!
	\brief Attempt to close the current client
	\return True on success (this includes the absence of client to close)
	\param bypassMod Whther modification state shall be bypassed
*/
bool qmdiWorkspace::closeCurrent(bool bypassMod)
{
	QWidget *w = m_current; //pWS->activeWindow();
	
	if ( w )
	{
		if ( bypassMod )
			dynamic_cast<qmdiClient*>(w)->setContentModified(false);
		
		if ( w->close() )
		{
			emit widgetClosed(w);
			return true;
		} else {
			return false;
		}
	}
	
	return true;
}

/*!
	\brief Attempt to print the current client (if printable...)
*/
void qmdiWorkspace::printCurrent()
{
	qmdiClient *c = dynamic_cast<qmdiClient*>((QWidget*)m_current);
	
	if ( c )
		c->print();
	
}

/*!
	\brief Display widgets in tile
*/
void qmdiWorkspace::tile()
{
	m_area->tileSubWindows();
}

/*!
	\brief Display widgets in cascade
*/
void qmdiWorkspace::cascade()
{
	m_area->cascadeSubWindows();
}

/*!
	\brief Adds a client
*/
void qmdiWorkspace::addClient(qmdiClient *c)
{
	if ( !c )
		return;
	
	addWidget( dynamic_cast<QWidget*>(c) );
}

/*!
	\brief Adds a widget
*/
void qmdiWorkspace::addWidget(QWidget *e)
{
	if ( !e )
		return;
	
	qmdiClient *c = dynamic_cast<qmdiClient*>(e);
	
	if ( !c )
		return;
	
	c->setServer(this);
	
	if ( host() )
	{
		host()->clientOpened(c);
	} else {
		qDebug("no host...");
	}
	
	//e->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	
	QString s = e->windowTitle();
	s.remove("[*]");
	
	m_tab->addTab(s);
	m_widgets << e;
	m_wrappers << m_area->addSubWindow(e);
	
	if ( windowList().count() <= 1 )
	{
		if ( isVisible() )
			e->showMaximized();
		else
			m_maximizeOnShow = true;
	} else {
		e->show();
	}
	
	setActiveWindow(e, true);
	
	m_wrappers.last()->setAttribute(Qt::WA_DeleteOnClose, true);
	
	emit widgetAdded(e);
	emit widgetsOpened(true);
}

/*!
	\return The current file (filename of the active window)
*/
QString qmdiWorkspace::file() const
{
	qmdiClient *c = dynamic_cast<qmdiClient*>((QWidget*)m_current);
	
	if ( c )
		return c->fileName();
	
	return QString();
}

/*!
	\return The active window
*/
QWidget* qmdiWorkspace::activeWindow() const
{
	return m_current;
}

/*!
	\brief Sets the active window
*/
void qmdiWorkspace::setActiveWindow(QWidget *w, bool force)
{
	int idx = m_widgets.indexOf(w);
	
	if ( (idx < 0) || (!force && (w == m_current)) )
		return;
	
	m_locked = true;
	
	//qDebug("changing active window...");
	
	if ( !w->parent() )
	{
		//qDebug("raising undocked...");
		w->show();
		w->raise();
	} else {
		if ( m_wrappers.at(idx) )
			m_area->setActiveSubWindow(m_wrappers.at(idx));
		
		m_tab->setCurrentIndex(idx);
	}
	
	mergeMDI(w);
	
	m_locked = false;
}

/*!
	\return A list of owned files (clients filenames)
*/
QStringList qmdiWorkspace::files() const
{
	QStringList l;
	
	foreach ( QWidget *w, windowList() )
	{
		qmdiClient *c = dynamic_cast<qmdiClient*>(w);
		
		if ( !c || c->fileName().isEmpty() )
			continue;
		
		l << c->fileName();
	}
	
	return l;
}

/*!
	\return A list of owned widgets
*/
QWidgetList qmdiWorkspace::windowList() const
{
	return m_widgets; //pWS->windowList();
}

/*!
	\internal
*/
void qmdiWorkspace::currentChanged(int i)
{
	if ( m_locked || (i < 0) || (i >= m_widgets.count()) )
		return;
	
	QWidget *w = m_widgets.at(i);
	
	//qDebug("activating tab...");
	
	if ( w != m_current )
		setActiveWindow(w);
}

/*!
	\internal
*/
void qmdiWorkspace::subWindowActivated(QMdiSubWindow *sw)
{
	static bool _auto_lock = false;
	
	if ( !sw )
	{
		_auto_lock = true;
		return;
	} else if ( _auto_lock ) {
		_auto_lock = false;
		setActiveWindow(m_current, true);
		return;
	}
	
	int idx = m_wrappers.indexOf(sw);
	
	if ( m_locked || (idx < 0) || (idx == m_tab->currentIndex()) )
		return;
	
	//qDebug("activating subwindow...");
	
	QWidget *w = m_widgets.at(idx);
	
	if ( w != m_current )
		setActiveWindow(w);
}

/*!
	\internal
*/
void qmdiWorkspace::mergeMDI(QWidget *w)
{
	if ( !host() )
	{
		return;
		qFatal("qmdiWorkspace : no host accessible, aborting merging...");
	}
	
	qmdiClient	*oc = dynamic_cast<qmdiClient*>((QWidget*)m_current),
				*nc = dynamic_cast<qmdiClient*>(w);
	
	if ( nc == oc )
		return;
	
	if ( m_current )
	{
		host()->unmergeClient(oc);
		
		disconnect(	m_current	, SIGNAL( contentModified(bool) ),
					this		, SLOT  ( emitCurrentModified(bool) ) );
		
		disconnect(	m_current	, SIGNAL( titleChanged(const QString&) ),
					this		, SLOT  ( currentTitleChanged(const QString&) ) );
		
		if ( !m_current->parent() )
			m_current->hide();
		
	}
	
	m_current = w;
	
	emit indexChanged(w);
	emit indexChanged(m_widgets.indexOf(w));
	
	if ( m_current )
	{
		if ( !nc )
			qFatal("Non client widget...");
		
		host()->mergeClient(nc);
		m_current->setFocus();
		
		emitCurrentModified( nc->isContentModified() );
		emit currentPrintable( nc->isPrintable() );
		
		connect(m_current	, SIGNAL( contentModified(bool) ),
				this		, SLOT  ( emitCurrentModified(bool) ) );
		
		connect(m_current	, SIGNAL( titleChanged(QString) ),
				this		, SLOT  ( currentTitleChanged(QString) ) );
		
		if ( nc->perspective() )
		{
			//qDebug("client has a perspective...");
			nc->perspective()->show();
		} else {
			host()->currentClientChanged(nc);
		}
	} else {
		emitCurrentModified(false);
		emit currentPrintable(false);
	}
	
	host()->updateGUI(m_main);
}

/*!
	\internal
*/
void qmdiWorkspace::clientDeleted(QObject *o)
{
	#ifdef _DEV_DEBUG_
	qDebug("qmdiWorkspace : client deleted %p", o);
	#endif
	
	QWidget *w = qobject_cast<QWidget*>(o);
	
	if ( !w )
		return;
	
	int i = m_widgets.indexOf(w);
	
	if ( i == -1 )
		return;
	
	o->deleteLater();
	
	m_widgets.removeAt(i);
	m_wrappers.removeAt(i);
	
	m_tab->removeTab(i);
	
	QMdiSubWindow *sw = m_area->activeSubWindow();
	
	if ( sw && (m_wrappers.indexOf(sw) != -1) )
		m_current = m_widgets.at(m_wrappers.indexOf(sw));
	else
		m_current = 0;
	
	if ( !host() )
	{
		return;
		qFatal("qmdiWorkspace : no host accessible, aborting deletion...");
	}
	
	qmdiClient *c = dynamic_cast<qmdiClient*>(w);
	
	if ( c )
	{
		host()->clientClosed(c);
		host()->unmergeClient(c);
	}
	
	if ( m_current )
	{
		c = dynamic_cast<qmdiClient*>((QWidget*)m_current);
		
		host()->mergeClient(c);
		m_current->setFocus();
		
		emitCurrentModified(c->isContentModified());
		emit currentPrintable(c->isPrintable());
		
		connect(m_current	, SIGNAL( contentModified(bool) ),
				this		, SLOT  ( emitCurrentModified(bool) ) );
		
		connect(m_current	, SIGNAL( titleChanged(const QString&) ),
				this		, SLOT  ( currentTitleChanged(const QString&) ) );
		
	} else {
		emit widgetsOpened(false);
		emitCurrentModified(false);
		emit currentPrintable(false);
	}
	
	host()->updateGUI(m_main);
}

/*!
	\internal
*/
void qmdiWorkspace::currentTitleChanged(const QString& t)
{
	m_tab->setTabText(m_tab->currentIndex(), t);
}

/*!
	\internal
*/
void qmdiWorkspace::emitCurrentModified(bool m)
{
	emit currentModified(m);
	
	int idx = m_tab->currentIndex();
	QString t = m_tab->tabText(idx);
	
	bool end = t.endsWith("*");
	
	if ( m && !end )
		t += "*";
	else if ( !m && end )
		t.chop(1);
	else
		return;
	
	m_tab->setTabText(idx, t);
}

/*!
	\internal
*/
bool qmdiWorkspace::eventFilter(QObject *o, QEvent *e)
{
	if ( e && o )
	{
		if ( o == m_area )
		{
			/*switch ( e->type() )
			{
				case QEvent::WindowActivate :
				case QEvent::WindowDeactivate :
				{
					return true;
				}
					
				default:
					break;
			}
			*/
		} else if ( o == m_tab ) {
			switch ( e->type() )
			{
				case QEvent::MouseButtonDblClick :
				{
					// make sure the tab is maximized and docked...
					if ( m_current )
					{
						if ( !m_current->parent() )
						{
							m_wrappers[m_tab->currentIndex()] = m_area->addSubWindow(m_current);
						}
						
						m_current->showMaximized();
					}
					
					break;
				}
					
				case QEvent::MouseButtonPress :
				{
					QMouseEvent *me = static_cast<QMouseEvent*>(e);
					
					if ( me->button() != Qt::LeftButton )
					{
						m_mightDrag = false;
						m_dragPos = QPoint();
						
						if ( me->button() == Qt::MidButton )
							return true;
						
					} else {
						m_mightDrag = true;
						m_dragPos = me->pos();
					}
					
					//e->accept();
					
					// filter out...
					//return true;
					break;
				}
					
				case QEvent::MouseButtonRelease :
				{
					QMouseEvent *me = static_cast<QMouseEvent*>(e);
					
					if ( me->button() == Qt::MidButton )
					{
						// undocking...
						int i = tabAt(me->pos());
						
						//qDebug("undocking tab %i", i);
						
						if ( (i >= 0) && (i < m_widgets.count()) )
						{
							QWidget *w = m_widgets.at(i);
							
							if ( w->parent() )
							{
								// attempt not to alter the workspace layout
								setActiveWindow(w);
								w->showNormal();
								m_wrappers[i] = 0;
								m_area->removeSubWindow(w);
								w->setWindowFlags(w->windowFlags() | Qt::WindowStaysOnTopHint);
							} else {
								m_area->addSubWindow(w);
								w->setWindowFlags(w->windowFlags() & ~Qt::WindowStaysOnTopHint);
							}
							
							setActiveWindow(w);
						}
						
						return true;
					}
					
					m_mightDrag = false;
					m_dragPos = QPoint();
					
					//e->accept();
					
					// filter out...
					//return true;
					
					break;
				}
					
				case QEvent::MouseMove :
				{
					QMouseEvent *me = static_cast<QMouseEvent*>(e);
					
					if (
							(me->buttons() & Qt::LeftButton)
						&&
							(
								(m_dragPos - me->pos()).manhattanLength()
							>=
								QApplication::startDragDistance()
							)
						)
					{
						int tab = tabAt(m_dragPos);
						
						if ( tab == -1 )
							break;
						
						qmdiClient *c = dynamic_cast<qmdiClient*>(m_widgets.at(tab));
						
						if ( !c )
							break;
						
						QDrag *drag = new QDrag(this);
						QMimeData *data = new QMimeData;
						
						QList<QUrl> urls;
						
						urls << QUrl::fromLocalFile(c->fileName());
						
						data->setUrls(urls);
						
						data->setData(	"internal/x-tab",
										QByteArray::number(tab)
									);
						
						data->setData(
										"internal/x-tabbar",
						#if QT_POINTER_SIZE == 4
										QByteArray::number(reinterpret_cast<quint32>(m_tab))
						#elif QT_POINTER_SIZE == 8
										QByteArray::number(reinterpret_cast<quint64>(m_tab))
						#endif
									);
						
						drag->setMimeData(data);
						
						drag->start();
						
						e->accept();
						
						// filter out...
						return true;
					}
					
					break;
				}
					
				case QEvent::DragEnter :
				{
					QDragEnterEvent *de = static_cast<QDragEnterEvent*>(e);
					
					if (
							de->mimeData()
						&&
							(
								de->mimeData()->hasUrls()
							||
								(
									de->mimeData()->hasFormat("internal/x-tab")
								&&
									de->mimeData()->hasFormat("internal/x-tabbar")
								)
							)
						)
					{
						de->acceptProposedAction();
						
						// filter out...
						return true;
					}
					
					break;
				}
					
					
				case QEvent::DragMove :
				{
					QDragMoveEvent *de = static_cast<QDragMoveEvent*>(e);
					
					de->acceptProposedAction();
					
					// filter out...
					return true;
				}
					
					
				case QEvent::Drop :
				{
					QDropEvent *de = static_cast<QDropEvent*>(e);
					
					if (
							de->mimeData()->hasFormat("internal/x-tab")
						&&
							de->mimeData()->hasFormat("internal/x-tabbar")
						&&
							(
							#if QT_POINTER_SIZE == 4
								reinterpret_cast<QTabBar*>(
									de->mimeData()->data("internal/x-tabbar").toUInt()
								)
							#elif QT_POINTER_SIZE == 8
								reinterpret_cast<QTabBar*>(
									de->mimeData()->data("internal/x-tabbar").toULongLong()
								)
							#endif
							==
								m_tab
							)
						)
					{
						int tab = de->mimeData()->data("internal/x-tab").toInt(),
							newTab = tabAt(de->pos());
						
						if ( tab == newTab )
							break;
						
						// TODO : check for right or left tab bound...
						
						// 1) backup tab content
						// 2) remove tab
						// 3) insert new one...
						QVariant tabData = m_tab->tabData(tab);
						
						QIcon tabIcon = m_tab->tabIcon(tab);
						
						QString tabText = m_tab->tabText(tab),
								tabToolTip = m_tab->tabToolTip(tab),
								tabWhatsThis = m_tab->tabWhatsThis(tab);
						
						QColor tabTextColor = m_tab->tabTextColor(tab);
						
						m_tab->removeTab(tab);
						QWidget *tabWidget = m_widgets.takeAt(tab);
						
						if ( newTab == -1 )
						{
							m_widgets.append(tabWidget);
							newTab = m_tab->addTab(tabIcon, tabText);
						} else if ( tab < newTab ) {
							--newTab;
							
							m_widgets.insert(newTab, tabWidget);
							m_tab->insertTab(newTab, tabIcon, tabText);
						} else {
							m_widgets.insert(newTab, tabWidget);
							m_tab->insertTab(newTab, tabIcon, tabText);
						}
						
						/*
						tabs.insert(newTab, tabs.at(tab));
						m_tab->insertTab(newTab, tabIcon, tabText);
						*/
						
						m_tab->setTabData(newTab, tabData);
						m_tab->setTabToolTip(newTab, tabToolTip);
						m_tab->setTabWhatsThis(newTab, tabWhatsThis);
						m_tab->setTabTextColor(newTab, tabTextColor);
						
						/*
						if ( (tab < newTab) || (newTab < 0) )
						{
							tabs.removeAt(tab);
							m_tab->removeTab(tab);
						} else {
							tabs.removeAt(tab + 1);
							m_tab->removeTab(tab + 1);
						}
						*/
						
						m_tab->setCurrentIndex(newTab);
						
					} else if ( de->mimeData()->hasUrls() ) {
						
						dropEvent(de);
						
						/*
						foreach ( QUrl url, de->mimeData()->urls() )
							;
						*/
					}
					
					de->acceptProposedAction();
					
					// filter out...
					return true;
				}
					
					
				default:
					break;
			}
		}
	}
	
	return QWidget::eventFilter(o, e);
}

int qmdiWorkspace::tabAt(const QPoint& p)
{
	for ( int i = 0; i < m_tab->count(); ++i )
	{
		QRect rect = m_tab->tabRect(i);
		
		if ( rect.contains(p) )
			return i;
	}
	
	return -1;
}

/*!
	\internal
*/
void qmdiWorkspace::dragEnterEvent(QDragEnterEvent *e)
{
	if ( e->mimeData() && e->mimeData()->hasUrls() )
	{
		e->acceptProposedAction();
	}
}

/*!
	\internal
*/
void qmdiWorkspace::dragMoveEvent(QDragMoveEvent *e)
{
	return QWidget::dragMoveEvent(e);
}

/*!
	\internal
*/
void qmdiWorkspace::dropEvent(QDropEvent *e)
{
	qmdiMainWindow *mw = dynamic_cast<qmdiMainWindow*>(host());
	
	if ( mw && e->mimeData() && e->mimeData()->hasUrls() )
	{
		// managed openning 
		
		foreach ( QUrl url, e->mimeData()->urls() )
			mw->fileOpen(url.toLocalFile());
		
	} else {
		// direct drop : what to open???
		qWarning("Dropping to unmanaged workspace unsupported for now...");
	}
	
	e->acceptProposedAction();
}

/*!
	\internal
*/
void qmdiWorkspace::showEvent(QShowEvent *e)
{
	QWidget::showEvent(e);
}

/*!
	\internal
*/
void qmdiWorkspace::hideEvent(QHideEvent *e)
{
	QWidget::hideEvent(e);
}

/*!
	\internal
*/
void qmdiWorkspace::contextMenuEvent(QContextMenuEvent *e)
{
	e->accept();
	
	m_menu->exec(e->globalPos());
}
