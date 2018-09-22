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

#include "qbuildengine.h"

/*!
	\file qbuildengine.cpp
	\brief Implementation of the QBuildEngine class.
*/

#include "qbuilder.h"

#include "qshortcutmanager.h"
#include "edyukapplication.h"
#include "edyuktemplatemanager.h"

#include <QDir>
#include <QMenu>
#include <QMutex>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QAction>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>
#include <QActionGroup>
#include <QInputDialog>
#include <QProgressDialog>

// TODO : merge exec action and exec target selection...

class QBuildTask : public QThread
{
	Q_OBJECT
	
	public:
		QBuildTask(QBuilder::Command *c, const QString& mode, QBuildEngine *e, QProgressDialog *d)
		 : m_index(0), m_abort(false), m_scan(false), m_process(0), m_engine(e), m_mode(mode), m_progress(d)
		{
			m_source = m_input = e->m_activeSource;
			
			if ( c->isStandalone() )
			{
				QString mode = m_engine->selectedMode(c);
				
				m_commands << c;
				m_modes << mode;
				
				if ( c->depends().count() )
				{
					int idx;
					int depcount = m_commands.count();
					QList<QBuilder::Command*> cmd = m_commands;
					
					m_commands << c->depends();
					depcount = m_commands.count() - depcount;
					
					for ( int i = 0; i < depcount; ++i )
						m_modes << mode;
					
					while ( (idx = m_commands.indexOf(0)) != -1 )
					{
						m_modes.removeAt(idx);
						m_commands.removeAt(idx);
						
						foreach ( QBuilder::Command *cc, cmd )
						{
							m_commands.insert(idx, cc);
							m_modes.insert(idx, m_engine->selectedMode(cc));
						}
					}
				}
				
				//qDebug("\t %p [final]", c);
			} else {
				QBuildChain cb = m_engine->m_pipelines.value(m_engine->m_activePipeline);
				
				foreach ( QBuilder *b, cb )
				{
					//qDebug("command(s) for step %i", ++idx);
					
					if ( b->commands().isEmpty() )
						continue;
					
					if ( b->commands().contains(c) )
					{
						QString mode = m_engine->selectedMode(c);
						
						m_commands << c;
						m_modes << mode;
						
						if ( c->depends().count() )
						{
							int idx;
							int depcount = m_commands.count();
							QList<QBuilder::Command*> cmd = m_commands;
							
							m_commands << c->depends();
							depcount = m_commands.count() - depcount;
							
							for ( int i = 0; i < depcount; ++i )
								m_modes << mode;
							
							while ( (idx = m_commands.indexOf(0)) != -1 )
							{
								m_modes.removeAt(idx);
								m_commands.removeAt(idx);
								
								foreach ( QBuilder::Command *cc, cmd )
								{
									m_commands.insert(idx, cc);
									m_modes.insert(idx, m_engine->selectedMode(cc));
								}
							}
						}
						
						//qDebug("\t %p [final]", c);
						break;
					} else {
						QBuilder::Command *cmd = b->commands().at(0);
						
						//qDebug("\t %p [transition]", cmd);
						
						m_commands << cmd;
						m_modes << m_engine->selectedMode(cmd);
					}
				}
			}
			
			//qDebug() << "Modes : " << m_modes << endl;
			
			QMetaObject::invokeMethod(m_engine, "switchToolbar", Q_ARG(bool, true));
		}
		
		virtual ~QBuildTask()
		{
			abort();
			
			delete m_progress;
			m_progress = 0;
		}
		
	public slots:
		void abort()
		{
			m_abort = true;
			
			m_commands.clear();
			
			if ( m_process )
			{
				// flush remaining log
				processReadyRead();
				
				if ( m_buffer.count() )
					sendLog(m_buffer);
				
				sendLog(QString());
				sendLog(tr("-- Task %1 --").arg(tr("aborted by user")));
				
				QProcess *p = m_process;
				m_process = 0;
				
				p->kill();
				p->waitForFinished();
				p->deleteLater();
			}
			
			exit(-1);
		}
		
		virtual void finished(int reason)
		{
			if ( m_scan )
			{
				//qDebug("requesting target list update");
				emit targetListUpdateRequested();
				m_scan = false;
			}
			
			if ( !m_process )
				return;
			
			#ifdef _DEBUG_
			//qDebug("task finished : %i", reason);
			#endif
			
			// flush remaining log
			processReadyRead();
			
			if ( m_buffer.count() )
				sendLog(m_buffer);
			
			sendLog(QString());
			sendLog(tr("-- Task %1 --")
					.arg(
						reason
					?
						tr("ended due to error(s)")
					:
						tr("finished with no errors")
					)
				);
			
			if ( m_process )
			{
				m_process->deleteLater();
				m_process = 0;
			}
			
			// stop event loop
			exit(reason);
		}
		
		virtual void setProgress(int percent)
		{
			Q_UNUSED(percent)
		}
		
		virtual void setCurrentOperation(const QString& op)
		{
			Q_UNUSED(op)
		}
		
		virtual void sendLog(const QString& log)
		{
			emit m_engine->log(log);
		}
		
		virtual void sendMessage(const QString& file, int line, const QString& message)
		{
			if ( QFileInfo(file).isAbsolute() || !m_process )
				emit m_engine->message(file, line, message);
			else
				emit m_engine->message(QDir(m_process->workingDirectory()).absoluteFilePath(file), line, message);
		}
		
	signals:
		void filesChanged(const QStringList& l);
		
		void targetListUpdateRequested();
		void commandFailed(QBuilder::Command *cmd, int error);
		//void message(const QString& file, int line, const QString& msg);
		
	protected:
		virtual void run()
		{
			QTimer::singleShot(100, this, SLOT( step() ));
			
			sendLog(tr("-- Task %1 --").arg(tr("started")));
			
			exec();
			
			if ( m_process )
			{
				m_process->deleteLater();
				m_process = 0;
			}
			
			// re-enable build functions
			if ( m_engine )
			{
				QMetaObject::invokeMethod(m_engine, "switchToolbar", Q_ARG(bool, false));
			}
		}
		
	private slots:
		void step()
		{
			if ( m_abort )
				return;
			
			#ifdef _DEBUG_
			if ( !isRunning() )
				qDebug("synchronous...");
			#endif
			
			if ( m_index >= m_commands.count() )
				return finished(-1);
			
			QBuilder::Command *c = m_commands.at(m_index);
			m_parser = c->outputParser();
			
			#ifdef _DEBUG_
			//qDebug("step %i : %s", m_index, qPrintable(c->label()));
			#endif
			
			sendLog(QString());
			sendLog(tr(">> Step %1 : %2 <<")
						.arg(QString::number(m_index))
						.arg(c->label().remove('&'))
					);
			
			m_modified.clear();
			m_scan |= c->mayAffectTargetList();
			
			QBuilder::Command::Info ci = c->info(m_input, m_modes.at(m_index));
			
			if ( ci.exec.isEmpty() )
			{
				// handling of meta commands (i.e. grouping via deps)
				sendLog("[meta command]");
				
				if ( ++m_index < m_commands.count() )
					step();
				else
					finished(0);
				
				return;
			}
			
			QHash<QString, QString> macros;
			macros["source"] = m_source;
			macros["mode"] = m_mode;
			macros["input"] = m_input;
			
			for ( int i = 0; i < ci.arguments.count(); ++i )
			{
				EdyukTemplateManager::macro_substitution(ci.arguments[i], macros);
			}
			
			sendLog(tr("%1 $ %2 \"%3\"")
					.arg(QFileInfo(m_engine->m_activeSource).path())
					.arg(ci.exec)
					.arg(ci.arguments.join("\" \""))
					);
			
			sendLog(QString());
			
			#ifdef _DEBUG_
			//qDebug("$ %s \"%s\"",
			//		qPrintable(ci.exec),
			//		qPrintable(ci.arguments.join("\" \""))
			//		);
			#endif
			
			if ( !m_process )
			{
				m_process = new QProcess;
				m_process->setProcessChannelMode(QProcess::MergedChannels);
				m_process->setWorkingDirectory(QFileInfo(m_engine->m_activeSource).path());
				
				#ifdef _DEBUG_
				//qDebug("PWD : %s", qPrintable(m_process->workingDirectory()));
				#endif
				
				#ifndef Q_OS_WIN_32
				// adjust path settings to ensure shared libs in the same dir as exec are found
				
				QString dpv =
				#ifdef Q_OS_MAC
					"DYLD_LIBRARY_PATH="
				#else
					"LD_LIBRARY_PATH="
				#endif
					;
				
				QStringList env = QProcess::systemEnvironment() + m_process->environment();
				QStringList::iterator it = env.begin();
				
				while ( it != env.end() )
				{
					if ( it->startsWith(dpv) )
					{
						it->insert(dpv.count(), m_process->workingDirectory() + ":");
						break;
					}
					
					++it;
				}
				
				if ( it == env.end() )
				{
					env << (dpv + m_process->workingDirectory());
				}
				
				#ifdef _DEBUG_
				//qDebug("%s", qPrintable(env.join("\n")));
				#endif
				
				m_process->setEnvironment(env);
				#endif
				
				connect(m_process	, SIGNAL( readyRead() ),
						this		, SLOT  ( processReadyRead() ) );
				
				connect(m_process	, SIGNAL( error(QProcess::ProcessError) ),
						this		, SLOT  ( processError(QProcess::ProcessError) ) );
				
				connect(m_process	, SIGNAL( finished(int, QProcess::ExitStatus) ),
						this		, SLOT  ( processFinished(int, QProcess::ExitStatus) ) );
				
			}
			
			++m_index;
			
			m_input = ci.output;
			
			m_process->start(ci.exec, ci.arguments);
			
			//if ( !m_process->waitForStarted() )
			//	finished
		}
		
		void processError(QProcess::ProcessError error)
		{
			#ifdef _DEBUG_
			qDebug("process error(%i)", error);
			#endif
			
			if ( m_abort )
				return;
			
			if ( (m_index > 0) && ((m_index - 1) < m_commands.count()) )
				emit commandFailed(m_commands.at(m_index - 1), error);
			
			finished(-1);
		}
		
		void processFinished(int exitCode, QProcess::ExitStatus exitStatus)
		{
			#ifdef _DEBUG_
			//qDebug("process finished(%i, %i)", exitCode, exitStatus);
			#endif
			
			if ( m_modified.count() )
			{
				//qDebug("%i modified files : {%s}", m_modified.count(), qPrintable(m_modified.join(", ")));
				
				emit filesChanged(m_modified);
				
				if ( m_scan )
				{
					//qDebug("requesting target list update");
					emit targetListUpdateRequested();
					m_scan = false;
				}
			}
			
			if ( exitCode || exitStatus )
			{
				finished(-1);
			} else if ( m_index >= m_commands.count() ) {
				finished(0);
			} else {
				step();
			}
		}
		
		void processReadyRead()
		{
			if ( !m_process )
				return;
			
			QByteArray ba = m_process->readAll();
			m_buffer += QString::fromLocal8Bit(ba);
			QList<QByteArray> bal = ba.split('\n');
			
			int idx = m_buffer.lastIndexOf('\n'), a = 0, b;
			
			if ( idx != -1 )
			{
				QStringList lines = m_buffer.left(idx).split('\n');
				m_buffer = m_buffer.mid(idx + 1);
				
				idx = 0;
				QBuilder::ParsedLine line;
				
				foreach ( const QString& l, lines )
				{
					//qDebug("%s", bal.at(idx++).constData());
					
					if ( m_parser )
					{
						m_modified << m_parser->parse(l, line);
						
						if ( line.isValid() )
						{
							//qDebug("message : %s", qPrintable(line.message));
							sendMessage(line.file, line.line, line.message);
						}
					}
					
					sendLog(l);
				}
			}
		}
		
	private:
		int m_index;
		bool m_abort, m_scan;
		QProcess *m_process;
		QBuildEngine *m_engine;
		QStringList m_modified;
		QString m_buffer, m_mode, m_source, m_input;
		QProgressDialog *m_progress;
		QBuilder::CommandParser *m_parser;
		QList<QBuilder::Command*> m_commands;
		QStringList m_modes;
};

/*!
	\class QBuildEngine
	\brief A convenient way of managing build process
	
	QBuildEngine manages a set of QBuilder objects and uses them
	to build targets. The beauty of it is that every QBuilder
	object is completely independent from the other but, with
	simple computations, QBuildEngine is able to craft a chain
	of builders that go from the target to its ouptut.
*/

/*!

*/
QBuildEngine* QBuildEngine::instance()
{
	static QBuildEngine m_globalShared;
	
	return &m_globalShared;
}

/*!
	\brief ctor
*/
QBuildEngine::QBuildEngine(QObject *p)
 : QObject(p)
{
	m_abortTask = new QAction(QIcon(":/buildstop.png"), tr("&Abort"), this);
	m_abortTask->setEnabled(false);
	
	EDYUK_SHORTCUT(m_abortTask, "Build", "");
	
	connect(m_abortTask	, SIGNAL( triggered() ),
			this		, SLOT  ( abort() ) );
	
	m_execTarget = new QAction(QIcon(":/exec.png"), tr("&Run"), this);
	
	EDYUK_SHORTCUT(m_execTarget, "Build", "");
	
	connect(m_execTarget, SIGNAL( triggered() ),
			this		, SLOT  ( run() ) );
	
	m_actionGroup = new QActionGroup(this);
	
	connect(m_actionGroup	, SIGNAL( triggered(QAction*) ),
			this			, SLOT  ( actionTriggered(QAction*) ) );
	
	m_targetMenu = new QMenu(tr("Exec target"));
	
	m_targetGroup = new QActionGroup(this);
	m_targetGroup->setExclusive(true);
	
	connect(m_targetGroup	, SIGNAL( triggered(QAction*) ),
			this			, SLOT  ( execTargetChanged(QAction*) ) );
	
}

/*!
	\brief dtor
*/
QBuildEngine::~QBuildEngine()
{
	delete m_targetMenu;
}

/*!

*/
void QBuildEngine::retranslate()
{
	//clearActions();
	
	menus.setTranslation("&Build", tr("&Build"));
	toolbars.setTranslation("Build", tr("Build"));
	
	//updateActions();
	
	m_abortTask->setText(tr("&Abort"));
	m_targetMenu->setWindowTitle(tr("Exec target"));
	
	COMPONENT(shortcutManager)->translateContext("Build", tr("Build"));
	
	foreach ( QBuilder *b, m_builders )
		COMPONENT(shortcutManager)->translateContext(QString("Build/%1").arg(b->name()), tr("Build/%1").arg(b->label()));
	
	QHash<QAction*, QBuilder::Command*>::const_iterator it = m_commands.constBegin();
	
	while ( it != m_commands.constEnd() )
	{
		it.key()->setText((*it)->label());
		
		++it;
	}
}

/*!
	\return whether a task is running
*/
bool QBuildEngine::taskRunning() const
{
	return m_currentTask;
}

/*!
	\brief abort the current build task, if any
*/
void QBuildEngine::abort()
{
	if ( m_currentTask )
	{
		m_currentTask->abort();
		m_currentTask->deleteLater();
		m_currentTask = 0;
		
		emit taskFinished();
	}
}

/*!
	\brief run the current exec target task, if any
*/
void QBuildEngine::run()
{
	QString exe = activeTarget();
	
	if ( exe.isEmpty() )
	{
		QMessageBox::warning(0, tr("Unable to run target"), tr("No available target."));
		return;
	}
	
	if ( !QFile::exists(exe) || !QFileInfo(exe).isExecutable() )
	{
		QMessageBox::warning(0,
						tr("Unable to run target"),
						tr("The selected execution target is invalid.\nMost likely it has not been build properly yet.")
					);
		
		return;
	}
	
	QString arg = QInputDialog::getText(0, tr("Enter arguments"), tr("Arguments :"));
	
	QProcess::startDetached(exe, Edyuk::splitArguments(arg), QFileInfo(m_activeSource).absolutePath());
}

/*!
	\return the currently active build source
	
	The build source is the file from which build process starts (usually a project)
*/
QString QBuildEngine::activeSource() const
{
	return m_activeSource;
}

/*!
	\return the currently active exec target
	
	
*/
QString QBuildEngine::activeTarget() const
{
	if ( m_availableTargets.isEmpty() )
		return QString();
	
	QAction *a = m_targetGroup->checkedAction();
	
	return a ? a->text() : m_availableTargets.at(0);
}

/*!
	\brief Add a builder to the internal collection of the build engine.
*/
void QBuildEngine::addBuilder(QBuilder *b)
{
	if ( !b || b->commands().isEmpty() )
		return;
	
	//qDebug("adding builder %p", b);
	
	m_builders << b;
	
	QList<QBuilder::Command*> commands = b->commands();
	
	foreach ( QBuilder::Command *cmd, commands )
	{
		QAction *a = new QAction(cmd->icon(), cmd->label(), this);
		
		m_actionGroup->addAction(a);
		m_commands[a] = cmd;
		m_origins[cmd] = b;
	}
	
	COMPONENT(shortcutManager)->translateContext(QString("Build/%1").arg(b->name()), tr("Build/%1").arg(b->label()));
	
	// compute new pipelines
	
	QList<QBuildChain> terminal;
	
	foreach ( QBuilder *tb, m_builders )
	{
		if ( tb->outputType() == "exec" )
			terminal << tb;
	}
	
	while ( terminal.count() )
	{
		QBuildChain bc = terminal.takeFirst();
		
		QString in = bc.first()->inputType();
		
		m_pipelines[in] = bc;
		
		//qDebug("pipeline : %s -> %s", qPrintable(in), qPrintable(bc.last()->outputType()));
		
		foreach ( QBuilder *tb, m_builders )
		{
			if ( tb->outputType() == in )
			{
				bc.prepend(tb);
				terminal << bc;
				bc.removeFirst();
			}
		}
	}
}

/*!

*/
void QBuildEngine::setActiveSource(const QString& source, const QString& backend)
{
	if ( m_activeSource == source )
		return;
	
	if ( m_activeSource.count() )
	{
		Remanence r;
		QStringList l;
		
		foreach ( QMenu *m, m_modesMenus )
		{
			if ( m )
			{
				QList<QAction*> al = m->actions();
				
				if ( al.count() )
					l << al.at(0)->actionGroup()->checkedAction()->text();
				else
					l << QString();
				
			} else {
				l << QString();
			}
		}
		
		r.target = activeTarget();
		r.modes = l;
		
		m_last[m_activeSource] = r;
	}
	
	emit mergingRequested(this, false);
	
	menus.clear();
	toolbars.clear();
	
	m_availableTargets.clear();
	m_possibleOutputs.clear();
	
	m_targetMenu->clear();
	QList<QAction*> la = m_targetGroup->actions();
	
	foreach ( QAction *a, la )
	{
		m_targetGroup->removeAction(a);
		delete a;
	}
	
	foreach ( QMenu *m, m_modesMenus )
		delete m;
	
	m_modesMenus.clear();
	
	m_activeSource = source;
	m_activePipeline = backend;
	
	Remanence rm = m_last.value(m_activeSource);
	QStringList prevmodes = rm.modes;
	
	QBuildChain c = m_pipelines.value(backend);
	
	if ( c.count() )
	{
		menus["&Build"]->addAction(m_abortTask);
		menus["&Build"]->addSeparator();
		menus["&Build"]->addAction(m_execTarget);
		menus["&Build"]->addSeparator();
		
		toolbars["Build"]->addAction(m_abortTask);
		toolbars["Build"]->addSeparator();
		toolbars["Build"]->addAction(m_execTarget);
		toolbars["Build"]->addSeparator();
		
		//qDebug("builder found for %s [%i steps]", qPrintable(backend), c.count());
		
		// update menu
		QStringList modes;
		QStringList in, out;
		
		out << source;
		
		int off = 1;
		
		foreach ( QBuilder *b, c )
		{
			QList<QBuilder::Command*> commands = b->commands();
			
			bool notLast = b != c.last();
			
			if ( notLast )
				commands.removeAt(0);
			
			foreach ( QBuilder::Command *cmd, commands )
			{
				QAction *a = m_commands.key(cmd);
				
				if ( !a )
					continue;
				
				menus["&Build"]->addAction(a);
				toolbars["Build"]->addAction(a);
			}
			
			if ( commands.count() && notLast )
			{
				menus["&Build"]->addSeparator();
				toolbars["Build"]->addSeparator();
			}
			
			in = out;
			out.clear();
			modes.clear();
			
			foreach ( QString i, in )
			{
				QBuilder::Output o = b->output(i);
				
				m_possibleOutputs << o;
				
				foreach ( QStringList tl, o.targets )
				{
					modes << tl.at(0);
					tl.removeAt(0);
					out << tl;
				}
			}
			
			if ( modes.isEmpty() )
			{
				m_modesMenus << 0;
				continue;
			} else {
				QMenu *m = new QMenu(tr("Mode for %1").arg(b->label()));
				QActionGroup *ag = new QActionGroup(m);
				ag->setExclusive(true);
				
				QString pm;
				
				if ( prevmodes.count() > m_modesMenus.count() )
					pm = prevmodes.at(m_modesMenus.count());
				
				foreach ( QString mode, modes )
				{
					QAction *a = new QAction(mode, m);
					a->setCheckable(true);
					ag->addAction(a);
					m->addAction(a);
					
					if ( pm == mode )
						a->setChecked(true);
				}
				
				if ( !ag->checkedAction() )
					ag->actions().first()->setChecked(true);
				
				if ( modes.count() > 1 )
					menus["&Build"]->insertAction(m->menuAction(), off);
				
				m_modesMenus << m;
				++off;
			}
		}
		
		//qDebug("%i modes", off - 1);
		
		foreach ( QString t, out )
			if ( !m_availableTargets.contains(t) )
				m_availableTargets << t;
		
		//qDebug("%i targets", m_availableTargets.count());
		
		if ( m_availableTargets.count() > 1 )
		{
			foreach ( QString t, m_availableTargets )
			{
				QAction *a = new QAction(t, this);
				a->setCheckable(true);
				
				m_targetGroup->addAction(a);
				m_targetMenu->addAction(a);
				
				if ( t == rm.target )
					a->setChecked(true);
				
			}
			
			if ( !m_targetGroup->checkedAction() )
				m_targetGroup->actions().first()->setChecked(true);
			
			menus["&Build"]->insertAction(m_targetMenu->menuAction(), off);
		}
		
		emit mergingRequested(this, true);
		emitTargetListUpdateRequested();
	} else {
		qDebug("builder not found for %s", qPrintable(backend));
	}
}

/*!
	\internal
*/
void QBuildEngine::modeChanged(QAction *a)
{
	
}

/*!
	\internal
*/
void QBuildEngine::execTargetChanged(QAction *a)
{
	//qDebug("changing exec target to %s [%s]", qPrintable(activeTarget()), qPrintable(a->text()));
	
	emit execTargetChanged(activeTarget());
}

/*!
	\internal
*/
void QBuildEngine::emitTargetListUpdateRequested()
{
	// first update target list then change actions and finally emit signal
	QBuildChain c = m_pipelines.value(m_activePipeline);
	
	if ( c.count() )
	{
		QStringList in, out;
		out << m_activeSource;
		
		foreach ( QBuilder *b, c )
		{
			in = out;
			out.clear();
			
			foreach ( QString i, in )
			{
				QBuilder::Output o = b->output(i);
				
				m_possibleOutputs << o;
				
				foreach ( QStringList tl, o.targets )
				{
					tl.removeAt(0);
					out << tl;
				}
			}
		}
		
		// at this point we have the new list of available targets
		// let's DELTA this against the old one
		
		QList<QAction*> act = m_targetGroup->actions();
		
		foreach ( QString old, m_availableTargets )
		{
			if ( !out.contains(old) )
			{
				// target disappeared, let's blow!
				
				for ( int i = 0; i < act.count(); ++i )
				{
					QAction *a = act.at(i);
					
					if ( a->text() == old )
					{
						m_targetGroup->removeAction(a);
						m_targetMenu->removeAction(a);
						
						act.removeAt(i);
						delete a;
						break;
					}
				}
			}
		}
		
		if ( out.count() > 1 )
		{
			foreach ( QString t, out )
			{
				if ( !m_availableTargets.contains(t) )
				{
					// target created, let's rock!
					m_availableTargets << t;
					
					QAction *a = new QAction(t, this);
					a->setCheckable(true);
					
					m_targetGroup->addAction(a);
					m_targetMenu->addAction(a);
					
					//if ( t == rm.target )
					//	a->setChecked(true);
				}
			}
		}
		
		m_availableTargets = out;
	} else {
		qDebug("no way to update target list");
		return;
	}
	
	// TODO : update active exec target according to existing ones
	QList<QAction*> l = m_targetGroup->actions();
	
	QAction *a = m_targetGroup->checkedAction();
	
	if ( a )
	{
		QString s = a->text();
		
		if ( !QFile::exists(s) || !QFileInfo(s).isExecutable() )
		{
			// select the first existing target, if any
			//qDebug("selected target invalid trying to find another...");
			
			foreach ( QAction *b, l )
			{
				s = b->text();
				
				if ( QFile::exists(s) && QFileInfo(s).isExecutable() )
				{
					b->setChecked(true);
					break;
				}
			}
		}
	}
	
	emit targetListUpdateRequested();
}

/*!
	\internal
*/
void QBuildEngine::actionTriggered(QAction *a)
{
	if ( !a || !m_commands.contains(a) )
		return;
	
	QBuilder::Command *c = m_commands[a];
	
	emit taskAboutToStart();
	
	//qDebug("executing command : %s [%s]", qPrintable(a->text()), qPrintable(c->label()));
	
	QProgressDialog *dlg = new QProgressDialog;
	dlg->setWindowTitle(tr("Build task report"));
	
	QString m; // = m_activeModes.count() ? m_activeModes.last() : QString();
	
	m_currentTask = new QBuildTask(c, m, this, dlg);
	
	connect(m_currentTask	, SIGNAL( started() ),
			this			, SIGNAL( taskStarted() ) );
	
	connect(m_currentTask	, SIGNAL( finished() ),
			this			, SIGNAL( taskFinished() ) );
	
	connect(m_currentTask	, SIGNAL( finished() ),
			m_currentTask	, SLOT  ( deleteLater() ) );
	
	connect(m_currentTask	, SIGNAL( filesChanged(QStringList) ),
			this			, SIGNAL( filesChanged(QStringList) ) );
	
	connect(m_currentTask	, SIGNAL( targetListUpdateRequested() ),
			this			, SLOT  ( emitTargetListUpdateRequested() ) );
	
	connect(m_currentTask	, SIGNAL( commandFailed(QBuilder::Command*, int) ),
			this			, SLOT  ( commandFailed(QBuilder::Command*, int) ) );
	
	m_currentTask->start();
}

/*!
	\internal
*/
QString QBuildEngine::selectedMode(QBuilder::Command *c) const
{
	QHash<QBuilder::Command*, QBuilder*>::const_iterator it = m_origins.constFind(c);
	
	if ( it == m_origins.constEnd() )
	{
		//qDebug("unregistered command : %s", qPrintable(c->label()));
		
		return QString();
	}
	
	QBuilder *b = *it;
	QBuildChain bc = m_pipelines.value(m_activePipeline);
	
	int idx = bc.indexOf(b);
	
	if ( idx != -1 && m_modesMenus.at(idx) )
	{
		QList<QAction*> la = m_modesMenus.at(idx)->actions();
		
		if ( la.count() )
		{
			QString m = la.at(0)->actionGroup()->checkedAction()->text();
			
			//qDebug("mode for %s => %s", qPrintable(c->label()), qPrintable(m));
			
			return m;
		}
	}
	
	//qDebug("mode for %s [%i], not found", qPrintable(c->label()), idx);
	
	return QString();
}

/*!
	\internal
*/
void QBuildEngine::switchToolbar(bool enabled)
{
	m_abortTask->setEnabled(enabled);
	m_actionGroup->setEnabled(!enabled);
}

/*!
	\internal
*/
void QBuildEngine::commandFailed(QBuilder::Command *cmd, int error)
{
	QMessageBox::warning(0,
						tr("Build engine error"),
						tr(
							"The \"%1\" command %2...\n"
							"You are advised to check project and/or plugin settings."
						)
						.arg(cmd->label().replace('&', ""))
						.arg(error ? tr("crashed") : tr("failed to start"))
						);
	
}

#include "qbuildengine.moc"

