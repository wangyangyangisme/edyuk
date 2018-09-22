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
	\file gdbdriverthread.cpp
	
	\brief Implementation of GDBDriverThread
*/

#include "gdbdriverthread.h"

#include "gdbmemory.h"

#include "plugin.h"
#include "edyuk.h"
#include "qdebugginginteractionproxy.h"

#include <QTimer>
#include <QProcess>
#include <QFileInfo>
#include <QTextStream>
#include <QMessageBox>
#include <QReadWriteLock>
#include <QCoreApplication>

#define GNU_GDB_DEFAULT "gdb"

/*
	IMPORTANT : reserved command prefix (digit seq id)
	
	1 - 9 : reserved for GDBDriverThread internal use (forwarding blocked)
	
	11, 12 : register values 11 for hex formatting, 12 for default formatting
	
	15 : expression evaluation directly requested by the user, output to be directly displayed
	
	20 : reserved for internal use of GDBMemoryReader
	
	1000 - 1499 : reserved for register watchpoints
	
	
*/

class BlockingMessageCallback : public MessageCallback
{
	public:
		BlockingMessageCallback()
		 : m_hasAnswer(false), m_answer(0)
		{
		
		}
		
		int answer() const
		{
			return m_answer;
		}
		
		bool hasAnswer() const
		{
			return m_hasAnswer;
		}
		
		void wait()
		{
			qDebug("waiting");
			//QCoreApplication::processEvents();
			forever
			{
				m_lock.lockForRead();
				if ( hasAnswer() )
				{
					m_lock.unlock();
					break;
				}
				m_lock.unlock();
				
				QCoreApplication::processEvents();
				//QThread::msleep(50);
			}
		}
		
		virtual void response(int button)
		{
			QWriteLocker locker(&m_lock);
			m_answer = button;
			m_hasAnswer = true;
		}
		
	private:
		QReadWriteLock m_lock;
		bool m_hasAnswer;
		int m_answer;
};

/*
class GDBMemoryReader : public GDBResultHandler
{
	private:
		GDBMemoryBlock *m_block;
		
	public:
		GDBMemoryReader(GDBMemoryBlock *dest)
		 : m_block(dest)
		{
			
		}
		
		virtual bool discardable() const { return true; }
		
		virtual bool result(RecordNode *root, int nt)
		{
			if ( !m_block || !root || nt != GDBDriverThread::Success )
				return false;
			
			RecordNode *rn = root->field("memory");
			
			if ( !rn || rn->children.count() != 1 )
			{
				qWarning("something fucks up memory reading");
			} else {
				RecordNode *d = rn->children.first()->field("data");
				
				m_block->valid = false;
				m_block->data.resize(d->children.count());
				
				int col = 0;
				
				foreach ( RecordNode *c, d->children )
				{
					m_block->data[col] = c->value.toInt();
					++col;
				}
				
				m_block->valid = true;
			}
			
			return false;
		};
};
*/

GDBDriverThread::GDBDriverThread()
 : m_autoUpdateVar(false), m_autoUpdateReg(false), m_gdb(0), m_varHandler(0), m_regHandler(0)
{
	
}

GDBDriverThread::~GDBDriverThread()
{
	_killer();
}

QString GDBDriverThread::relativePath(const QString& s) const
{
	return Edyuk::makeRelative(m_source.count() ? m_source : m_target, s);
}

int GDBDriverThread::breakpointId(const QString& fn, int ln) const
{
	foreach ( const Breakpoint& bkpt, m_breakpoints )
	{
		if ( (fn == bkpt.file) && (ln == bkpt.line) )
			return bkpt.id;
	}
	
	return -1;
}

void GDBDriverThread::setTarget(const QString& target)
{
	m_target = target;
}

void GDBDriverThread::setSource(const QString& source)
{
	m_source = source;
}

bool GDBDriverThread::autoUpdateWatches() const
{
	return m_autoUpdateVar;
}

void GDBDriverThread::setAutoUpdateWatches(bool on)
{
	m_autoUpdateVar = on;
}

bool GDBDriverThread::autoUpdateRegisters() const
{
	return m_autoUpdateReg;
}

void GDBDriverThread::setAutoUpdateRegisters(bool on)
{
	m_autoUpdateReg = on;
}

GDBResultHandler* GDBDriverThread::variableUpdateHandler() const
{
	return m_varHandler;
}

void GDBDriverThread::setVariableUpdateHandler(GDBResultHandler *h)
{
	m_varHandler = h;
}

GDBResultHandler* GDBDriverThread::registerUpdateHandler() const
{
	return m_regHandler;
}

void GDBDriverThread::setRegisterUpdateHandler(GDBResultHandler *h)
{
	m_regHandler = h;
}

void GDBDriverThread::run()
{
	QTimer::singleShot(0, this, SLOT( _runner() ));
	
	exec();
	
	m_breakpoints.clear();
	emit breakpointsChanged();
}

void GDBDriverThread::_runner()
{
	m_currentHandler = 0;
	m_breakpoints.clear();
	setState(Uninitialized);
	emit breakpointsChanged();
	
	bool autoUpdateVar = GDBPlugin::configKey<bool>("QDebugger/GDBDriver/autoUpdateVar", true);
	bool autoUpdateReg = GDBPlugin::configKey<bool>("QDebugger/GDBDriver/autoUpdateReg", true);
	
	setAutoUpdateWatches(autoUpdateVar);
	setAutoUpdateRegisters(autoUpdateReg);
	
	QFileInfo info(m_target);
	
	if ( !m_gdb )
	{
		m_gdb = new QProcess;
		m_gdb->setReadChannelMode(QProcess::MergedChannels);
		
		connect(m_gdb	, SIGNAL( readyRead() ),
				this	, SLOT  ( readyRead() ) );
		
		connect(m_gdb	, SIGNAL( finished(int, QProcess::ExitStatus) ),
				this	, SLOT  ( finished(int, QProcess::ExitStatus) ) );
		
	}
	
	QStringList env = QProcess::systemEnvironment();
	
	#if defined(Q_OS_UNIX) || defined(Q_OS_MAC)
	
	#ifdef Q_OS_UNIX
	QString var = "LD_LIBRARY_PATH=";
	#elif defined(Q_OS_MAC)
	QString var = "DYLD_LIBRARY_PATH=";
	#endif
	
	bool bOK = false;
	QStringList pathes;
	pathes << info.absolutePath();
	
	QString spath = QFileInfo(m_source).absolutePath();
	
	if ( !pathes.contains(spath) )
		pathes << spath;
	
	for ( int i = env.count() - 1; i >= 0; i-- )
	{
		if ( env.at(i).startsWith(var) )
		{
			bOK = true;
			env[i] += ':';
			env[i] += pathes.join(":");
			
			break;
		}
	}
	
	if ( !bOK )
		env << var + pathes.join(":");
	#endif
	
	m_gdb->setEnvironment(env);
	m_gdb->setTextModeEnabled(true);
	m_gdb->setWorkingDirectory(info.absolutePath());
	
	emit log("\n-- GDB Driver : Session start --\n");
	
	// assume symbols present
	m_symbols = true;
	m_promptCount = 0;
	
	m_gdb->start(
			GDBPlugin::configKey<QString>("QDebugger/GDBDriver/gdb", GNU_GDB_DEFAULT),
			QStringList()
			<< "--quiet"
			<< "--fullname" << info.absoluteFilePath()
			<< "--interpreter=mi2"
		);
	
	if ( !m_gdb->waitForStarted(2000) )
	{
		emit error("Unable to start GDB : check your installation and environment variables.");
		emit log("\n-- GDB Driver : Session end --\n");
		
		quit();
		return;
	}
	
	m_gdb->waitForReadyRead(2000);
	
	//qDebug("m_symbols = %i", (int)m_symbols);
	// Use a convenience breakpoint at main if symbols present, run directly otherwise
	command(m_symbols ? "start" : "-exec-run");
	
	m_gdb->waitForBytesWritten(1000);
	m_gdb->waitForReadyRead(1000);
	
	if ( m_symbols )
	{
		command("-data-list-register-names", m_regHandler);
		command("11-data-list-register-values x", m_regHandler);
		command("12-data-list-register-values N", m_regHandler);
	}
	
	emit started();
}

void GDBDriverThread::_killer()
{
	if ( !m_gdb )
		return;
	
	m_commands.clear();
	
	if ( m_state == Waiting )
	{
		command("-gdb-exit");
	}
	
	m_gdb->kill();
	m_gdb->waitForFinished();
	delete m_gdb;
	
	m_gdb = 0;
}

static int fieldEnd(const QString& s, int i)
{
	int nest = 0;
	bool quoted = false, escaped = false;
	
	for ( ; i < s.count(); ++i )
	{
		QChar c = s.at(i);
		
		if ( quoted )
		{
			if ( escaped )
			{
				escaped = false;
			} else if ( c == '\"' ) {
				quoted = !quoted;
			} else if ( c == '\\' ) {
				escaped = !escaped;
			}
		} else {
			if ( c == '\'' )
			{
				++i;
				
				while ( (i < s.count()) && (s.at(i) != '\'') )
				{
					if ( c == '\\' )
						++i;
					
					++i;
				}
				
			} else if ( c == '{' || c == '[' ) {
				++nest;
			} else if ( c == '}' || c == ']' ) {
				--nest;
			} else if ( !nest && (c == ',') ) {
				break;
			} else if ( c == '\"' ) {
				quoted = true;
			}
		}
	}
	
	return i;
}

static int fieldEnd(const QString& s, int i, QChar nest)
{
	bool quoted = false, escaped = false;
	
	for ( ; i < s.count(); ++i )
	{
		QChar c = s.at(i);
		
		if ( quoted )
		{
			if ( escaped )
			{
				escaped = false;
			} else if ( c == '\"' ) {
				quoted = !quoted;
			} else if ( c == '\\' ) {
				escaped = !escaped;
			}
		} else {
			if ( c == '\'' )
			{
				++i;
				
				while ( (i < s.count()) && (s.at(i) != '\'') )
				{
					if ( c == '\\' )
						++i;
					
					++i;
				}
				
			} else if ( c == '{' ) {
				i = fieldEnd(s, i + 1, '}');
			} else if ( c == '[' ) {
				i = fieldEnd(s, i + 1, ']');
			} else if ( (nest == c) && !nest.isNull() ) {
				break;
			} else if ( !nest.isNull() && (c == ',') ) {
				break;
			} else if ( c == '\"' ) {
				quoted = true;
			}
		}
	}
	
	return i;
}

static bool isComposite(const QString& s)
{
	if ( !((s.startsWith('{') && s.endsWith('}')) || (s.startsWith('[') && s.endsWith(']'))) )
		return false;
	
	bool c = fieldEnd(s, 0) == s.count();
	
	return c;
}

static void parse(const QString& s, RecordNode *root)
{
	root->type = RecordNode::Composite;
	//qDebug("parsing : %s", qPrintable(s));
	
	int idx = 0;
	QString buffer;
	
	/*
	if ( isComposite(s) && !(s.startsWith('[') && root->children.count()) )
	{
		parse(s.mid(1, s.count() - 2), root);
		
		return;
	}
	*/
	
	int index = 0;
	bool quoted = false, escaped = false;
	
	for ( int i = 0; i < s.count(); ++i )
	{
		QChar c = s.at(i);
		
		if ( !quoted && (c == ',') )
		{
			if ( buffer.count() )
			{
				// list...
				
				RecordNode *n = new RecordNode(QString::number(index++));
				n->value = buffer;
				root->children << n;
			}
			
			buffer.clear();
			continue;
		} else if ( !quoted && (c == '=') ) {
			RecordNode *n = new RecordNode(buffer);
			root->children << n;
			
			int len = fieldEnd(s, i + 1) - (i + 1);
			
			QString sub = s.mid(i + 1, len);
			
			//qDebug("field:(name, value)=(%s, %s)", qPrintable(buffer), qPrintable(sub));
			
			if ( !isComposite(sub) )
			{
				if ( sub.startsWith('\"') )
					sub.remove(0, 1);
				
				if ( sub.endsWith('\"') )
					sub.chop(1);
				
				n->value = sub;
			} else {
				parse(sub.mid(1, sub.count() - 2), n);
			}
			
			i = i + len + 1;
			buffer.clear();
			continue;
		} else if ( (c == '{') || c == '[' ) {
			RecordNode *n = new RecordNode(QString::number(idx++));
			root->children << n;
			
			int e = fieldEnd(s, i);
			parse(s.mid(i + 1, e - (i + 2)), n);
			
			buffer.clear();
			i = e;
			continue;
		} else if ( !escaped && (c == '\"') ) {
			quoted = !quoted;
			continue;
		} else if ( quoted && (c == '\\') ) {
			escaped = !escaped;
		}
		
		
		buffer += c;
	}
	
	if ( buffer.isEmpty() )
		return;
	
	RecordNode *n = new RecordNode(QString::number(index));
	n->value = buffer;
	root->children << n;
}

static QFile _gdb_log_file(".gdb.log");
static QTextStream _gdb_log;

void dump(RecordNode *n, QString indent = QString())
{
	int cnt = n->children.count();
	
	QString prefix = indent + n->name + ' ';
	
	if ( n->type == RecordNode::Simple )
	{
		//qDebug("%s= %s\n", qPrintable(prefix), qPrintable(n->value));
		_gdb_log << prefix << "= " << n->value << endl << endl;
	} else if ( cnt ) {
		//qDebug("%s{", qPrintable(prefix));
		_gdb_log << prefix << "{" << endl;
		indent += '\t';
		
		foreach ( RecordNode *c, n->children )
			dump(c, indent);
		
		indent.chop(1);
		//qDebug("%s}", qPrintable(indent));
		
		_gdb_log << indent << "}" << endl;
	} else {
		//qDebug("%s= ?", qPrintable(prefix));
		
		_gdb_log << prefix << "= ?" << endl;
	}
}

static QString indent;

void GDBDriverThread::readyRead()
{
	if ( !_gdb_log.device() )
	{
		_gdb_log_file.open(QFile::WriteOnly);
		_gdb_log.setDevice(&_gdb_log_file);
	}
	
	setState(Busy);
	
	QStringList l;
	quint64 sz = m_gdb->bytesAvailable();
	static const quint64 chunkSize = 1024;
	static const QRegExp unstripped("0x[0-9a-fA-F]+(\\s*\\w+)?\\s+main\\s*\\(\\)");
	
	while ( sz )
	{
		int chunk = qMin(chunkSize, sz);
		QByteArray d = m_gdb->read(chunk);
		
		//_gdb_log << d;
		
		char c = d.count() ? d.at(0) : 0;
		
		// in case of badly buffered output from debugged process make sure GDB output don't get lost
		//if ( c == '*' || c == '^' || c == '@' || c == '&' || c == '+' || c == '=' )
		//	m_buffer += '\n';
		
		m_buffer += QString::fromLocal8Bit(d);
		
		sz -= chunk;
		
		//qDebug(">> [chunk] \"%s\"", d.constData());
		
		int idx = m_buffer.indexOf('\n');
		
		while ( idx != -1 )
		{
			if ( idx == (m_buffer.length() - 1) )
			{
				m_buffer.chop(1);
				l << m_buffer;
				m_buffer.clear();
			} else {
				l << m_buffer.left(idx);
				m_buffer = m_buffer.mid(idx + 1);
			}
			
			//qDebug(">> [line] \"%s\"", qPrintable(l.last()));
			
			idx = m_buffer.indexOf('\n');
		}
	}
	
	indent += ' ';
	
	//foreach ( const QString& s , l )
	//	qDebug("%s@%s", qPrintable(indent), qPrintable(s));
	
	bool m_prompt = false;
	
	for ( int i = 0; i < l.count(); ++i )
	{
		QString s = l.at(i);
		
		s.remove("\\n"); //.remove("\\\"").remove("\"");
		s = s.trimmed();
		
		if ( s.isEmpty() )
			continue;
		
		//qDebug("%s", qPrintable(s));
		
		_gdb_log << endl << " -- Parsing -- " << endl;
		_gdb_log << s << endl << "-- EOP -- " << endl;
		
		m_prompt = false;
		
		RecordNode rn("/root");
		rn.type = RecordNode::Composite;
		
		int len = 0;
		
		while ( s.at(len).isDigit() )
		{
			++len;
		}
		
		if ( len )
		{
			rn.id = s.left(len).toInt();
			
			s.remove(0, len);
			//qDebug("tagged request : %i, %s ", rn.id, qPrintable(s));
		}
		
		int comma = s.indexOf(',');
		
		if ( (s.at(0) == '*' || s.at(0) == '^') && (comma != -1) )
		{
			parse(s.mid(comma + 1), &rn);
			dump(&rn);
			_gdb_log << endl << " -- EOD -- " << endl;
		} else if ( comma == -1 ) {
			if ( s.at(1) == '\"' )
				s.remove(1, 1);
			
			if ( s.endsWith('\"') )
				s.chop(1);
			
			comma = s.length();
			_gdb_log << endl << " -- EOD -- " << endl;
		}
		
		if ( s == "(gdb)" )
		{
			// back to prompt : enter Waiting state
			m_prompt = true;
			++m_promptCount;
		} else if ( s.at(0) == '~' ) {
			// GDB output record : console stream
			s = s.mid(1);
			
			if ( m_promptCount <= 2 )
			{
				if ( s == "(no debugging symbols found)" || unstripped.exactMatch(s) )
				{
					m_symbols = false;
					emit information("The debug target does not provide debugging symbols.");
				} else if (
							s.contains("No such file or directory")
						||
							s.contains("Program exited with code")
						)
				{
					// no point staying...
					BlockingMessageCallback waitForConfirmation;
					
					emit error(
							"Failed to start the debugging target.\n"
							"All pendings command will be discarded.\n"
							"Do you want to close the debugging session?",
							&waitForConfirmation,
							QMessageBox::Yes | QMessageBox::No
							);
					
					waitForConfirmation.wait();
					
					m_commands.clear();
					
					if ( waitForConfirmation.answer() != QMessageBox::No )
					{
						command("-gdb-exit");
					}
				}
			} else {
				emit log(s);
			}
			
			if ( s.startsWith("\\032\\032") )
			{
				QStringList f = s.remove(0, 8).split(':');
				
				if ( f.count() > 1 )
					emit location(f.at(0), f.at(1).toInt());
			}
			
		} else if ( s.at(0) == '@' ) {
			// GDB output record : target stream
			emit log(tr("target : %1").arg(s.mid(1)));
			
		} else if ( s.at(0) == '&' ) {
			// GDB output record : log stream
			//emit log(tr("(gdb) %1").arg(s.mid(1)));
			
		} else if ( s.at(0) == '*' ) {
			// GDB output record : exec stream
			QString c = s.mid(1, comma - 1);
			
			//qDebug("* %s", qPrintable(c));
			
			if ( c == "stopped" )
			{
				QString stopreason = rn.fieldValue("reason");
				
				RecordNode *frame = rn.field("frame");
				
				if ( frame )
				{
					QString fn = frame->fieldValue("fullname");
					int line = frame->fieldValue("line").toInt();
					
					emit location(fn, line);
				}
				
				if ( stopreason == "exited" )
				{
					//finished(rn.fieldValue("exit-code").toInt(), QProcess::NormalExit);
					BlockingMessageCallback waitForConfirmation;
					
					emit information(
							QString(
								"The program being debugged exited with code.\n"
								"All pendings command will be discarded.\n"
								"Do you want to close the debugging session?"
							).arg(rn.fieldValue("exit-code")),
							&waitForConfirmation,
							QMessageBox::Yes | QMessageBox::No
							);
					
					waitForConfirmation.wait();
					
					m_commands.clear();
					
					if ( waitForConfirmation.answer() != QMessageBox::No )
					{
						command("-gdb-exit");
					}
				} else if ( stopreason == "exited-normally" ) {
					//finished(0, QProcess::NormalExit);
					BlockingMessageCallback waitForConfirmation;
					
					emit information(
							"The program being debugged exited normally.\n"
							"All pendings command will be discarded.\n"
							"Do you want to close the debugging session?",
							&waitForConfirmation,
							QMessageBox::Yes | QMessageBox::No
							);
					
					waitForConfirmation.wait();
					
					m_commands.clear();
					
					if ( waitForConfirmation.answer() != QMessageBox::No )
					{
						command("-gdb-exit");
					}
				} else if ( stopreason == "exited-signalled" ) {
					
					emit information(
							"The program being debugged exited due to a system signal."
							);
					
					//command("-gdb-exit");
				} else if ( stopreason == "breakpoint-hit" ) {
					
					int id = rn.fieldValue("bkptno").toInt();
					
					for ( int i = 0; i < m_breakpoints.count(); ++i )
					{
						if ( m_breakpoints.at(i).id == id )
							++m_breakpoints[i].times;
					}
					
					emit breakpointsChanged();
					emit autoUpdateTick();
					
					// update registers/variables/... ?
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				} else if ( stopreason == "watchpoint-trigger" ) {
					// get old and new value : how to display?
					
					emit autoUpdateTick();
					
					// update registers/variables/... ?
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				} else if ( stopreason == "read-watchpoint-trigger" ) {
					// do something?
					
					emit autoUpdateTick();
					
					// update registers/variables/... ?
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				} else if ( stopreason == "access-watchpoint-trigger" ) {
					// do something?
					
					emit autoUpdateTick();
					
					// update registers/variables/... ?
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				} else if ( stopreason == "location-reached" ) {
					// update registers/variables/... ?
					
					emit autoUpdateTick();
					
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				} else if ( stopreason == "function-finished" ) {
					// update registers/variables/... ?
					
					emit autoUpdateTick();
					
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				} else if ( stopreason == "end-stepping-range" ) {
					// update registers/variables/... ?
					
					emit autoUpdateTick();
					
					//if ( m_autoUpdateVar )
					//	command("-var-update --all-values *", m_varHandler);
					
					//if ( m_autoUpdateReg )
					//	command("-data-list-changed-registers", m_regHandler);
					
				}
			}
			
		} else if ( s.at(0) == '+' ) {
			// GDB output record : status stream
			QString c = s.mid(1, comma - 1);
			
			qDebug("+ %s", qPrintable(c));
			
		} else if ( s.at(0) == '=' ) {
			// GDB output record : notify stream
			QString c = s.mid(1, comma - 1);
			
			qDebug("= %s", qPrintable(c));
			
		} else if ( s.at(0) == '^' ) {
			// GDB output record : result stream
			QString c = s.mid(1, comma - 1);
			
			//qDebug("^ %s", qPrintable(c));
			
			if ( c == "done" )
			{
				// operation succeded : parse output
				
				RecordNode *brk = rn.field("bkpt");
				RecordNode *wpt = rn.field("wpt");
				RecordNode *bt = rn.field("BreakpointTable");
				
				if ( brk )
				{
					Breakpoint bkpt;
					
					bool ok = false;
					int bln = brk->fieldValue("line").toInt(&ok);
					
					bkpt.id = brk->fieldValue("number").toInt();
					if ( ok ) bkpt.line = bln;
					bkpt.file = brk->fieldValue("fullname");
					bkpt.function = brk->fieldValue("func");
					bkpt.address = brk->fieldValue("addr");
					bkpt.condition = brk->fieldValue("cond");
					bkpt.times = brk->fieldValue("times").toInt();
					bkpt.ignore = brk->fieldValue("ignore").toInt();
					bkpt.enabled = brk->fieldValue("enabled") == "y";
					
					QString def = m_breakDef.dequeue();
					
					if ( def.contains(':') )
					{
						if ( bkpt.file.isEmpty() )
						{
							bkpt.file = def.section(':', 0, 0);
						}
						
						bool ok = false;
						QString extra = def.section(':', 1, 1);
						int ln = extra.toInt(&ok);
						
						if ( (bkpt.line != ln) && ok )
						{
							bkpt.line = ln;
						} else if ( !ok && bkpt.function.isEmpty() ) {
							bkpt.function = extra;
						}
					} else if ( def.startsWith('*') && (bkpt.address.isEmpty() || bkpt.address == "<MULTIPLE>") ) {
						bkpt.address = def.mid(1);
					}
					
					m_breakpoints << bkpt;
					emit breakpointsChanged();
					
					//qDebug("breakpoint created : %s:%i [%s]", qPrintable(bkpt.file), bkpt.line, qPrintable(l.at(i)));
				} else if ( bt ) {
					bt = bt->field("body");
					
					foreach ( RecordNode *c, bt->children )
					{
						if ( c->name != "bkpt" )
							continue;
						
						Breakpoint& bkpt = m_breakpoints[c->fieldValue("number").toInt()];
						
						bkpt.condition = c->fieldValue("cond");
						bkpt.times = c->fieldValue("times").toInt();
						bkpt.ignore = c->fieldValue("ignore").toInt();
						bkpt.enabled = c->fieldValue("enabled") == "y";
					}
					
					emit breakpointsChanged();
				} else if ( rn.id == 1 ) {
					// reserved for command parametrization (through expr evaluation)
					//qDebug("reserved output");
					
					//dump(&rn);
					
					if ( rn.field("value") && m_commands.count() )
					{
						Command& cmd = m_commands.head();
						QString val = rn.fieldValue("value");
						QString seq = QString::number(cmd.params.count() + 1);
						seq.prepend('%');
						
						//qDebug("result of parametrization : %s", qPrintable(val));
						
						cmd.str.replace(seq, val);
						cmd.cond.replace(seq, val);
					}
				} else if ( rn.id == 2 ) {
					// reserved for command conditionalization
					
					//qDebug("conditional result : %s", qPrintable(rn.fieldValue("value")));
					
					if ( rn.field("value") && m_commands.count() )
					{
						QString val = rn.fieldValue("value");
						
						if ( val != "1" && val != "true" )
						{
							Command cmd = m_commands.dequeue();
							
							if ( cmd.handler )
							{
								RecordNode fake("/root");
								int idx = cmd.str.indexOf("-");
								
								if ( idx != 0 && idx != -1 )
									fake.id = cmd.str.left(idx).toInt();
								
								cmd.handler->result(&fake, Failure);
							}
						}
					}
					
				} else if ( rn.id == 3 ) {
					// reserved
					qDebug("reserved output");
					
				} else if ( rn.id == 4 ) {
					// reserved
					qDebug("reserved output");
					
				} else if ( rn.id == 5 ) {
					// reserved
					qDebug("reserved output");
					
				} else if ( rn.id == 6 ) {
					// reserved
					qDebug("reserved output");
					
				} else if ( rn.id == 7 ) {
					// reserved
					qDebug("reserved output");
					
				} else if ( rn.id == 8 ) {
					// reserved
					qDebug("reserved output");
					
				} else if ( rn.id == 9 ) {
					// reserved
					qDebug("reserved output");
					
				} else {
					//qDebug("propagating record... [%x, %x]", m_currentHandler, m_varHandler);
					//dump(&rn);
					
					bool fwd = true;
					
					if ( m_currentHandler )
					{
						if ( m_currentHandler->result(&rn, Success) )
						{
							fwd = false;
							
							if ( m_currentHandler->discardable() )
								delete m_currentHandler;
						}
					}
					
					if ( fwd )
					{
						emit result(&rn, Success);
					}
				}
				
			} else if ( c == "running" ) {
				// asynchronous operation going on...
				
			} else if ( c == "connected" ) {
				// 
				
			} else if ( c == "error" ) {
				// error : notify the user
				emit error(rn.fieldValue("msg")); //log(tr("Error : %1").arg(s.mid(comma + 1)));
			} else if ( c == "exit" ) {
				// gdb exited : end debugging session
				
				//finished(0, QProcess::NormalExit);
				//_killer();
			} else {
				if ( !(m_currentHandler && m_currentHandler->result(&rn, Unknown)) )
					emit result(&rn, Unknown);
			}
			
		} else if ( unstripped.exactMatch(s) ) {
			m_symbols = false;
		} else {
			emit log(s);
		}
	}
	
	//qDebug("parsing finished.");
	
	indent.chop(1);
	
	if ( m_prompt )
	{
		//qDebug("%sresetting handler.", qPrintable(indent));
		m_currentHandler = 0;
		processCommand();
	}
}

void GDBDriverThread::setState(int state)
{
	m_state = state;
	
	emit stateChanged(state);
}

void GDBDriverThread::error(QProcess::ProcessError e)
{
	emit error(tr("Unable to start GDB : check your installation."));
	emit log("\n-- GDB Driver : Session end --\n");
	setState(Uninitialized);
	
	quit();
}

void GDBDriverThread::finished(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit log("\n-- GDB Driver : Session end --\n");
	setState(Uninitialized);
	
	quit();
}

void GDBDriverThread::removeBreakpoint(const QString& fn, int ln)
{
	QList<int>ids;
	
	foreach ( const Breakpoint& bkpt, m_breakpoints )
	{
		if ( (bkpt.file) == fn && (bkpt.line == ln) )
			ids << bkpt.id;
	}
	
	if ( ids.isEmpty() )
		return;
	
	QString idl;
	
	foreach ( int id, ids )
		idl += QString::number(id).prepend(' ');
	
	command(QString("-break-delete") + idl);
}

void GDBDriverThread::command(const QString& str, GDBResultHandler *h)
{
	m_commands.enqueue(Command(str, h));
	
	if ( m_state == Waiting )
	{
		//qDebug("%sprocessing command.", qPrintable(indent));
		processCommand();
	}
}

void GDBDriverThread::command(const QString& str, const QStringList& params, GDBResultHandler *h)
{
	m_commands.enqueue(Command(str, params, h));
	
	if ( m_state == Waiting )
	{
		//qDebug("%sprocessing command.", qPrintable(indent));
		processCommand();
	}
}

void GDBDriverThread::command(const QString& str, const QString& cond, const QStringList& params, GDBResultHandler *h)
{
	m_commands.enqueue(Command(str, cond, params, h));
	
	if ( m_state == Waiting )
	{
		//qDebug("%sprocessing command.", qPrintable(indent));
		processCommand();
	}
}

void GDBDriverThread::readMemory(GDBMemoryBlock *block)
{
	if ( !block )
		return;
	
	//static const QString cmd("-data-read-memory %1 x 1 1 %2");
	
	//GDBMemoryReader *reader = new GDBMemoryReader(block);
	
	//command(cmd.arg(reinterpret_cast<quintptr>(block->address)).arg(block->data.size()), reader);
}

void GDBDriverThread::processCommand()
{
	if ( m_commands.isEmpty() )
	{
		setState(Waiting);
		//qDebug("%sprompting back!", qPrintable(indent));
		return;
	}
	
	QString cmdstr;
	Command& cmd = m_commands.head();
	
	if ( cmd.params.count() )
	{
		//qDebug("parameter evaluation");
		
		cmdstr = "1-data-evaluate-expression \"";
		cmdstr += cmd.params.takeLast();
		cmdstr += '\"';
		m_currentHandler = 0;
	} else if ( cmd.cond.count() ) {
		cmdstr = "2-data-evaluate-expression \"";
		cmdstr += cmd.cond;
		cmdstr += '\"';
		m_currentHandler = 0;
		
		cmd.cond.clear();
	} else {
		Command cmd = m_commands.dequeue();
		
		if ( cmd.str.startsWith("-break-delete") )
		{
			QStringList ids = cmd.str.mid(13).simplified().split(' ');
			
			foreach ( QString id, ids )
			{
				int k = id.toInt();
				
				for ( int i = 0; i < m_breakpoints.count(); ++i )
				{
					if ( m_breakpoints.at(i).id == k )
					{
						const Breakpoint& bkpt = m_breakpoints.at(i);
						emit setVisualBreakpoint(bkpt.file, bkpt.line, false);
						
						m_breakpoints.removeAt(i);
						
						break;
					}
				}
			}
			
			emit breakpointsChanged();
		} else if ( cmd.str.startsWith("-break-insert") ) {
			QString def = cmd.str.mid(14);
			m_breakDef.enqueue(def);
		}
		
		//qDebug("%ssetting handler to %x [%s]", qPrintable(indent), cmd.handler, qPrintable(cmd.str));
		
		m_currentHandler = cmd.handler;
		cmdstr = cmd.str;
	}
	
	setState(Busy);
	cmdstr += '\n';
	m_gdb->write(cmdstr.toLocal8Bit());
	
	if ( !m_gdb->waitForBytesWritten(500) )
	{
		emit error(tr("Unable to send command to GDB."));
		emit log("\n-- GDB Driver : Session end --\n");
		quit();
		return;
	}
	
	emit log(QString("(gdb) ") + cmdstr);
}
