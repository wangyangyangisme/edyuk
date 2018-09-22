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

/*!
	\file qdebuggingengine.cpp
	
	\brief Implementation of QDebuggingEngine
*/

#include "qdebuggingengine.h"

#include "qdebugginginteractionproxy.h"

#include "qeditor.h"
#include "qlinemarksinfocenter.h"

#include "edyukapplication.h"
#include "edyukgui.h"
#include "qshortcutmanager.h"

#include <QFile>
#include <QAction>

QDebugger::QDebugger()
 : qmdiClient()
{
	
}

QDebugger::~QDebugger()
{
	
}

QString QDebugger::target() const
{
	return QDebuggingEngine::instance()->m_activeTarget;
}

QString QDebugger::source() const
{
	return QDebuggingEngine::instance()->m_activeSource;
}

void QDebugger::started()
{
	QStringList l = COMPONENT(gui)->openedFiles();
	QDebuggingEngine *d = QDebuggingEngine::instance();
	QLineMarksInfoCenter *i = QLineMarksInfoCenter::instance();
	int bid = QLineMarksInfoCenter::instance()->markTypeId("breakpoint");
	
	foreach ( QString f, l )
	{
		QLineMarkList ml = i->marks(f);
		
		foreach ( const QLineMark& m, ml )
		{
			if ( m.mark == bid )
			{
				qDebug("\t# %s:%i", qPrintable(m.file), m.line);
				setBreakpoint(m.file, m.line, true);
			}
		}
	}
	
	emit d->started();
}

QDebuggingInteractionProxy* QDebugger::interactionProxy() const
{
	return QDebuggingEngine::instance()->interactionProxy();
}

void QDebugger::sendLog(const QString& line) const
{
	emit QDebuggingEngine::instance()->log(line);
}

void QDebugger::getCurrentLocation(QString& file, int& line)
{
	line = -1;
	file.clear();
	
	QEditor *e = qobject_cast<QEditor*>(COMPONENT(gui)->activeWindow());
	
	if ( !e )
		return;
	
	file = e->fileName();
	line = e->cursor().lineNumber() + 1;
}

void QDebugger::setLocation(const QString& file, int line, bool activeBkpt)
{
	if ( !activeBkpt )
	{
		QEditor *e = qobject_cast<QEditor*>(COMPONENT(gui)->fileOpen(file));
		
		if ( !e )
			return;
		
		QDocumentCursor c(e->document(), line);
		e->setCursor(c);
		e->setFocus();
		return;
	}
	
	int bid = QLineMarksInfoCenter::instance()->markTypeId("active-breakpoint");
	
	if ( m_break.first.count() && m_break.second > 0 )
	{
		QLineMark mrk(m_break.first, m_break.second, bid);
		QLineMarksInfoCenter::instance()->toggleLineMark(mrk);
	}
	
	m_break.first = file;
	m_break.second = line;
	
	if ( m_break.first.count() && m_break.second > 0 )
	{
		COMPONENT(gui)->fileOpen(m_break.first);
		QLineMark mrk(m_break.first, m_break.second, bid);
		QLineMarksInfoCenter::instance()->toggleLineMark(mrk);
	}
}

void QDebugger::toggleBreakpointOnCurrentLine()
{
	QEditor *e = qobject_cast<QEditor*>(COMPONENT(gui)->activeWindow());
	
	if ( !e )
		return;
	
	int bid = QLineMarksInfoCenter::instance()->markTypeId("breakpoint");
	
	QLineMark mrk(e->fileName(), e->cursor().lineNumber() + 1, bid);
	QLineMarksInfoCenter::instance()->toggleLineMark(mrk);
}

void QDebugger::setVisualBreakpoint(const QString& filename, int line, bool on)
{
	bool prev = QLineMarksInfoCenter::instance()->blockSignals(true);
	int bid = QLineMarksInfoCenter::instance()->markTypeId("breakpoint");
	
	if ( filename.count() && line > 0 )
	{
		QLineMark mrk(filename, line, bid);
		
		if ( on )
			QLineMarksInfoCenter::instance()->addLineMark(mrk);
		else
			QLineMarksInfoCenter::instance()->removeLineMark(mrk);
	}
	
	QLineMarksInfoCenter::instance()->blockSignals(prev);
}

/*!

*/
QDebuggingEngine* QDebuggingEngine::instance()
{
	static QDebuggingEngine m_globalShared;
	
	return &m_globalShared;
}

/*!
	\brief ctor
*/
QDebuggingEngine::QDebuggingEngine(QObject *p)
 : QObject(p), m_cdbg(0)
{
	m_interactionProxy = new QDebuggingInteractionProxy(this);
	
	QLineMarksInfoCenter *lmic = QLineMarksInfoCenter::instance();
	
	connect(lmic, SIGNAL( lineMarkAdded(QLineMark) ),
			this, SLOT  ( lineMarkAdded(QLineMark) ) );
	
	connect(lmic, SIGNAL( lineMarkRemoved(QLineMark) ),
			this, SLOT  ( lineMarkRemoved(QLineMark) ) );
	
}

/*!
	\brief dtor
*/
QDebuggingEngine::~QDebuggingEngine()
{
	
}

bool QDebuggingEngine::isInDebuggingSession() const
{
	return m_cdbg ? m_cdbg->isRunning() : false;
}

QDebugger* QDebuggingEngine::currentDebugger() const
{
	return m_cdbg;
}

void QDebuggingEngine::retranslate()
{
	foreach ( QDebugger *b, m_debuggers )
	{
		b->retranslate();
		
		COMPONENT(shortcutManager)->translateContext(QString("Debuger/%1").arg(b->name()), tr("Debuger/%1").arg(b->label()));
	}
}

QDebuggingInteractionProxy* QDebuggingEngine::interactionProxy() const
{
	return m_interactionProxy;
}

void QDebuggingEngine::addDebugger(QDebugger *b)
{
	if ( !b )
		return;
	
	//qDebug("new debugger : %s", qPrintable(b->fileName()));
	
	m_debuggers << b;
	
	QList<QAction*> l = b->actions();
	
	foreach ( QAction *a , l )
	{
		QKeySequence ks = a->shortcut();
		a->setShortcut(QKeySequence());
		EDYUK_SHORTCUT(a, QString("Debuger/%1").arg(b->name()), ks.toString());
	}
	
	COMPONENT(shortcutManager)->translateContext(QString("Debuger/%1").arg(b->name()), tr("Debuger/%1").arg(b->label()));
}

void QDebuggingEngine::setTarget(const QString& target)
{
	//qDebug("active target : %s", qPrintable(target));
	terminateSession();
	
	bool changed = m_activeTarget != target;
	bool oldExists = QFile::exists(m_activeTarget), newExists = QFile::exists(target);
	
	QDebugger *ndbg = 0;
	
	if ( changed )
	{
		m_activeTarget = target;
		
		foreach ( QDebugger *dbg, m_debuggers )
		{
			if ( dbg->isSupportedInput(target, QString()) )
			{
				ndbg = dbg;
				break;
			}
		}
	} else {
		if ( m_cdbg && (newExists != oldExists) )
		{
			emit mergingRequested(m_cdbg, newExists);
			
			if ( m_cdbg->customDock() )
				emit widgetInsertionRequested(m_cdbg->customDock(), newExists);
			
		}
		
		return;
	}
	
	if ( (m_cdbg == ndbg) && (oldExists == newExists) )
		return;
	
	if ( m_cdbg ) //&& oldExists )
	{
		emit mergingRequested(m_cdbg, false);
		
		if ( m_cdbg->customDock() )
			emit widgetInsertionRequested(m_cdbg->customDock(), false);
		
		m_cdbg = 0;
	}
	
	m_cdbg = ndbg;
	
	if ( m_cdbg && newExists )
	{
		emit mergingRequested(m_cdbg, true);
		
		if ( m_cdbg->customDock() )
			emit widgetInsertionRequested(m_cdbg->customDock(), true);
		
	}
}

void QDebuggingEngine::setSource(const QString& source)
{
	m_activeSource = source;
}

void QDebuggingEngine::terminateSession()
{
	if ( m_cdbg && m_cdbg->isRunning() )
		m_cdbg->terminate();
}

void QDebuggingEngine::lineMarkAdded(const QLineMark& mark)
{
	int mid = QLineMarksInfoCenter::instance()->markTypeId("breakpoint");
	
	if ( !m_cdbg || !m_cdbg->isRunning() || (mark.mark != mid) )
		return;
	
	m_cdbg->setBreakpoint(mark.file, mark.line, true);
}

void QDebuggingEngine::lineMarkRemoved(const QLineMark& mark)
{
	int mid = QLineMarksInfoCenter::instance()->markTypeId("breakpoint");
	
	if ( !m_cdbg || !m_cdbg->isRunning() || (mark.mark != mid) )
		return;
	
	m_cdbg->setBreakpoint(mark.file, mark.line, false);
}
