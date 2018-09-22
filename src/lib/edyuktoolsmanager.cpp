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

#include "edyuktoolsmanager.h"

#include "edyukapplication.h"

#include "edyuktoolsdialog.h"

#include "edyukgui.h"
#include "qshortcutmanager.h"
#include "qmdiworkspace.h"

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QFileInfo>
#include <QMessageBox>

static QStringList escaped(const QStringList& l)
{
	QStringList f;
	
	foreach ( QString s, l )
	{
		if ( s.contains( QChar(0x20) ) )
			f << "\"" + s + "\"";
		else
			f << s;
	}
	
	return f;
}

/*!
	\file edyuktoolsmanager.cpp
	\brief Implementation of the EdyukToolsManager class
	
	\see EdyukToolsManager
*/

/*!
	\class EdyukToolsManager
	\brief This class manages tools
	
	Tools are usually command line applications meant to perfrom basic tasks
	that are not build into Edyuk nor in a plugin.
	
	For the sake of simplicity tools can be added and removed programatically
	by plugins but it is recommended to use the tools configuration dialog,
	accessible in the Tools menu.
	
	A tool is basically defined by :
		- a caption (string used to create menu entry)
		- an executable
		- a working directory
		- a set of arguments
	
	The three last values can be "macro'ed". This means that a tool can act
	according to the current state of Edyuk. All macros are placed between
	'$' characters. Valid macros are :
		- PROJECT : current project's name
		- PROJECT_FILE : current project's full filename
		- PROJECT_PATH : current project's full filepath
		- PROJECT_FILES : all files belonging to current project
		- FILE : active file name
		- FILES : opened files
	
	XML representation :
	
	A tools file is written in DOM level 2 :
	
	\code
	<!DOCTYPE TOOLS>
	<TOOLS>
		<tool id="0" caption="Some tool" program="empty" />
	</TOOLS>
	\endcode
	
	Supported tools attribute are :
		- id (internal sorting : do NOT interfere!!!)
		- caption (string displayed on menu)
		- program (executable, can be full filename or just name, if the program
		is in PATH)
		- pwd (working directory)
		- arguments (arguments passed to the program, spaces must be quoted
		properly)
		- environment (additional environment variables to be passed to the
		tool. This isn't supported by the tools dialog.)
	
*/

/*!
	Constructor
*/
EdyukToolsManager::EdyukToolsManager()
{
	pActions = new QActionGroup(this);
	connect(pActions, SIGNAL( triggered(QAction*) ),
			this	, SLOT  ( toolTriggered(QAction*) ) );
	
	pDoc = new QDomDocument("TOOLS");
	
	pDialog = new EdyukToolsDialog(this);
	
	readXml();
}

/*!
	Destructor
*/
EdyukToolsManager::~EdyukToolsManager()
{
	writeXml();
	
	delete pDoc;
}

/*!
	\brief Shows the shortcut configuration dialog
*/
void EdyukToolsManager::configure()
{
	pDialog->exec();
}

/*!

*/
void EdyukToolsManager::retranslate()
{
	pDialog->retranslate();
}

/*!

*/
int EdyukToolsManager::addTool(const QString& caption, const QString& exe,
							const QString& pwd, const QStringList& args,
							const QStringList& env)
{
	if ( caption.isEmpty() )
		return -1;
	
	int id = tools().size();
	Tool t = findXml(QString::number(id));
	
	t.setCaption(caption);
	t.setProgram(exe);
	t.setWorking(pwd);
	t.setArguments(args);
	t.setEnvironment(env);
	
	updateActions();
	
	return id;
}

/*!

*/
void EdyukToolsManager::remTool(int id)
{
	if ( id < 0 )
		return;
	
	Tool tg;
	QDomNodeList l = tools();
	
	for ( int i = 0; i < l.size(); i++ )
	{
		Tool elem = l.at(i).toElement();
		
		if ( elem.isNull() )
			continue;
		
		int tid = elem.id();
		
		if ( tid == id )
		{
			//qDebug("removing tool %i", id);
			tg = elem;
		} else if ( tid > id ) {
			//qDebug("moving tool %i to %i", tid, tid - 1);
			elem.setId(tid - 1);
		}
	}
	
	QList<QAction*> actions = pActions->actions();
	
	foreach ( QAction *a, actions )
	{
		int actionId = a->data().toInt();
		
		if ( actionId == id )
		{
			pActions->removeAction(a);
			//COMPONENT(shortcutManager)->unregisterAction(a);
			delete a;
		} else if ( actionId > id ) {
			a->setData(actionId - 1);
		}
	}
	
	if ( !tg.isNull() )
		tg.parentNode().removeChild(tg);
	
	// update menus
	//updateActions();
}

/*!
	
*/
void EdyukToolsManager::swapToolIds(int id1, int id2)
{
	QDomNodeList l = tools();
	
	for ( int i = 0; i < l.size(); i++ )
	{
		Tool elem = l.at(i).toElement();
		
		if ( elem.isNull() )
			continue;
		
		int tid = elem.id();
		
		if ( tid == id1 )
		{
			//qDebug("removing tool %i", id);
			elem.setId(id2);
		} else if ( tid == id2 ) {
			//qDebug("moving tool %i to %i", tid, tid - 1);
			elem.setId(id1);
		}
	}
	
	QList<QAction*> al = pActions->actions();
	
	foreach ( QAction *a, al )
	{
		int id = a->data().toInt();
		
		if ( id == id1 )
		{
			a->setData(id2);
		} else if ( id == id2 ) {
			a->setData(id1);
		}
	}
	
	updateActions();
}

/*!

*/
void EdyukToolsManager::readXml()
{
	QFile file(Edyuk::settingsPath() + "tools.xml");
	
	if ( !file.open(QFile::ReadOnly | QFile::Text) )
	{
		if ( !file.open(QFile::WriteOnly | QFile::Text) )
			qWarning("Unable to access tools...");
		
		QString s = "<!DOCTYPE TOOLS>\n<TOOLS>\n\n</TOOLS>\n";
		
		QTextStream out(&file);
		out << s;
		
		file.close();
		
		if ( !file.open(QFile::ReadOnly | QFile::Text) )
			return (void)qWarning("Unable to access tools...");
	}
	
	if ( !pDoc->setContent(&file) || pDoc->documentElement().isNull() )
	{
		//qDebug() << "empty tools file...";
		
		QDomElement root = pDoc->createElement("TOOLS");
		
		pDoc->appendChild(root);
	}
	
	updateActions();
}

/*!

*/
void EdyukToolsManager::writeXml()
{
	QFile f(Edyuk::settingsPath() + "tools.xml");
	QTextStream out(&f);
	
	if ( f.open(QFile::WriteOnly | QFile::Text) )
		out << pDoc->toString(4).replace("    ", "\t");
	else
		qWarning("Can\'t save tools : check out permissions");
	
}

/*!

*/
EdyukToolsManager::Tool EdyukToolsManager::tool(int id)
{
	if ( id < 0 )
		return QDomElement();
	
	Tool elem;
	QDomNodeList l = tools();
	
	for ( int i = 0; i < l.size(); i++ )
	{
		elem = l.at(i).toElement();
		
		if ( elem.id() == id )
			return elem;
	}
	
	//qDebug("creating tool [%i]", id);
	
	elem = pDoc->createElement("tool");
	elem.setId(id);
	
	pDoc->documentElement().appendChild(elem);
	
	return elem;
}

/*!

*/
void EdyukToolsManager::updateActions()
{
	QList<QAction*> oldActions = pActions->actions();
	
	QDomNodeList l = tools();
	QVector<QAction*> actions(l.size());
	
	for ( int i = 0; i < l.size(); i++ )
	{
		Tool t = l.at(i).toElement();
		
		if ( t.isNull() )
			continue;
		
		int id = t.id();
		
		if ( (id < 0) || (id >= l.size()) )
		{
			qWarning("invalid action id : %i", id);
			continue;
		}
		
		QString caption = t.caption();
		
		QAction *action = 0;
		
		/*
		for ( int i = 0; i < oldActions.count(); ++i )
		{
			QAction *a = oldActions.at(i);
			
			if ( a->data().toInt() == id )
			{
				action = a;
				oldActions.removeAt(i);
				break;
			}
		}
		*/
		
		if ( action )
		{
			// update caption
			action->setText(caption);
		} else {
			// create new action
			action = new QAction(caption, this);
			action->setData(id);
			//pActions->addAction(action);
			EDYUK_SHORTCUT(action, "Tools/User", "");
		}
		
		actions[id] = action;
	}
	
	// it is SO insane to be forced to do that to workaround QActionGroup...
	foreach ( QAction *a, actions )
	{
		if ( a )
		{
			pActions->addAction(a);
		}
	}
	
	// it is SO insane to be forced to do that to workaround QActionGroup.
	foreach ( QAction *a, oldActions )
	{
		pActions->removeAction(a);
		delete a;
	}
	
	emit toolsChanged(pActions);
}

/*!

*/
EdyukToolsManager::Tool EdyukToolsManager::findXml(const QString& id)
{
	return tool( id.toInt() );
}

/*!

*/
QDomNodeList EdyukToolsManager::tools() const
{
	return pDoc->elementsByTagName("tool");
}

/*!

*/
void EdyukToolsManager::toolError(QProcess::ProcessError e)
{
	QString msg = tr("Code : %1\nDescription : %2\n");
	
	switch ( e )
	{
		case QProcess::FailedToStart :
			msg = msg
				.arg(tr("QProcess::FailedToStart"))
				.arg(tr("The process failed to start.\n"
						"Either the invoked program is missing, or \n"
						"you may have insufficient permissions to \n"
						"invoke the program."
						)
					);
			break;
			
		case QProcess::Crashed :
			msg = msg
				.arg(tr("QProcess::Crashed"))
				.arg(tr("The process crashed some time\n"
						"after starting successfully."
						)
					);
			break;
			
		case QProcess::Timedout :
			msg = msg
				.arg(tr("QProcess::Timedout"))
				.arg(tr("The last waitFor...() function\n"
						"timed out. The state of QProcess is unchanged,\n"
						"and you can try calling waitFor...() again."
						)
					);
			break;
			
		case QProcess::WriteError :
			msg = msg
				.arg(tr("Code : QProcess::WriteError"))
				.arg(tr("An error occurred when attempting\n"
						"to write to the process. For example, the process\n"
						"may not be running, or it may have closed its\n"
						"input channel."
						)
					);
			break;
			
		case QProcess::ReadError :
			msg = msg
				.arg(tr("Code : QProcess::ReadError"))
				.arg(tr("An error occurred when attempting to\n"
						"read from the process. For example, the process may\n"
						"not be running."
						)
					);
			break;
			
		default :
			msg = msg
				.arg(tr("Code : QProcess::UnknownError"))
				.arg(tr("Not clear enough?."));
			break;
	}
	
	QMessageBox::warning(0, "Tool error!", msg);
}

/*!
	\internal
*/
void EdyukToolsManager::toolTriggered(QAction *a)
{
	int id = a->data().toInt();
	
	Tool d = tool(id);
	
	if ( d.caption() != a->text() )
		return (void)qWarning("Unable to access tool %s : wrong id",
								qPrintable(a->text()));
	
	
	QProcess *t = new QProcess;
	
	connect(t	, SIGNAL( finished(int, QProcess::ExitStatus) ),
			t	, SLOT  ( deleteLater() ) );
	
	connect(t	, SIGNAL( error(QProcess::ProcessError) ),
			this, SLOT  ( toolError(QProcess::ProcessError) ) );
	
	
	QHash<QString, QString> macros;
	QHash<QString, QString>::iterator i;
	
	QString proj, // = COMPONENT(workspace)->currentProject(),
			file = COMPONENT(gui)->currentFile(),
			files = escaped( COMPONENT(gui)->openedFiles() ).join(" "),
			proj_files; // = escaped( COMPONENT(workspace)->files(proj) ).join(" ");
	
	QFileInfo info(proj);
	
	macros.insert("PROJECT", info.baseName());
	macros.insert("PROJECT_FILE", info.fileName());
	macros.insert("PROJECT_PATH", info.path());
	macros.insert("PROJECT_FILES", proj_files);
	macros.insert("FILE", file);
	macros.insert("FILES", files);
	
	QString prog = d.program(),
			working = d.working();
	
	QStringList args = d.arguments();
	
	args.removeAll(QString());
	
	for ( i = macros.begin(); i != macros.end(); i++ )
	{
		QString before = "$" + i.key() + "$",
				after = *i;
		
		prog.replace(before, after);
		working.replace(before, after);
		args.replaceInStrings(before, after);
	}
	
	if ( working.count() )
		t->setWorkingDirectory(working);
	
	qDebug("starting tool : [%s %s]", qPrintable(prog), qPrintable(args.join(" ")));
	
	t->start(prog, args);
	
	//QProcess::startDetached(prog, args, working);
}
