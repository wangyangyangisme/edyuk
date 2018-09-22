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

#include "edyuktoolsdialog.h"

#include "edyuktoolsmanager.h"

/*!
	\file edyuktoolsdialog.cpp
	\brief Implementation of the EdyukToolsDialog class
	
	\see EdyukToolsDialog
*/

#include <QDir>
#include <QFileInfo>
#include <QFileDialog>

/*!
	\ingroup gui
	@{
	
	\class EdyukToolsDialog
	\brief A simple dialog used to manages tool on run-time
	
	\note Changes are immediately propagated to the tools menu.
	
	\see EdyukToolsManager
*/

EdyukToolsDialog::EdyukToolsDialog(EdyukToolsManager *m, QWidget *p)
 : QDialog(p), pManager(m)
{
	setupUi(this);
	
	helpFrame->hide();
}

void EdyukToolsDialog::retranslate()
{
	retranslateUi(this);
}

void EdyukToolsDialog::exec()
{
	lwTools->clear();
	
	EdyukToolsManager::Tool t;
	QDomNodeList l = pManager->tools();
	
	QVector<EdyukToolsManager::Tool> tools(l.size());
	
	for ( int i = 0; i < l.size(); i++  )
	{
		t = l.at(i).toElement();
		
		int id = t.id();
		
		if ( id < l.size() && i > -1 )
			tools[id] = t;
		else
			qWarning("Invalid tool : %s. (id=%i)", qPrintable(t.caption()), id);
	}
	
	foreach ( t, tools )
	{
		QListWidgetItem *item = new QListWidgetItem(t.caption());
		lwTools->addItem(item);
	}
	
	lwTools->setCurrentRow(lwTools->count() ? 0 : -1);
	on_lwTools_currentRowChanged(lwTools->currentRow());
	
	int ret = QDialog::exec();
	
	if ( ret == Accepted )
	{
		pManager->writeXml();
		pManager->updateActions();
	} else {
		pManager->readXml();
	}
}

void EdyukToolsDialog::on_lwTools_currentRowChanged(int i)
{
	if ( i >= lwTools->count() )
		i = lwTools->count() - 1;
	
	bool exist = i != -1;
	
	if ( exist )
	{
		EdyukToolsManager::Tool t = pManager->tool(i);
		
		leCaption->setText(t.caption());
		leProg->setText(t.program());
		lePWD->setText(t.working());
		leArgs->setText(t.arguments().join(" "));
	} else {
		leCaption->clear();
		leProg->clear();
		lePWD->clear();
		leArgs->clear();
	}
	
	gbProperties->setEnabled(exist);
	bDelete->setEnabled(exist);
	
	bUp->setEnabled(exist && (i > 0));
	bDown->setEnabled(exist && (i < (lwTools->count() - 1)));
}

void EdyukToolsDialog::on_bNew_clicked()
{
	pManager->addTool(tr("New tool"), QString(), QString());
	
	QListWidgetItem *i = new QListWidgetItem(tr("New tool"));
	lwTools->addItem(i);
	lwTools->setCurrentRow(lwTools->count() - 1);
	leCaption->setFocus();
}

void EdyukToolsDialog::on_bDelete_clicked()
{
	int id = lwTools->currentRow();
	
	pManager->remTool(id);
	
	lwTools->blockSignals(true);
	
	delete lwTools->takeItem(id);
	
	int nid = id >= lwTools->count() ? id - 1 : id;
	
	lwTools->setCurrentRow(nid);
	
	lwTools->blockSignals(false);
	
	on_lwTools_currentRowChanged(nid);
}

void EdyukToolsDialog::on_bUp_clicked()
{
	int i = lwTools->currentRow();
	
	if ( i < 1 )
		return;
	
	pManager->swapToolIds(i, i - 1);
	/*
	EdyukToolsManager::Tool cur = pManager->tool(i);
	EdyukToolsManager::Tool up = pManager->tool(i - 1);
	
	cur.setId(i - 1);
	up.setId(i);
	*/
	
	QListWidgetItem *c = lwTools->takeItem(i);
	QListWidgetItem *u = lwTools->takeItem(i - 1);
	
	lwTools->insertItem(i - 1, c);
	lwTools->insertItem(i, u);
	
	lwTools->setCurrentRow(i - 1);
}

void EdyukToolsDialog::on_bDown_clicked()
{
	int i = lwTools->currentRow();
	
	if ( i >= (lwTools->count() - 1) || i < 0 )
		return;
	
	pManager->swapToolIds(i, i + 1);
	/*
	EdyukToolsManager::Tool cur = pManager->tool(i);
	EdyukToolsManager::Tool down = pManager->tool(i + 1);
	
	cur.setId(i + 1);
	down.setId(i);
	*/
	QListWidgetItem *u = lwTools->takeItem(i + 1);
	QListWidgetItem *c = lwTools->takeItem(i);
	
	lwTools->insertItem(i, u);
	lwTools->insertItem(i + 1, c);
	
	lwTools->setCurrentRow(i + 1);
}

void EdyukToolsDialog::on_leCaption_editingFinished()
{
	int id = lwTools->currentRow();
	QString txt = leCaption->text();
	
	QListWidgetItem *i = lwTools->currentItem();
	
	if ( !i || txt.isEmpty() )
		return;
	
	EdyukToolsManager::Tool t = pManager->tool(id);
	
	i->setText(txt);
	t.setCaption(txt);
}

static const QString _envSplitter =
#ifdef Q_OS_WIN32
	";"
#else
	":"
#endif
	;

static const QString _invalidInputSheet(
	"QLineEdit { background-color : red; color : white; }"
);

static const QString _ambiguousInputSheet(
	"QLineEdit { background-color : yellow; color : white; }"
);

void EdyukToolsDialog::on_leProg_editingFinished()
{
	EdyukToolsManager::Tool t = pManager->tool( lwTools->currentRow() );
	
	t.setProgram( leProg->text() );
}

void EdyukToolsDialog::on_leProg_textChanged()
{
	// check whether runable...
	QString exe = leProg->text();
	
	leProg->setStyleSheet(QString());
	
	if ( exe.count() )
	{
		QFileInfo exeinfo(exe);
		
		if ( (exe.count('/') || exe.count('\\')) )
		{
			// absolute path specified : simple check
			if ( !exeinfo.exists() || !exeinfo.isExecutable() )
			{
				leProg->setStyleSheet(_invalidInputSheet);
			}
		} else {
			// gotta check env to figure out what will be called
			// TODO : change the line edit to inform the user which exe will be called?
			
			QStringList env = QProcess::systemEnvironment();
			
			foreach ( QString entry, env )
			{
				if ( entry.startsWith("PATH=") )
				{
					entry.remove(0, 5);
					
					QStringList alt;
					QStringList pathes = entry.split(_envSplitter);
					
					foreach ( QString path, pathes )
					{
						QDir d(path);
						//d.setFilter(QDir::Files | QDir::Executable);
						
						QFileInfo info(d.filePath(exe));
						
						if ( info.exists() && info.isExecutable() )
						{
							alt << info.filePath();
						}
						
					}
					
					if ( alt.isEmpty() )
					{
						leProg->setStyleSheet(_invalidInputSheet);
					} else if ( alt.count() > 1 ) {
						leProg->setStyleSheet(_ambiguousInputSheet);
					}
					
					break;
				}
			}
		}
	}
}

void EdyukToolsDialog::on_lePWD_editingFinished()
{
	EdyukToolsManager::Tool t = pManager->tool( lwTools->currentRow() );
	
	t.setWorking( lePWD->text() );
}

void EdyukToolsDialog::on_lePWD_textChanged()
{
	// check existence
	QString pwd = lePWD->text();
	
	lePWD->setStyleSheet(QString());
	
	if ( pwd.count() )
	{
		QFileInfo pwdinfo(pwd);
		
		if ( !pwdinfo.exists() || !pwdinfo.isDir() )
		{
			lePWD->setStyleSheet(_invalidInputSheet);
		}
	}
}

void EdyukToolsDialog::on_leArgs_editingFinished()
{
	EdyukToolsManager::Tool t = pManager->tool( lwTools->currentRow() );
	
	QStringList before = leArgs->text().split( QChar(0x20) ),
				after;
	
	for ( int i = 0; i < before.count(); i++ )
	{
		QString s = before.at(i);
		
		if ( s.startsWith("\"") )
			while ( !s.endsWith("\"") && (i+1) < before.count() )
				s += before.at(++i);
		
		after << s;
	}
	
	t.setArguments(after);
}

void EdyukToolsDialog::on_tbSelectProgram_clicked()
{
	// TODO : use a better selector with the following features
	// * filter to only show executables
	// * offer a way to directly access apps registered in the DE without finding their path
	
	QString exe = QFileDialog::getOpenFileName(this, tr("Select a program"), leProg->text());
	
	if ( exe.count() )
	{
		leProg->setText(exe);
		on_leProg_editingFinished();
	}
}

void EdyukToolsDialog::on_tbSelectWorkingDir_clicked()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select a working directory"), lePWD->text());
	
	if ( dir.count() )
	{
		lePWD->setText(dir);
		on_lePWD_editingFinished();
	}
}

/*! @} */
