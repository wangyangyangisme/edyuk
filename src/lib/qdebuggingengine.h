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

#ifndef _QDEBUGGING_ENGINE_H_
#define _QDEBUGGING_ENGINE_H_

#include "edyuk.h"

/*!
	\file qdebuggingengine.h
	
	\brief Definition of the QDebuggingEngine class
*/

#include <QObject>
#include <QStringList>

#include "qmdiclient.h"

#include "qdebugger.h"

struct QLineMark;

class EDYUK_EXPORT QDebuggingEngine : public QObject
{
	friend class QDebugger;
	friend class QDebuggingInteractionProxy;
	
	Q_OBJECT
	
	public:
		QDebuggingEngine(QObject *p = 0);
		virtual ~QDebuggingEngine();
		
		static QDebuggingEngine* instance();
		
		bool isInDebuggingSession() const;
		QDebugger* currentDebugger() const;
		QDebuggingInteractionProxy* interactionProxy() const;
		
	public slots:
		void retranslate();
		
		void addDebugger(QDebugger *b);
		
		void setSource(const QString& source);
		void setTarget(const QString& target);
		
		void terminateSession();
		
	signals:
		void started();
		void log(const QString& line);
		void mergingRequested(qmdiClient *c, bool on);
		void widgetInsertionRequested(QWidget *w, bool on);
		
	private slots:
		void lineMarkAdded(const QLineMark& mark);
		void lineMarkRemoved(const QLineMark& mark);
		
	private:
		QDebugger *m_cdbg;
		QList<QDebugger*> m_debuggers;
		
		QString m_activeTarget, m_activeSource;
		
		QDebuggingInteractionProxy *m_interactionProxy;
};

#endif // !_QDEBUGGING_ENGINE_H_
