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

#include "edyukconfigdialog.h"

/*!
	\file edyukconfigdialog.cpp
	\brief Implementation of the EdyukConfigDialog class
	
	\see EdyukConfigDialog
*/

#include "qsettingsserver.h"
#include "qsettingsclient.h"

#include "qeditconfig.h"
#include "qsnippetedit.h"

#include "qdocument.h"
#include "qformatscheme.h"
#include "qformatconfig.h"
#include "qlanguagefactory.h"

#include "qpluginmanagerconfig.h"

#include "qwidgetstack.h"

#include <QMessageBox>
#include <QFontDatabase>

/*!
	\class EdyukConfigDialog
	\brief A dialog to edit application-wide settings.
*/

/*!
	\brief ctor
*/
EdyukConfigDialog::EdyukConfigDialog(QSettingsServer *s, QWidget *p)
 : QDialog(p), pServer(s)
{
	setupUi(this);
	
	connect(this, 		SIGNAL( accepted() ),
			this, 		SLOT  ( apply() ) );
	
	connect(buttonBox,  SIGNAL( accepted() ),
			this,		SLOT  ( tryAccept() ) );
	
	connect(buttonBox,  SIGNAL( rejected() ),
			this,		SLOT  ( tryReject() ) );
	
	connect(buttonBox,  SIGNAL( clicked(QAbstractButton*) ),
			this,		SLOT  ( slotButtonBarClicked(QAbstractButton*) ) );
	
	m_pluginConfig = new QPluginManagerConfig(0, this);
	
	m_editConfig = new QEditConfig(this);
	m_editConfig->setApplyImmediately(true);
	
	m_formatConfig = new QFormatConfig(this);
	m_formatConfig->setAutonomous(true);
	
	m_snippetConfig = new QSnippetEdit(this);
	
	QSettingsClient c(s, "editor");
	QStringList keys = c.allKeys();
	
	foreach ( QString k, keys )
		m_editKeys[k] = c.value(k);
	
	m_editConfig->loadKeys(m_editKeys);
	
	m_editKeys.clear();
	
	connect(m_editConfig, SIGNAL( keyChanged(QString, QVariant) ),
			this		, SLOT  ( editorKeyChanged(QString, QVariant) ) );
	
	tabOptions->addTab(m_pluginConfig, tr("Plugins"));
	tabOptions->addTab(m_editConfig, tr("Editor"));
	tabOptions->addTab(m_formatConfig, tr("Formatting"));
	tabOptions->addTab(m_snippetConfig, tr("Snippets"));
}

EdyukConfigDialog::~EdyukConfigDialog()
{
	
}

void EdyukConfigDialog::retranslate()
{
	cboReopenFiles->clear();
	cboReopenProjects->clear();
	cboInstances->clear();
	cboPerspective->clear();
	cboLanguage->clear();
	cboManagerDock->clear();
	cboAutoSaveOnBuild->clear();
	cboAutoSaveOnExit->clear();
	
	retranslateUi(this);
	
	tabOptions->setTabText(1, tr("Plugins"));
	m_pluginConfig->retranslate();
	
	tabOptions->setTabText(2, tr("Editor"));
	m_editConfig->retranslate();
	
	tabOptions->setTabText(3, tr("Formatting"));
	m_editConfig->retranslate();
	
	tabOptions->setTabText(4, tr("Snippets"));
	m_snippetConfig->retranslate();
}

void EdyukConfigDialog::setSnippetManager(QSnippetManager *m)
{
	m_snippetConfig->setSnippetManager(m);
}

void EdyukConfigDialog::loadFormatSchemes(QLanguageFactory *f)
{
	QFormatScheme *def = QDocument::defaultFormatScheme();
	
	if ( def )
		m_formatConfig->addScheme("default", def);
	
	QStringList langs = f->languages();
	
	foreach ( const QString& lang, langs )
	{
		const QLanguageFactory::LangData& ld = f->languageData(lang);
		
		if ( ld.s != def )
		{
			m_formatConfig->addScheme(ld.lang, ld.s);
		}
	}
}

void EdyukConfigDialog::editorKeyChanged(const QString& key, const QVariant& value)
{
	//m_editKeys[key] = value;
	QSettingsClient c(pServer, "editor");
	c.setValue(key, value);
	
	//qDebug("setting key %s to %s", qPrintable(key), qPrintable(value.toString()));
}

void EdyukConfigDialog::tryAccept()
{
	m_pluginConfig->tryCommit();
	
	if ( tabOptions->currentWidget() == m_formatConfig )
		m_formatConfig->apply();
	
	accept();
}

void EdyukConfigDialog::tryReject()
{
	m_pluginConfig->tryCommit();
	
	if ( tabOptions->currentWidget() == m_formatConfig )
		m_formatConfig->cancel();
	
	reject();
}

void EdyukConfigDialog::closeEvent(QCloseEvent *e)
{
	m_pluginConfig->tryCommit();
	
	QDialog::closeEvent(e);
}

void EdyukConfigDialog::slotButtonBarClicked(QAbstractButton *button)
{
	if ( buttonBox->buttonRole(button) == QDialogButtonBox::ResetRole )
	{
		//if ( !pServer )
		//	return;
		//
		//pServer->setDefault();
		//reload();
		
		// recent files/projects
		spnDisplayRecentFiles->setValue(15);
		spnDisplayRecentProjects->setValue(5);
		// !recent
		
		// opened files/projects
		int files = Edyuk::ReopenAll;
		chkReopenFiles->setChecked(true);
		cboReopenFiles->setCurrentIndex(files);
		
		int projects = Edyuk::ReopenAll;
		chkReopenProjects->setChecked(true);
		cboReopenProjects->setCurrentIndex(projects);
		// !opened
		
		// gui
		cboPerspective->setCurrentIndex(Edyuk::ReloadPerspectiveLastUsed);
		cboInstances->setCurrentIndex(Edyuk::InstanceSingle);
		// !gui
		
		// autosave
		cboAutoSaveOnBuild->setCurrentIndex(Edyuk::AlwaysAsk);
		cboAutoSaveOnExit->setCurrentIndex(Edyuk::AlwaysAsk);
		// !autosave
		
		// systray
		cbMinimizeToTray->setChecked(EDYUK_TRAY_DEFAULT);
		cbWarnUponMinimize->setChecked(true);
		// !systray
		
		// lang
		cboLanguage->setCurrentIndex(Edyuk::SystemLocale);
		// !lang
		
		// manager dock
		cboManagerDock->setCurrentIndex(QWidgetStack::ToolBox);
		// !manager dock
	}
}

void EdyukConfigDialog::apply()
{
	if ( !pServer )
		return;
	
	QSettingsClient s(pServer);
	
	// recent files/projects
	s.beginGroup("recent");
	
	s.setValue("filecount", spnDisplayRecentFiles->value());
	s.setValue("projectcount", spnDisplayRecentProjects->value());
	
	s.endGroup();
	// !recent
	
	// opened files/projects
	s.beginGroup("opened");
	
	if ( chkReopenFiles->isChecked() )
	{
		switch ( cboReopenFiles->currentIndex() )
		{
			case 0: // All
				s.setValue("file_mode", Edyuk::ReopenAll);
				break;
				
			case 1:	// Current
				s.setValue("file_mode", Edyuk::ReopenCurrent);
				break;
		}
	} else {
		s.setValue("file_mode", Edyuk::ReopenNone);
	}

	if ( chkReopenProjects->isChecked() )
	{
		switch ( cboReopenProjects->currentIndex() )
		{
			case 0: // All
				s.setValue("project_mode", Edyuk::ReopenAll);
				break;
				
			case 1:	// Current
				s.setValue("project_mode", Edyuk::ReopenCurrent);
				break;
		}
	} else {
		s.setValue("project_mode", Edyuk::ReopenNone);
	}
	
	s.endGroup();
	// !opened
	
	// gui
	s.beginGroup("gui");
	s.setValue("mode", cboPerspective->currentIndex());
	s.setValue("instances", cboInstances->currentIndex());
	s.endGroup();
	// !gui
	
	// systray
	s.beginGroup("tray");
	s.setValue("enabled", cbMinimizeToTray->isChecked());
	s.setValue("warn", cbWarnUponMinimize->isChecked());
	s.endGroup();
	// !systray
	
	// autosave
	s.beginGroup("autosave");
	s.setValue("build", cboAutoSaveOnBuild->currentIndex());
	s.setValue("exit", cboAutoSaveOnExit->currentIndex());
	s.endGroup();
	// !autosave
	
	// lang
	s.beginGroup("lang");
	s.setValue("mode", cboLanguage->currentIndex());
	s.endGroup();
	// !lang
	
	// manager dock
	s.beginGroup("docks");
	s.setValue("manager-display", cboManagerDock->currentIndex());
	s.endGroup();
	// !manager dock
	
	if ( tabOptions->currentWidget() == m_formatConfig )
		m_formatConfig->apply();
	
}

void EdyukConfigDialog::reload()
{
	if ( !pServer )
		return;
	
	QSettingsClient s(pServer);
	
	// recent files/projects
	s.beginGroup("recent");
	
	spnDisplayRecentFiles->setValue(s.value("filecount", 15).toInt());
	spnDisplayRecentProjects->setValue(s.value("projectcount", 5).toInt());
	
	s.endGroup();
	// !recent
	
	// opened files/projects
	s.beginGroup("opened");
	
	int files = s.value("file_mode", Edyuk::ReopenAll).toInt();
	
	if ( files != Edyuk::ReopenNone )
	{
		chkReopenFiles->setChecked(true);
		cboReopenFiles->setCurrentIndex(files); // != Edyuk::ReopenAll);
	}
	
	int projects = s.value("project_mode", Edyuk::ReopenAll).toInt();
	
	if ( projects != Edyuk::ReopenNone )
	{
		chkReopenProjects->setChecked(true);
		cboReopenProjects->setCurrentIndex(projects); // != Edyuk::ReopenAll);
	}
	
	s.endGroup();
	// !opened
	
	// gui
	s.beginGroup("gui");
	int perspective_mode = s.value("mode", Edyuk::ReloadPerspectiveLastUsed).toInt();
	cboPerspective->setCurrentIndex(perspective_mode);

	int instances = s.value("instances", Edyuk::InstanceSingle).toInt();
	cboInstances->setCurrentIndex(instances);
		
	s.endGroup();
	// !gui
	
	// systray
	s.beginGroup("tray");
	cbMinimizeToTray->setChecked(s.value("enabled", EDYUK_TRAY_DEFAULT).toBool());
	cbWarnUponMinimize->setChecked(s.value("warn", true).toBool());
	s.endGroup();
	// !systray
	
	// autosave
	s.beginGroup("autosave");
	cboAutoSaveOnBuild->setCurrentIndex(s.value("build", Edyuk::AlwaysAsk).toInt());
	cboAutoSaveOnExit->setCurrentIndex(s.value("exit", Edyuk::AlwaysAsk).toInt());
	s.endGroup();
	// !autosave
	
	// lang
	s.beginGroup("lang");
	
	int lang = s.value("mode", Edyuk::SystemLocale).toInt();
	cboLanguage->setCurrentIndex(lang);
	
	s.endGroup();
	// !lang
	
	// manager dock
	s.beginGroup("docks");
	cboManagerDock->setCurrentIndex(s.value("manager-display", QWidgetStack::ToolBox).toInt());
	s.endGroup();
	// !manager dock
	
	// editor
	s.beginGroup("editor");
	
	//m_editConfig->setValues(s);
	
	m_editKeys.clear();
	QStringList keys = s.allKeys();
	
	foreach ( QString k, keys )
		m_editKeys[k] = s.value(k);
	
	m_editConfig->loadKeys(m_editKeys);
	
	m_editKeys.clear();
	
	s.endGroup();
	// !editor

	// here comes the plugin-related stuff
	//reloadPlugins();
	m_pluginConfig->commit();
	
	tabOptions->setCurrentIndex(0);
}
