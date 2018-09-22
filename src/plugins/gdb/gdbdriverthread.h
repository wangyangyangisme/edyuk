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

#ifndef _GDB_DRIVER_THREAD_H_
#define _GDB_DRIVER_THREAD_H_

/*!
	\file gdbdriverthread.h
	
	\brief Definition of GDBDriverThread
*/

#include "gdbresult.h"

#include "qdebugger.h"

#include <QQueue>
#include <QString>
#include <QThread>
#include <QProcess>

struct Breakpoint;
struct GDBMemoryBlock;

class MessageCallback;

class GDBDriverThread : public QThread
{
	Q_OBJECT
	
	public:
		enum DriverState
		{
			Uninitialized,
			Running,
			Waiting,
			Busy,
			Error
		};
		
		enum ResultType
		{
			Unknown,
			Success,
			Failure
		};
		
		GDBDriverThread();
		virtual ~GDBDriverThread();
		
		void removeBreakpoint(const QString& fn, int ln);
		int breakpointId(const QString& fn, int ln) const;
		
		QString relativePath(const QString& s) const;
		
		void setSource(const QString& source);
		void setTarget(const QString& target);
		void command(const QString& cmd, GDBResultHandler *h = 0);
		void command(const QString& cmd, const QStringList& params, GDBResultHandler *h = 0);
		void command(const QString& cmd, const QString& cond, const QStringList& params, GDBResultHandler *h = 0);
		
		void readMemory(GDBMemoryBlock *block);
		
		inline QList<Breakpoint>& breakpoints()
		{ return m_breakpoints; }
		
		inline const QList<Breakpoint>& breakpoints() const
		{ return m_breakpoints; }
		
		bool autoUpdateWatches() const;
		void setAutoUpdateWatches(bool on);
		
		GDBResultHandler* variableUpdateHandler() const;
		void setVariableUpdateHandler(GDBResultHandler *h);
		
		bool autoUpdateRegisters() const;
		void setAutoUpdateRegisters(bool on);
		
		GDBResultHandler* registerUpdateHandler() const;
		void setRegisterUpdateHandler(GDBResultHandler *h);
		
	protected:
		virtual void run();
		
	signals:
		void started();
		void log(const QString& l);
		
		void autoUpdateTick();
		
		void error(const QString& msg) const;
		void warning(const QString& msg) const;
		void question(const QString& msg) const;
		void information(const QString& msg) const;
		
		void error(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		void warning(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		void question(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		void information(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		
		void stateChanged(int state);
		void location(const QString& fn, int line);
		
		void result(RecordNode *root, int type);
		
		void breakpointsChanged();
		
		void setVisualBreakpoint(const QString& file, int line, bool on);
		
	private slots:
		void _runner();
		void _killer();
		
		void readyRead();
		void processCommand();
		void setState(int state);
		void error(QProcess::ProcessError error);
		void finished(int exitCode, QProcess::ExitStatus exitStatus);
		
	private:
		struct Command
		{
			Command()
			 : handler(0) {}
			
			Command(const QString& s)
			 : str(s), handler(0) {}
			
			Command(const QString& s, GDBResultHandler *h)
			 : str(s), handler(h) {}
			
			Command(const QString& s, const QStringList& p, GDBResultHandler *h)
			 : str(s), params(p), handler(h) {}
			
			Command(const QString& s, const QString& c, const QStringList& p, GDBResultHandler *h)
			 : str(s), cond(c), params(p), handler(h) {}
			
			Command(const Command& o)
			 : str(o.str), cond(o.cond), params(o.params), handler(o.handler)
			{}
			
			Command& operator = (const Command& o)
			{
				str = o.str;
				handler = o.handler;
				return *this;
			}
			
			QString str;
			QString cond;
			QStringList params;
			GDBResultHandler *handler;
		};
		
		int m_state;
		int m_promptCount;
		bool m_symbols, m_autoUpdateVar, m_autoUpdateReg;
		QProcess *m_gdb;
		QQueue<Command> m_commands;
		QQueue<QString> m_breakDef;
		QList<Breakpoint> m_breakpoints;
		QString m_target, m_buffer, m_source;
		GDBResultHandler *m_currentHandler, *m_varHandler, *m_regHandler;
};

#endif // !_GDB_DRIVER_THREAD_H_

