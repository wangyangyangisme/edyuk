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
	\file qmakesettings.cpp
	
	\brief Implementation of QMakeSettings
*/

#include "qmakesettings.h"

#include "qprojectmodel.h"
#include "qprojectproxymodel.h"

#include "qmakebackend.h"

using namespace QMakeModel;

#include "edyuk.h"

#include <QMenu>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>

static void removeVariable(QProject *p, const QString& var);
static void setValue(QProject *p, const QString& var, const QString& val);
static void addValue(QProject *p, const QString& var, const QString& val);
static void removeValue(QProject *p, const QString& var, const QString& val, bool fullRemove = false);

QMakeSettings::QMakeSettings(QWidget *p)
 : QDialog(p), m_project(0)
{
	setupUi(this);
	
	m_proxy = new QProjectProxyModel(this);
	m_proxy->setDetailLevel(-1);
	
	lwDefines->setContextMenuPolicy(Qt::CustomContextMenu);
	lwIncludes->setContextMenuPolicy(Qt::CustomContextMenu);
	lwLibraries->setContextMenuPolicy(Qt::CustomContextMenu);
	
	wAdvanced->layout()->addWidget(tvAdvanced->actionBar());
	
	lwQtModules->addItems(QStringList()
			<< "QtCore"
			<< "QtGui"
			<< "QtNetwork"
			<< "QtOpenGL"
			<< "QtSql"
			<< "QtScript"
			<< "QtSvg"
			<< "QtWebKit"
			<< "QtXml"
			<< "QtXmlPatterns"
			<< "Phonon"
			<< "Qt3Support"
			<< "QtDBus"
			<< "QtTest"
			<< "QtHelp"
			<< "QtDesigner"
			<< "QtUiTools"
			<< "QtAssistant"
		);
	
	for ( int i = 0; i < lwQtModules->count(); ++i )
	{
		QListWidgetItem *it = lwQtModules->item(i);
		it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
	}
}

QMakeSettings::~QMakeSettings()
{
	QProjectModel *m = qobject_cast<QProjectModel*>(m_proxy->sourceModel());
	
	if ( m )
	{
		m->removeEditorWrapper(m_proxy);
	}
}

void QMakeSettings::setProject(QProject *pro)
{
	Project *p = dynamic_cast<Project*>(pro);
	
	if ( !p )
	{
		qDebug("invalid project...");
		return;
	}
	
	m_project = 0;
	
	QString tmp;
	QStringList l;
	static QStringList _templates = QStringList()
		<< "app"
		<< "lib"
		<< "subdirs"
		<< "vcapp"
		<< "vclib"
		;
	
	// general setup
	leName->setText(p->query("APP_NAME"));
	leAuthor->setText(p->query("APP_AUTHOR"));
	
	tmp = p->query("TEMPLATE");
	
	cbTemplate->setCurrentIndex(_templates.indexOf(tmp));
	
	l = Search::compute(p, "CONFIG", QStringList(), Search::Project);
	
	if ( tmp == "subdirs" )
	{
		cbOrdered->setEnabled(true);
		cbOrdered->setChecked(l.contains("ordered"));
		
		leOutputPath->setEnabled(false);
		leOutputName->setEnabled(false);
	} else {
		leOutputPath->setText(p->query("DESTDIR"));
		
		if ( leOutputPath->text().isEmpty() )
			leOutputPath->setText(QFileInfo(pro->name()).absolutePath());
		
		leOutputName->setText(p->query("TARGET"));
	}
	
	cbLanguage->setCurrentIndex(
						Search::hasValue(p, "CONFIG", "-=", "qt", Search::Project)
					?
						0
					:
						1
					);
	
	if ( l.contains("debug_and_release") )
	{
		rbDebugRelease->setChecked(true);
		cbBuildAll->setChecked(l.contains("build_all"));
	} else if ( l.contains("debug") && !l.contains("release") ) {
		rbDebug->setChecked(true);
	} else if ( l.contains("release") && !l.contains("debug") ) {
		rbRelease->setChecked(true);
	} else {
		qWarning("Conflicting setting of build mode.");
	}
	
	if (
			l.contains("warn_off")
		||
			Search::hasValue(p, "CONFIG", "-=", "warn_on", Search::Project)
		)
	{
		cbWarnings->setChecked(false);
	} else {
		cbWarnings->setChecked(true);
	}
	
	cbSTL->setChecked(l.contains("stl"));
	cbRTTI->setChecked(l.contains("rtti"));
	cbExceptions->setChecked(l.contains("exceptions"));
	
	cbDLL->setChecked(l.contains("dll"));
	cbStaticLib->setChecked(l.contains("staticlib"));
	cbPlugin->setChecked(l.contains("plugin"));
	cbDesigner->setChecked(l.contains("designer"));
	
	tmp = p->query("VERSION");
	
	if ( tmp.count() )
	{
		gbVersion->setChecked(true);
		l = tmp.split('.');
		
		sbMajor->setValue(l.at(0).toInt());
		
		if ( l.count() > 1 )
			sbMinor->setValue(l.at(1).toInt());
		
		if ( l.count() > 2 )
			sbRelease->setValue(l.at(2).toInt());
		
		if ( l.count() > 3 )
			sbBuild->setValue(l.at(3).toInt());
	}
	
	// include/libs/defines setup
	l = Search::compute(p, "INCLUDEPATH", QStringList(), Search::Project);
	lwIncludes->clear();
	lwIncludes->addItems(l);
	
	l = Search::compute(p, "DEFINES", QStringList(), Search::Project);
	lwDefines->clear();
	lwDefines->addItems(l);
	
	l = Search::compute(p, "LIBS", QStringList(), Search::Project);
	lwLibraries->clear();
	lwLibraries->addItems(l);
	
	l = Search::compute(p, "QT", QStringList(), Search::Project);
	
	for ( int i = 0; i < lwQtModules->count(); ++i )
	{
		QListWidgetItem *it = lwQtModules->item(i);
		QString equiv = it->text().remove("Qt").toLower();
		
		if (
				l.contains(equiv)
			||
				(
					(i < 2)
				&&
					!Search::hasValue(p, "QT", "-=", equiv, Search::Project)
				)
			)
			it->setCheckState(Qt::Checked);
		else
			it->setCheckState(Qt::Unchecked);
	}
	
	// translations setup
	
	
	// advanced setup
	QProjectModel *m = qobject_cast<QProjectModel*>(m_proxy->sourceModel());
	
	if ( m )
	{
		m->removeEditorWrapper(m_proxy);
	}
	
	m_proxy->setSourceModel(p->model());
	p->model()->addEditorWrapper(m_proxy);
	
	tvAdvanced->setModel(m_proxy);
	tvAdvanced->setRootIndex(m_proxy->mapFromSource(p->model()->index(p)));
	
	//
	m_project = p;
}

void QMakeSettings::on_leName_editingFinished()
{
	setValue(m_project, "APP_NAME", leName->text());
}

void QMakeSettings::on_leAuthor_editingFinished()
{
	setValue(m_project, "APP_AUTHOR", leAuthor->text());
}

void QMakeSettings::on_leOutputName_editingFinished()
{
	setValue(m_project, "TARGET", leOutputName->text());
}

void QMakeSettings::on_leOutputPath_editingFinished()
{
	setValue(m_project, "DESTDIR", Edyuk::makeRelative(m_project->name(), leOutputPath->text()));
}

void QMakeSettings::on_leBuildPath_editingFinished()
{
	QString path = Edyuk::makeRelative(m_project->name(), leBuildPath->text());
	
	if ( path.isEmpty() )
	{
		setValue(m_project, "UI_DIR", path);
		setValue(m_project, "MOC_DIR", path);
		setValue(m_project, "RCC_DIR", path);
		setValue(m_project, "OBJECTS_DIR", path);
	} else {
		setValue(m_project, "UI_DIR", path + "/ui");
		setValue(m_project, "MOC_DIR", path + "/moc");
		setValue(m_project, "RCC_DIR", path + "/rcc");
		setValue(m_project, "OBJECTS_DIR", path + "/obj");
	}
}

void QMakeSettings::on_tbOutputPath_released()
{
	QString path = leOutputPath->text();
	
	if ( QFileInfo(path).isRelative() )
		path = m_project->absoluteFilePath(path);
	
	path = m_project->relativeFilePath(QFileDialog::getExistingDirectory(0, tr("Choose the output path"), path));
	
	if ( path.count() && path != leOutputPath->text() )
	{
		leOutputPath->setText(path);
		
		setValue(m_project, "DESTDIR", Edyuk::makeRelative(m_project->name(), path));
	}
}

void QMakeSettings::on_cbTemplate_currentIndexChanged(const QString& tpl)
{
	setValue(m_project, "TEMPLATE", tpl);
	
	bool lib = (tpl == "lib" || tpl == "vclib");
	
	cbDLL->setEnabled(lib);
	cbPlugin->setEnabled(lib);
	cbStaticLib->setEnabled(lib);
	
	gbOutput->setEnabled(tpl != "subdirs");
}

void QMakeSettings::on_cbLanguage_currentIndexChanged(const QString& lng)
{
	if ( !lng.contains("Qt4") )
		removeValue(m_project, "CONFIG", "qt", true);
	else
		addValue(m_project, "CONFIG", "qt");
}

void QMakeSettings::on_rbDebug_toggled(bool on)
{
	//qDebug("debug[%i]", on);
	
	if ( on )
	{
		addValue(m_project, "CONFIG", "debug");
	} else {
		removeValue(m_project, "CONFIG", "debug");
	}
}

void QMakeSettings::on_rbRelease_toggled(bool on)
{
	//qDebug("release[%i]", on);
	
	if ( on )
	{
		addValue(m_project, "CONFIG", "release");
	} else {
		removeValue(m_project, "CONFIG", "release");
	}
}

void QMakeSettings::on_rbDebugRelease_toggled(bool on)
{
	//qDebug("debug_and_release[%i]", on);
	
	if ( on )
	{
		addValue(m_project, "CONFIG", "debug_and_release");
	} else {
		removeValue(m_project, "CONFIG", "debug_and_release");
	}
}

void QMakeSettings::on_cbWarnings_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "warn_on");
		removeValue(m_project, "CONFIG", "warn_off");
	} else {
		addValue(m_project, "CONFIG", "warn_off");
		removeValue(m_project, "CONFIG", "warn_on");
	}
}

void QMakeSettings::on_cbExceptions_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "exceptions");
	} else {
		removeValue(m_project, "CONFIG", "exceptions");
	}
}

void QMakeSettings::on_cbRTTI_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "rtti");
	} else {
		removeValue(m_project, "CONFIG", "rtti");
	}
}

void QMakeSettings::on_cbSTL_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "stl");
	} else {
		removeValue(m_project, "CONFIG", "stl");
	}
}

void QMakeSettings::on_cbBuildAll_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "build_all");
	} else {
		removeValue(m_project, "CONFIG", "build_all");
	}
}

void QMakeSettings::on_cbOrdered_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "ordered");
	} else {
		removeValue(m_project, "CONFIG", "ordered");
	}
}

void QMakeSettings::on_cbStaticLib_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "staticlib");
	} else {
		removeValue(m_project, "CONFIG", "staticlib");
	}
}

void QMakeSettings::on_cbDLL_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "dll");
	} else {
		removeValue(m_project, "CONFIG", "dll");
	}
}

void QMakeSettings::on_cbPlugin_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "plugin");
	} else {
		removeValue(m_project, "CONFIG", "plugin");
	}
}

void QMakeSettings::on_cbDesigner_toggled(bool on)
{
	if ( on )
	{
		addValue(m_project, "CONFIG", "designer");
	} else {
		removeValue(m_project, "CONFIG", "designer");
	}
}

void QMakeSettings::on_gbVersion_toggled(bool on)
{
	if ( on )
		setValue(m_project, "VERSION", QString("%1.%2.%3.%4")
					.arg(QString::number(sbMajor->value()))
					.arg(QString::number(sbMinor->value()))
					.arg(QString::number(sbRelease->value()))
					.arg(QString::number(sbBuild->value()))
				);
	else
		removeVariable(m_project, "VERSION");
}

void QMakeSettings::on_sbMajor_valueChanged(int val)
{
	setValue(m_project, "VERSION", QString("%1.%2.%3.%4")
					.arg(QString::number(sbMajor->value()))
					.arg(QString::number(sbMinor->value()))
					.arg(QString::number(sbRelease->value()))
					.arg(QString::number(sbBuild->value()))
			);
}

void QMakeSettings::on_sbMinor_valueChanged(int val)
{
	setValue(m_project, "VERSION", QString("%1.%2.%3.%4")
					.arg(QString::number(sbMajor->value()))
					.arg(QString::number(sbMinor->value()))
					.arg(QString::number(sbRelease->value()))
					.arg(QString::number(sbBuild->value()))
			);
}

void QMakeSettings::on_sbRelease_valueChanged(int val)
{
	setValue(m_project, "VERSION", QString("%1.%2.%3.%4")
					.arg(QString::number(sbMajor->value()))
					.arg(QString::number(sbMinor->value()))
					.arg(QString::number(sbRelease->value()))
					.arg(QString::number(sbBuild->value()))
			);
}

void QMakeSettings::on_sbBuild_valueChanged(int val)
{
	setValue(m_project, "VERSION", QString("%1.%2.%3.%4")
					.arg(QString::number(sbMajor->value()))
					.arg(QString::number(sbMinor->value()))
					.arg(QString::number(sbRelease->value()))
					.arg(QString::number(sbBuild->value()))
			);
}

void QMakeSettings::on_lwQtModules_itemChanged(QListWidgetItem *i)
{
	if ( !i )
		return;
	
	static QStringList strongRem = QStringList()
		<< "core"
		<< "gui";
	
	bool enabled = i->checkState() & Qt::Checked;
	QString var = "QT", equiv = i->text().remove("Qt").toLower();
	
	if ( i->listWidget()->row(i) >= 12 )
	{
		// special handling for some modules
		var = "CONFIG";
		
		if ( equiv == "test" )
			equiv = "qtestlib";
	}
	
	if ( enabled )
	{
		addValue(m_project, var, equiv);
	} else {
		removeValue(m_project, var, equiv, strongRem.contains(equiv));
	}
}

static void contextMenu(QListWidget *lw, const QPoint& p,
						QProject *project, const QString& var)
{
	QAction *add, *remove;
	
	add = new QAction(QMakeSettings::tr("Add..."), lw);
	
	remove = new QAction(QMakeSettings::tr("Remove"), lw);
	remove->setEnabled(lw->currentItem());
	
	QMenu m;
	m.addAction(add);
	m.addAction(remove);
	
	QAction *a = m.exec(lw->mapToGlobal(p));
	
	if ( a == add )
	{
		QString val = QInputDialog::getText(0,
											QMakeSettings::tr("Add new value"),
											QMakeSettings::tr("Type new value : "));
		
		lw->addItem(val);
		addValue(project, var, val);
	} else if ( a == remove ) {
		QListWidgetItem *i = lw->takeItem(lw->currentRow());
		removeValue(project, var, i->text());
	}
	
	delete add;
	delete remove;
}

void QMakeSettings::on_lwDefines_customContextMenuRequested(const QPoint& p)
{
	contextMenu(lwDefines, p, m_project, "DEFINES");
}

void QMakeSettings::on_lwIncludes_customContextMenuRequested(const QPoint& p)
{
	contextMenu(lwIncludes, p, m_project, "INCLUDEPATH");
}

void QMakeSettings::on_lwLibraries_customContextMenuRequested(const QPoint& p)
{
	contextMenu(lwLibraries, p, m_project, "LIBS");
}

static void removeVariable(QProject *p, const QString& var)
{
	if ( !p )
		return;
	
	int idx = -1;
	
	while ( (idx + 1) < p->children().count() )
	{
		INode *i = INode::fromNode(p->children().at(++idx));
		
		if ( i->type != INode::Variable )
			continue;
		
		int nidx = i->data.lastIndexOf(' ');
		
		QString op = i->data.mid(nidx);
		
		if ( i->data.left(nidx) != var )
			continue;
		
		delete p->children().at(idx);
	}
}

static void setValue(QProject *p, const QString& var, const QString& val)
{
	if ( !p )
		return;
	
	int idx = -1;
	INode *first = 0;
	
	while ( (idx + 1) < p->children().count() )
	{
		INode *i = INode::fromNode(p->children().at(++idx));
		
		if ( i->type != INode::Variable )
			continue;
		
		int nidx = i->data.lastIndexOf(' ');
		
		QString op = i->data.mid(nidx);
		
		if ( (op == "~=") || (op == "-=") || (i->data.left(nidx) != var) )
			continue;
		
		if ( first )
		{
			delete p->children().at(idx);
		} else {
			first = i;
			
			// remove old values;
			p->children().at(idx)->clear();
			qDeleteAll(i->children);
		}
	}
	
	if ( !first && val.count() )
	{
		//qDebug("creating variable %s", qPrintable(var));
		first = new INode(INode::Variable, var + " =");
		
		setNextNodeType(QProjectNode::Folder);
		INodeBackingStore::instance()->appendChild(INode::fromNode(p),
													first,
													NodeProvider);
	}
	
	if ( first && val.count() )
	{
		INode *value = new INode(INode::Value, val);
		
		setNextNodeType(QProjectNode::Other);
		INodeBackingStore::instance()->appendChild(first,
													value,
													NodeProvider);
	}
}

static void addValue(QProject *p, const QString& var, const QString& val)
{
	if ( !p )
		return;
	
	INode *first = 0;
	bool found = false;
	
	foreach ( QProjectNode *n, p->children() )
	{
		INode *i = INode::fromNode(n);
		
		if ( i->type != INode::Variable )
			continue;
		
		int idx = i->data.lastIndexOf(' ');
		
		QString op = i->data.mid(idx);
		
		if ( (op == "~=") || (i->data.left(idx) != var) )
			continue;
		
		if ( (op == "=") || (op == "*=") || (op == "+=") || (!first && (op != "-=")) )
			first = i;
		
		foreach ( QProjectNode *v, n->children() )
		{
			if ( v->name() == val )
			{
				if ( op == "-=" )
				{
					INodeBackingStore::instance()
											->removeChild(
													i,
													INode::fromNode(v)
												);
					
				} else {
					found = true;
				}
			}
		}
	}
	
	if ( found )
		return;
	
	if ( !first && val.count() )
	{
		first = new INode(INode::Variable, var + " *=");
		
		setNextNodeType(QProjectNode::Folder);
		INodeBackingStore::instance()->appendChild(INode::fromNode(p),
													first,
													NodeProvider);
	}
	
	if ( first && val.count() )
	{
		INode *value = new INode(INode::Value, val);
		
		setNextNodeType(QProjectNode::Folder);
		INodeBackingStore::instance()->appendChild(first,
													value,
													NodeProvider);
	}
}

static void removeValue(QProject *p, const QString& var, const QString& val, bool fullRemove)
{
	if ( !p )
		return;
	
	bool found = false;
	INode *first = 0;
	
	foreach ( QProjectNode *n, p->children() )
	{
		INode *i = INode::fromNode(n);
		
		if ( i->type != INode::Variable )
			continue;
		
		int idx = i->data.lastIndexOf(' ');
		
		QString op = i->data.mid(idx);
		
		if ( (op == "~=") || (i->data.left(idx) != var) )
			continue;
		
		if ( op == "-=" )
		{
			if ( !first )
				continue;
			
			first = i;
		}
		
		foreach ( QProjectNode *v, n->children() )
		{
			if ( v->name() == val )
			{
				INodeBackingStore::instance()
										->removeChild(
												i,
												INode::fromNode(v)
											);
				
				found = true;
			}
		}
	}
	
	if ( (found && !fullRemove) || val.isEmpty() )
		return;
	
	if ( !first )
	{
		first = new INode(INode::Variable, var + " -=");
		
		setNextNodeType(QProjectNode::Folder);
		INodeBackingStore::instance()->appendChild(INode::fromNode(p),
													first,
													NodeProvider);
	}
	
	if ( first )
	{
		INode *value = new INode(INode::Value, val);
		
		setNextNodeType(QProjectNode::Folder);
		INodeBackingStore::instance()->appendChild(first,
													value,
													NodeProvider);
	}
}
