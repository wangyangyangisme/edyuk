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

#include "qdesignerperspective.h"

#include "qdesignerclient.h"

#include "qdesignerwidgetbox.h"
#include "qdesigneractioneditor.h"
#include "qdesignerpropertyeditor.h"
#include "qdesignerobjectinspector.h"
#include "qdesignersignalsloteditor.h"

#include "qmdiworkspace.h"
#include "qmdimainwindow.h"

#include <QIcon>
#include <QAction>
#include <QTimerEvent>
#include <QPluginLoader>

#include <QDesignerComponents>

#include <QDesignerFormEditorInterface>
#include <QDesignerActionEditorInterface>
#include <QDesignerPropertyEditorInterface>
#include <QDesignerObjectInspectorInterface>
#include <QDesignerFormWindowCursorInterface>
#include <QDesignerFormEditorPluginInterface>
#include <QDesignerFormWindowManagerInterface>

#include "qdesignerinternals.h"

QDesignerPerspective::QDesignerPerspective()
 : qmdiPerspective(0)
{
	QDesignerComponents::initializeResources();
	
	pDesigner = QDesignerComponents::createFormEditor(this);
	
	(void)QDesignerComponents::createTaskMenu(pDesigner, this);
	
	aModes = new QActionGroup(this);
	aModes->setExclusive(true);
	
	aEditWidgets = new QAction(tr("Edit Widgets"), this);
	aEditWidgets->setCheckable(true);
	aEditWidgets->setIcon(QIcon(pDesigner->resourceLocation() + "/widgettool.png"));
	
	connect(aEditWidgets, SIGNAL( triggered() ),
			this		, SLOT  ( editWidgets() ) );
	
	aEditWidgets->setChecked(true);
	
	aModes->addAction(aEditWidgets);
	menus["&Mode"]->addAction(aEditWidgets);
	toolbars["Mode"]->addAction(aEditWidgets);
	
	QList<QObject*> plugins = SafetyNet::QDesignerInternals::pluginInstances(pDesigner);
	
	foreach (QObject *plugin, plugins)
	{
		QDesignerFormEditorPluginInterface *fep = qobject_cast<QDesignerFormEditorPluginInterface*>(plugin);
		
		if ( fep )
		{
			if ( !fep->isInitialized() )
				fep->initialize(pDesigner);
			
			QAction *a = fep->action();
			
			if ( a )
			{
				a->setCheckable(true);
				
				aModes->addAction(a);
				menus["&Mode"]->addAction(a);
				toolbars["Mode"]->addAction(a);
			}
		}
	}
	
	aModes->setEnabled(false);
	
	QDesignerComponents::initializePlugins(pDesigner);
	
	pWidgetBox = new QDesignerWidgetBox(this);
	addDockWidget(pWidgetBox, "Widget box", Qt::LeftDockWidgetArea);
	
	pObjectInspector = new QDesignerObjectInspector(this);
	addDockWidget(pObjectInspector, "Object inspector", Qt::RightDockWidgetArea);
	
	pPropertyEditor = new QDesignerPropertyEditor(this);
	addDockWidget(pPropertyEditor, "Property Editor", Qt::RightDockWidgetArea);
	
	pActionEditor = new QDesignerActionEditor(this);
	addDockWidget(pActionEditor, "Action editor", Qt::BottomDockWidgetArea);
	
	pSignalSlotEditor = new QDesignerSignalSlotEditor(this);
	addDockWidget(pSignalSlotEditor, "Signals/Slots editor", Qt::TopDockWidgetArea);
	
	//qdesigner_internal::QDesignerIntegration *pIntegration =
	//	new qdesigner_internal::QDesignerIntegration(pDesigner, this);
	SafetyNet::QDesignerInternals::createIntegration(pDesigner, this);
	
	connect(pDesigner->formWindowManager(),
			SIGNAL( activeFormWindowChanged(QDesignerFormWindowInterface*) ),
			this,
			SLOT  ( activeFormWindowChanged(QDesignerFormWindowInterface*) )
		);
	
}


QDesignerPerspective::~QDesignerPerspective()
{
	
}

QIcon QDesignerPerspective::icon() const
{
	return QIcon(":/designer.png");
}

QString QDesignerPerspective::name() const
{
	return tr("Integrated Qt4 Designer");
}

void QDesignerPerspective::retranslate()
{
	pWidgetBox->retranslate();
	pPropertyEditor->retranslate();
	pObjectInspector->retranslate();
	pActionEditor->retranslate();
	pSignalSlotEditor->retranslate();
	
	qmdiPerspective::retranslate();
}

void QDesignerPerspective::setMainWindow(qmdiMainWindow *w)
{
	qmdiPerspective::setMainWindow(w);
	
	if ( !w )
		return;
	
	connect(w->workspace()	, SIGNAL( indexChanged(QWidget*) ),
			this			, SLOT  ( formChanged(QWidget*) ) );
	
	pDesigner->setTopLevel(w);
}


QStringList QDesignerPerspective::filters() const
{
	QStringList l;
	
	l
		<< tr("Qt Designer forms ( *.ui )")
		;
	
	return l;
}

qmdiPerspective::Affinity QDesignerPerspective::affinity(qmdiClient *c) const
{
	if ( dynamic_cast<QDesignerClient*>(c) )
		return qmdiPerspective::Exclusive;
	
	return qmdiPerspective::None;
}

qmdiClient* QDesignerPerspective::open(const QString& filename)
{
	if ( filename.endsWith(".ui") )
	{
		QDesignerClient *c = new QDesignerClient(createForm(), filename);
		c->setPerspective(this);
		
		return c;
	}
	
	return 0;
}

bool QDesignerPerspective::canOpen(const QString& filename) const
{
	return filename.endsWith(".ui");
}

QDesignerFormEditorInterface* QDesignerPerspective::handler()
{
	return pDesigner;
}

QDesignerFormWindowInterface* QDesignerPerspective::createForm()
{
	QDesignerFormWindowManagerInterface* m = pDesigner->formWindowManager();
	QDesignerFormWindowInterface* w = m->createFormWindow();
	
	w->setFeatures(QDesignerFormWindowInterface::DefaultFeature);
	return w;
}

void QDesignerPerspective::formChanged(QWidget *w)
{
	QDesignerFormWindowInterface *i = 0;
	QDesignerClient *c = qobject_cast<QDesignerClient*>(w);
	
	if ( c )
		i = c->interface();
	
	aModes->setEnabled(i);
	
	pDesigner->formWindowManager()->setActiveFormWindow(i);
}

void QDesignerPerspective::activeFormWindowChanged(QDesignerFormWindowInterface* w)
{
	// disconnect form selection changed
	if ( w )
	{
		disconnect(w, SIGNAL( selectionChanged() ), this, SLOT( selectionChanged() ) );
	}
 
	// inspector
	pDesigner->objectInspector()->setFormWindow( w );
 
	// set new object for property editor
	pDesigner->propertyEditor()->setObject(w);
 
	// set new object for action editor
	pDesigner->actionEditor()->setFormWindow(w);
 
	// connect form selection changed
	if ( w )
	{
		connect(w, SIGNAL( selectionChanged() ), this, SLOT( selectionChanged() ) );
	}
}

void QDesignerPerspective::selectionChanged()
{
	// set current object for property editor
	if ( QDesignerFormWindowInterface* w = pDesigner->formWindowManager()->activeFormWindow() )
		pDesigner->propertyEditor()->setObject( w->cursor()->hasSelection() ? w->cursor()->selectedWidget( 0 ) : w->mainContainer() );
}

void QDesignerPerspective::editWidgets()
{
	if ( !mainWindow() )
		return;
	
	qmdiWorkspace *w = mainWindow()->workspace();
	
	if ( !w )
		return;
	
	foreach ( QWidget *cw, w->windowList() )
	{
		QDesignerClient *c = qobject_cast<QDesignerClient*>(cw);
		
		if ( !c || !c->interface() )
			continue;
		
		c->interface()->editWidgets();
	}
}

qmdiClient* QDesignerPerspective::createEmptyClient(qmdiClientFactory *f)
{
	Q_UNUSED(f)
	
	/*
		TODO : propose a choice between QDialog, QWidget, QMainWindow, ...
	*/
	
	static int count = 0;
	
	QDesignerFormWindowInterface* w = createForm();
	w->setMainContainer(new QWidget());
	
	QDesignerClient *dc = new QDesignerClient(w, "");
	dc->setPerspective(this);
	dc->setTitle( tr("untitled form %1").arg(++count) );
	w->setDirty(true);
	
	return dc;
}
