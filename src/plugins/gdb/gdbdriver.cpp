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

#include "gdbdriver.h"

#include "plugin.h"
#include "qdebugginginteractionproxy.h"

#include "gdbdriverui.h"
#include "gdbdriverthread.h"

#include <QAction>
#include <QFileInfo>
#include <QScrollBar>
#include <QTreeWidget>
#include <QActionGroup>
#include <QInputDialog>
#include <QTreeWidgetItem>

class GDBDriverObject : public QObject
{
	Q_OBJECT
	
	public:
		GDBDriverObject(GDBDriver *driver)
		 : QObject(0), d(driver)
		{
			
		}
		
		bool event(QEvent *e)
		{
			switch ( e->type() )
			{
				case Edyuk::RunTimeTranslation :
					
					break;
					
				default:
					break;
			}
			
			return QObject::event(e);
		}
		
	private slots:
		void start()
		{ d->start(); }
		
		void started()
		{ d->started(); }
		
		void customCommand()
		{ d->customCommand(); }
		
		void toggleBreakpoint()
		{ d->toggleBreakpoint(); }
		
		void runToCurrentLine()
		{
			int line;
			QString file;
			
			d->getCurrentLocation(file, line);
			
			d->command(QString("-exec-until %1:%2").arg(file).arg(line));
		}
		
		void command(QAction *a)
		{ d->command(a); }
		
		void stateChanged(int state)
		{ d->stateChanged(state); }
		
		void forwardLog(const QString& l)
		{ d->forwardLog(l); }
		
		void location(const QString& fn, int line)
		{ d->location(fn, line); }
		
		void result(RecordNode *root, int type)
		{ d->processResult(root, type); }
		
		void setVisualBreakpoint(const QString& file, int line, bool on)
		{ d->setVisualBreakpoint(file, line, on); }
		
		void itemActivated(QTreeWidgetItem *i, int c)
		{
			if ( !i )
				return;
			
			d->setLocation(i->text(2), i->text(3).toInt() - 1, false);
		}
		
	private:
		GDBDriver *d;
};

GDBDriver::GDBDriver()
{
	o = new GDBDriverObject(this);
	
	d = new GDBDriverThread;
	
	ui = new GDBDriverUi(this);
	d->setVariableUpdateHandler(ui);
	d->setRegisterUpdateHandler(ui);
	
	ui->connect(d, SIGNAL( autoUpdateTick() ),
				SLOT( autoUpdateTick() ) );
	
	QDebuggingInteractionProxy *proxy = interactionProxy();
	
	o->connect(d, SIGNAL( started() ),
				SLOT( started() ) );
	
	o->connect(d, SIGNAL( log(QString) ),
				SLOT( forwardLog(QString) ) );
	
	proxy->connect(d, SIGNAL( error(QString) ),
					SLOT( error(QString) ) );
	proxy->connect(d, SIGNAL( warning(QString) ),
					SLOT( warning(QString) ) );
	proxy->connect(d, SIGNAL( question(QString) ),
					SLOT( question(QString) ) );
	proxy->connect(d, SIGNAL( information(QString) ),
					SLOT( information(QString) ) );
	
	proxy->connect(d, SIGNAL( error(QString, MessageCallback*, int, int) ),
					SLOT( error(QString, MessageCallback*, int, int) ),
					Qt::QueuedConnection);
	proxy->connect(d, SIGNAL( warning(QString, MessageCallback*, int, int) ),
					SLOT( warning(QString, MessageCallback*, int, int) ) );
	proxy->connect(d, SIGNAL( question(QString, MessageCallback*, int, int) ),
					SLOT( question(QString, MessageCallback*, int, int) ) );
	proxy->connect(d, SIGNAL( information(QString, MessageCallback*, int, int) ),
					SLOT( information(QString, MessageCallback*, int, int) ) );
	
	o->connect(d, SIGNAL( location(QString, int) ),
				SLOT( location(QString, int) ) );
	
	o->connect(d, SIGNAL( stateChanged(int) ),
				SLOT( stateChanged(int) ) );
	
	o->connect(d, SIGNAL( result(RecordNode*, int) ),
				SLOT( result(RecordNode*, int) ) );
	
	o->connect(d, SIGNAL( setVisualBreakpoint(QString, int, bool) ),
				SLOT( setVisualBreakpoint(QString, int, bool) ) );
	
	ui->connect(d, SIGNAL( breakpointsChanged() ),
				SLOT( updateBreakpointTable() ) );
	
	m_group = new QActionGroup(o);
	
	m_start = new QAction(QIcon(":/debug-run.png"), GDBDriverObject::tr("&Start"), o);
	m_start->setShortcut(QKeySequence("SHIFT+ALT+S"));
	m_start->setData("start");
	
	m_stop = new QAction(QIcon(":/debug-stop.png"), GDBDriverObject::tr("S&top"), o);
	m_stop->setShortcut(QKeySequence("SHIFT+ALT+Q"));
	m_stop->setData("-gdb-exit");
	
	m_break = new QAction(QIcon(":/breakpoint.png"), GDBDriverObject::tr("Toggle &breakpoint"), o);
	m_break->setShortcut(QKeySequence("SHIFT+ALT+B"));
	m_break->setData("");
	
	m_continue = new QAction(QIcon(":/debug-resume.png"), GDBDriverObject::tr("&Continue"), o);
	m_continue->setShortcut(QKeySequence("SHIFT+ALT+C"));
	m_continue->setData("-exec-continue");
	
	m_until = new QAction(QIcon(""), GDBDriverObject::tr("Until loop end"), o);
	m_until->setData("-exec-until");
	
	m_runto = new QAction(QIcon(":/debug-runto.png"), GDBDriverObject::tr("Run to current line"), o);
	m_runto->setData("");
	
	m_finish = new QAction(QIcon(":/debug-stepout.png"), GDBDriverObject::tr("Step &out"), o);
	m_finish->setShortcut(QKeySequence("SHIFT+ALT+O"));
	m_finish->setData("-exec-finish");
	
	m_step = new QAction(QIcon(":/debug-stepin.png"), GDBDriverObject::tr("Step &in"), o);
	m_step->setShortcut(QKeySequence("SHIFT+ALT+I"));
	m_step->setData("-exec-step");
	
	m_stepi = new QAction(QIcon(":/debug-step-inst.png"), GDBDriverObject::tr("Step in instruction"), o);
	m_stepi->setData("-exec-step-instruction");
	
	m_next = new QAction(QIcon(":/debug-next.png"), GDBDriverObject::tr("&Next line"), o);
	m_next->setShortcut(QKeySequence("SHIFT+ALT+N"));
	m_next->setData("-exec-next");
	
	m_nexti = new QAction(QIcon(":/debug-next-inst.png"), GDBDriverObject::tr("Next instruction"), o);
	m_nexti->setData("-exec-next-instruction");
	
	m_backtrace = new QAction(GDBDriverObject::tr("Show backtrace"), o);
	m_backtrace->setData("-stack-list-frames");
	
	m_customCommand = new QAction(QIcon(":/debugger.png"), GDBDriverObject::tr("&Custom command"), o);
	m_customCommand->setData("");
	
	m_group->addAction(m_stop);
	m_group->addAction(m_continue);
	m_group->addAction(m_until);
	m_group->addAction(m_finish);
	m_group->addAction(m_next);
	m_group->addAction(m_nexti);
	m_group->addAction(m_step);
	m_group->addAction(m_stepi);
	m_group->addAction(m_backtrace);
	
	m_stop->setEnabled(false);
	m_continue->setEnabled(false);
	m_finish->setEnabled(false);
	m_step->setEnabled(false);
	m_stepi->setEnabled(false);
	m_next->setEnabled(false);
	m_nexti->setEnabled(false);
	m_until->setEnabled(false);
	m_runto->setEnabled(false);
	m_backtrace->setEnabled(false);
	m_customCommand->setEnabled(false);
	
	QObject::connect(	m_start	, SIGNAL( triggered() ),
						o		, SLOT  ( start() ) );
	
	QObject::connect(	m_break	, SIGNAL( triggered() ),
						o		, SLOT  ( toggleBreakpoint() ) );
	
	QObject::connect(	m_runto	, SIGNAL( triggered() ),
						o		, SLOT  ( runToCurrentLine() ) );
	
	QObject::connect(	m_group	, SIGNAL( triggered(QAction*) ),
						o		, SLOT  ( command(QAction*) ) );
	
	QObject::connect(	m_customCommand	, SIGNAL( triggered() ),
						o				, SLOT  ( customCommand() ) );
	
	menus["&Debug"]->addAction(m_start);
	menus["&Debug"]->addAction(m_stop);
	menus["&Debug"]->addSeparator();
	menus["&Debug"]->addAction(m_break);
	menus["&Debug"]->addSeparator();
	menus["&Debug"]->addAction(m_continue);
	menus["&Debug"]->addAction(m_until);
	menus["&Debug"]->addAction(m_next);
	menus["&Debug"]->addAction(m_step);
	menus["&Debug"]->addAction(m_finish);
	menus["&Debug"]->addAction(m_runto);
	menus["&Debug"]->addSeparator();
	menus["&Debug"]->addAction(m_nexti);
	menus["&Debug"]->addAction(m_stepi);
	menus["&Debug"]->addSeparator();
	menus["&Debug"]->addAction(m_backtrace);
	menus["&Debug"]->addSeparator();
	menus["&Debug"]->addAction(m_customCommand);
	
	toolbars["Debug"]->addAction(m_start);
	toolbars["Debug"]->addAction(m_stop);
	toolbars["Debug"]->addSeparator();
	toolbars["Debug"]->addAction(m_continue);
	toolbars["Debug"]->addAction(m_until);
	toolbars["Debug"]->addAction(m_runto);
	toolbars["Debug"]->addAction(m_next);
	toolbars["Debug"]->addAction(m_step);
	toolbars["Debug"]->addAction(m_finish);
	toolbars["Debug"]->addSeparator();
	toolbars["Debug"]->addAction(m_break);
	
	menus.setTranslation("&Debug", GDBDriverObject::tr("&Debug"));
	toolbars.setTranslation("Debug", GDBDriverObject::tr("Debug"));
}

GDBDriver::~GDBDriver()
{
	if ( ui )
		delete ui;
	
	d->quit();
	delete d;
	delete o;
}

void GDBDriver::retranslate()
{
	m_start->setText(GDBDriverObject::tr("&Start"));
	m_stop->setText(GDBDriverObject::tr("S&top"));
	m_break->setText(GDBDriverObject::tr("Toggle &breakpoint"));
	m_continue->setText(GDBDriverObject::tr("&Continue"));
	m_until->setText(GDBDriverObject::tr("Until loop end"));
	m_runto->setText(GDBDriverObject::tr("Run to current line"));
	m_finish->setText(GDBDriverObject::tr("Step &out"));
	m_step->setText(GDBDriverObject::tr("Step &in"));
	m_stepi->setText(GDBDriverObject::tr("Step in instruction"));
	m_next->setText(GDBDriverObject::tr("&Next line"));
	m_nexti->setText(GDBDriverObject::tr("Next instruction"));
	m_backtrace->setText(GDBDriverObject::tr("Show backtrace"));
	m_customCommand->setText(GDBDriverObject::tr("&Custom command"));
	
	menus.setTranslation("&Debug", GDBDriverObject::tr("&Debug"));
	toolbars.setTranslation("Debug", GDBDriverObject::tr("Debug"));
	
	ui->retranslate();
}

QString GDBDriver::name() const
{
	return "GDB";
}

QString GDBDriver::label() const
{
	return GDBDriverObject::tr("GDB");
}

void GDBDriver::terminate()
{
	while ( d->isRunning() )
	{
		QMetaObject::invokeMethod(d, "_killer");
	}
}

bool GDBDriver::isRunning() const
{
	return d->isRunning();
}

QWidget* GDBDriver::customDock() const
{
	return ui;
}

QList<Breakpoint>& GDBDriver::breakpoints()
{
	return d->breakpoints();
}

const QList<Breakpoint>& GDBDriver::breakpoints() const
{
	return d->breakpoints();
}

void GDBDriver::setBreakpoint(const QString& filename, int line, bool on)
{
	QString fn(filename);
	//fn.replace("\"", "\\\"");
	
	if ( fn.contains(' ') )
		fn = d->relativePath(fn);
	
	if ( on && d->isRunning() )
	{
		d->command(QString("-break-insert %1:%2").arg(fn).arg(QString::number(line)));
	} else {
		d->removeBreakpoint(fn, line);
	}
}

void GDBDriver::toggleBreakpoint()
{
	toggleBreakpointOnCurrentLine();
}

bool GDBDriver::isSupportedInput(const QString& file, const QString& language) const
{
	QFileInfo inf(file);
	
	if ( inf.isExecutable() )
	{
		//d->setTarget(file);
		return true;
	}
	
	return false;
}

void GDBDriver::start()
{
	if ( d->isRunning() )
	{
		//d->command("-gdb-exit");
	} else {
		
		QString s = QInputDialog::getText(0, GDBDriverObject::tr("Enter arguments"), GDBDriverObject::tr("Arguments :"));
		
		d->setTarget(target());
		d->setSource(source());
		d->command(QString("-exec-arguments %1").arg(s));
		d->start();
	}
}

void GDBDriver::customCommand()
{
	QString cmd = QInputDialog::getText(0, GDBDriverObject::tr("GDB driver : custom command"), GDBDriverObject::tr("Command (GDB/MI syntax)"));
	
	d->command(cmd);
}

void GDBDriver::command(QAction *a)
{
	d->command(a->data().toString());
}

void GDBDriver::forwardLog(const QString& l)
{
	sendLog(l);
}

void GDBDriver::location(const QString& fn, int l)
{
	setLocation(fn, l);
}

void GDBDriver::command(const QString& cmd, GDBResultHandler *h)
{
	d->command(cmd, h);
}

void GDBDriver::command(const QString& cmd, const QStringList& params, GDBResultHandler *h)
{
	d->command(cmd, params, h);
}

void GDBDriver::command(const QString& cmd, const QString& cond, const QStringList& params, GDBResultHandler *h)
{
	d->command(cmd, cond, params, h);
}

void GDBDriver::stateChanged(int state)
{
	bool bf;
	
	bf = state == GDBDriverThread::Uninitialized;
	m_start->setEnabled(bf);
	m_stop->setEnabled(!bf);
	
	if ( bf && ui )
		ui->clear();
	
	//m_start->setText(bf ? tr("&Start") : tr("&Stop"));
	//m_start->setIcon(bf ? QIcon(":/debug-run.png") : QIcon(":/debug-stop.png"));
	
	if ( bf )
		setLocation(QString(), -1);
	
	bf = state == GDBDriverThread::Waiting;
	m_continue->setEnabled(bf);
	m_finish->setEnabled(bf);
	m_step->setEnabled(bf);
	m_stepi->setEnabled(bf);
	m_next->setEnabled(bf);
	m_nexti->setEnabled(bf);
	m_until->setEnabled(bf);
	m_runto->setEnabled(bf);
	m_backtrace->setEnabled(bf);
	m_customCommand->setEnabled(bf);
}

void GDBDriver::processResult(RecordNode *root, int type)
{
	if ( type == GDBDriverThread::Success )
	{
		RecordNode *rn = root->field("stack");
		
		if ( rn )
		{
			QTreeWidget *bt = new QTreeWidget;
			bt->setAttribute(Qt::WA_DeleteOnClose, true);
			bt->setColumnCount(4);
			bt->setRootIsDecorated(false);
			bt->setAlternatingRowColors(true);
			bt->setHeaderLabels(QStringList()
					<< "Function"
					<< "Address"
					<< "File"
					<< "Line"
				);
			
			QObject::connect(	bt	, SIGNAL( itemActivated(QTreeWidgetItem*, int) ),
								o	, SLOT  ( itemActivated(QTreeWidgetItem*, int) ) );
			
			QList<QTreeWidgetItem*> l;
			
			for ( int i = 0; i < rn->children.count(); ++i )
			{
				RecordNode *frame = rn->children.at(i);
				
				if ( frame->name != "frame" )
					continue;
				
				QTreeWidgetItem *it = new QTreeWidgetItem(QStringList()
						<< frame->fieldValue("func")
						<< frame->fieldValue("addr")
						<< frame->fieldValue("fullname")
						<< frame->fieldValue("line")
					);
				
				l << it;
			}
			
			bt->addTopLevelItems(l);
			
			bt->resizeColumnToContents(0);
			bt->resizeColumnToContents(1);
			bt->resizeColumnToContents(2);
			bt->resizeColumnToContents(3);
			
			QSize max = bt->maximumViewportSize();
			
			bt->resize(qBound(300, max.width(), 1024), qBound(80, max.height(), 400));
			
			bt->show();
		} else {
			
		}
	} else {
		
	}
}

#include "gdbdriver.moc"

