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

#include "edyuklogdock.h"

/*!
	\file edyuklogdock.cpp
	\brief Implementation of the EdyukLogDock class.
*/

#include "qwidgetstack.h"
#include "qmdimainwindow.h"

#include "qeditor.h"
#include "qdocumentcursor.h"
#include "qlinemarksinfocenter.h"

#include <QDir>
#include <QAction>
#include <QTabBar>
#include <QTextEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QResizeEvent>
#include <QStackedWidget>

class EdyukLogFrame : public QStackedWidget
{
	public:
		EdyukLogFrame(QWidget *p = 0)
		 : QStackedWidget(p)
		{
			m_sizeHint = QSize(600, 100);
		}
		
		virtual QSize sizeHint() const
		{
			return m_sizeHint;
			//return m_widget->isVisible() ? QSize(500, 100) : QSize(0, 0);
		}
		
		virtual QSize minimumSizeHint() const
		{
			return QSize(0, 0);
		}
		
		virtual void hideEvent(QHideEvent *e)
		{
			m_sizeHint = size();
			
			QStackedWidget::hideEvent(e);
		}
		
		QSize m_sizeHint;
};

/*!
	\class EdyukLogDock
	\brief Specialized dock widget
	
*/

/*!
	\brief ctor
*/
EdyukLogDock::EdyukLogDock(qmdiMainWindow *p)
 : QDockWidget(p), m_parent(p)
{
	setWindowTitle(tr("Logs"));
	
	m_tabs = new QTabBar(this);
	m_tabs->installEventFilter(this);
	
	m_frame = new EdyukLogFrame(this);
	//m_stack->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	
	connect(m_tabs	, SIGNAL( currentChanged(int) ),
			m_frame	, SLOT  ( setCurrentIndex(int) ) );
	
	#ifdef Q_WS_WIN
	QFont monoFont("Courier", 10);
	#else
	QFont monoFont("Monospace", 10);
	#endif
	monoFont.setStyleHint(QFont::Courier, QFont::PreferQuality);
	
	m_buildMessages = new QListWidget;
	
	connect(m_buildMessages	, SIGNAL( itemActivated(QListWidgetItem*) ),
			this			, SLOT  ( messageActivated(QListWidgetItem*) ) );
	
	m_tabs->addTab(tr("Messages"));
	m_frame->addWidget(m_buildMessages);
	
	m_buildLog = new QTextEdit;
	m_buildLog->setReadOnly(true);
	m_buildLog->document()->setDefaultFont(monoFont);
	m_buildLog->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	
	m_tabs->addTab(tr("Build/Debug log"));
	m_frame->addWidget(m_buildLog);
	
	m_edyukLog = new QTextEdit;
	m_edyukLog->setReadOnly(true);
	m_edyukLog->document()->setDefaultFont(monoFont);
	m_edyukLog->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	
	m_tabs->addTab(tr("Edyuk log"));
	m_frame->addWidget(m_edyukLog);
	
	setTitleBarWidget(m_tabs);
	setWidget(m_frame);
}

/*!
	\brief dtor
*/
EdyukLogDock::~EdyukLogDock()
{
	
}

/*!

*/
QSize EdyukLogDock::minimumSizeHint() const
{
	return m_frame->minimumSizeHint();
}

/*!

*/
QSize EdyukLogDock::sizeHint() const
{
	return m_frame->sizeHint();
}

/*!

*/
void EdyukLogDock::resizeEvent(QResizeEvent *e)
{
	QDockWidget::resizeEvent(e);
}

/*!

*/
void EdyukLogDock::paintEvent(QPaintEvent *e)
{
	QDockWidget::paintEvent(e);
}

/*!

*/
bool EdyukLogDock::eventFilter(QObject *o, QEvent *e)
{
	if ( o == m_tabs )
	{
		switch ( e->type() )
		{
			case QEvent::MouseButtonPress :
				
				if ( size().height() <= (m_tabs->size().height() + 10) )
				{
					//qDebug("gotcha...");
					Qt::DockWidgetArea dw = m_parent->dockWidgetArea(this);
					m_parent->removeDockWidget(this);
					m_frame->m_sizeHint = QSize(600, 100);
					m_frame->resize(600, 100);
					m_parent->update();
					show();
					m_parent->addDockWidget(dw, this);
					show();
					//toggleViewAction()->trigger();
					return false;
				}
				
			case QEvent::MouseButtonRelease :
				break;
				
			default:
				break;
		}
	}
	return QDockWidget::eventFilter(o, e);
}

/*!
	\brief retranslate all strings
*/
void EdyukLogDock::retranslate()
{
	setWindowTitle(tr("Logs"));
	
	m_tabs->setTabText(0, tr("Messages"));
	m_tabs->setTabText(1, tr("Build/Debug log"));
	m_tabs->setTabText(2, tr("Edyuk log"));
}

/*!
	\brief clear logs
*/
void EdyukLogDock::clear()
{
	m_buildLog->clear();
	m_buildMessages->clear();
	m_tabs->setCurrentIndex(1);
}

/*!
	\brief display a log line
*/
void EdyukLogDock::log(const QString& line)
{
	m_buildLog->append(line);
}

/*!
	\brief displays a message from build engine
*/
void EdyukLogDock::message(const QString& file, int line, const QString& msg)
{
//	Q_UNUSED(file)
//	Q_UNUSED(line)
//	Q_UNUSED(msg)
	
	QListWidgetItem *item = new QListWidgetItem;
	item->setText(msg);
	
	if ( file.count() )
	{
		item->setData(File, QDir::cleanPath(file));
		item->setData(Line, line);
		item->setTextColor(Qt::blue);
	}
	
	m_buildMessages->addItem(item);
}

/*!
	\brief display a log line
*/
void EdyukLogDock::edyukLog(const QString& line)
{
	m_edyukLog->append(line);
}

void EdyukLogDock::addExtraWidget(QWidget *w)
{
	m_tabs->addTab(w->windowTitle());
	m_frame->addWidget(w);
}

void EdyukLogDock::removeExtraWidget(QWidget *w)
{
	m_tabs->removeTab(m_frame->indexOf(w));
	m_frame->removeWidget(w);
}

/*!
	
*/
void EdyukLogDock::messageActivated(QListWidgetItem *i)
{
	if ( !i )
		return;
	
	bool ok = false;
	QString fn = i->data(File).toString();
	int ln = i->data(Line).toInt(&ok);
	
	if ( fn.isEmpty() )
		return;
	
	//qDebug("filename : %s", qPrintable(fn));
	
	QWidget *w = m_parent->fileOpen(fn);
//	QEditor *e = qobject_cast<QEditor*>(w);

	QLineMark mrk;
	mrk.mark = QLineMarksInfoCenter::instance()->markTypeId("error");
	mrk.file = fn;
	mrk.line = ln + 1;
	
	QLineMarksInfoCenter::instance()->addLineMark(mrk);
	
//	
//	if ( e && ok && (ln >= 0) )
//	{
//		QDocumentCursor c = e->cursor();
//		c.moveTo(ln, 0);
//		
//		if ( c.isValid() )
//			e->setCursor(c);
//	}
}
