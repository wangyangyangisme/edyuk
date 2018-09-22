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

#ifndef _GDB_DRIVER_H_
#define _GDB_DRIVER_H_

#include "qdebugger.h"

#include <QPointer>

class QAction;
class QActionGroup;

struct Breakpoint;
struct RecordNode;
class GDBDriverUi;
class GDBDriverObject;
class GDBDriverThread;
class GDBResultHandler;

class GDBDriver : public QDebugger
{
	friend class GDBDriverObject;
	
	public:
		GDBDriver();
		virtual ~GDBDriver();
		
		virtual void retranslate();
		
		virtual QString name() const;
		virtual QString label() const;
		
		virtual void terminate();
		virtual bool isRunning() const;
		
		virtual QWidget* customDock() const;
		virtual void setBreakpoint(const QString& filename, int line, bool on);
		virtual bool isSupportedInput(const QString& file, const QString& language) const;
		
		void command(const QString& cmd, GDBResultHandler *h = 0);
		void command(const QString& cmd, const QStringList& params, GDBResultHandler *h = 0);
		void command(const QString& cmd, const QString& cond, const QStringList& params, GDBResultHandler *h = 0);
		
		QList<Breakpoint>& breakpoints();
		const QList<Breakpoint>& breakpoints() const;
		
		inline void setLocation(const QString& file, int line, bool activate = true)
		{ QDebugger::setLocation(file, line, activate); }
		
	protected:
		void start();
		void customCommand();
		void toggleBreakpoint();
		void command(QAction *a);
		void stateChanged(int state);
		void forwardLog(const QString& l);
		void location(const QString& fn, int line);
		
		void processResult(RecordNode *root, int type);
		
	private:
		GDBDriverObject *o;
		GDBDriverThread *d;
		QPointer<GDBDriverUi> ui;
		
		QActionGroup *m_group;
		
		QAction *m_start,
				*m_stop,
				*m_break,
				*m_backtrace,
				*m_continue,
				*m_step,
				*m_stepi,
				*m_next,
				*m_nexti,
				*m_until,
				*m_runto,
				*m_finish,
				*m_customCommand;
};

#endif // !_GDB_DRIVER_H_

