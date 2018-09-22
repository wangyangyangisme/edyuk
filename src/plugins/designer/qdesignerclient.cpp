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

#include "qdesignerclient.h"

#include "qmdiserver.h"

#include <QFile>
#include <QTimer>
#include <QAction>
#include <QString>
#include <QBuffer>
#include <QMdiArea>
#include <QKeyEvent>
#include <QTextStream>
#include <QTimerEvent>
#include <QVBoxLayout>
#include <QMdiSubWindow>

#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>

#include <QFormBuilder>
#include <QDesignerFormEditorInterface>
#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>

#include <QUndoStack>
#include <QtDesigner>

//#include "qdesigner_formbuilder_p.h"

class InnerMdiArea : public QMdiArea
{
	public:
		InnerMdiArea(QWidget* parent = 0)
		 : QMdiArea(parent)
		{}
		
		bool eventFilter(QObject* object, QEvent* event)
		{
			return event->type() == QEvent::ApplicationActivate
				|| event->type() == QEvent::ApplicationDeactivate;
		}
};

QDesignerClient::QDesignerClient(QDesignerFormWindowInterface *i,
								const QString& f, QWidget *w)
 : qmdiWidget(w), pInterface(i)
{
	if ( !pInterface )
		qFatal("Can't instantiate a designer client with NULL interface");
	
	setPrintable(true);
	
	m_area = new InnerMdiArea(this);
	
	QVBoxLayout *l = new QVBoxLayout(this);
	l->setMargin(0);
	l->addWidget(m_area);
	
	if ( !f.isEmpty() && QFile::exists(f) )
	{
		QFile file(f);
		setFileName(f);
		
		pInterface->setFileName(f);
		pInterface->setContents(&file);
		
		// make sure the geometry won't be altered when adding the client to the workspace...
		m_geom = pInterface->mainContainer()->size();
		//pInterface->setProperty("lockGeometry", true);
		
		pInterface->setDirty(false);
	} else {
		m_geom = QSize(400, 300);
	}
	
	QMdiSubWindow *sub = m_area->addSubWindow(pInterface);
	sub->installEventFilter(this);
	
	const QSize decorationSize = sub->size() - sub->contentsRect().size();
	sub->resize(m_geom + decorationSize);
	
	connect(pInterface->commandHistory(), SIGNAL( indexChanged(int) ),
			this						, SLOT  ( updateChanged() ) );
	
	connect(pInterface	, SIGNAL( geometryChanged() ),
			this		, SLOT  ( geometryChanged() ) );
	
	connect(pInterface	, SIGNAL( activated(QWidget*) ),
			this		, SLOT  ( widgetActivated(QWidget*) ) );
	
	QDesignerFormWindowManagerInterface *m;
	m = pInterface->core()->formWindowManager();
	
	aUndo = m->actionUndo();
	aUndo->setIcon( QIcon(":/undo.png") );
	aRedo = m->actionRedo();
	aRedo->setIcon( QIcon(":/redo.png") );
	
	aCut = m->actionCut();
	aCopy = m->actionCopy();
	aPaste = m->actionPaste();
	aDelete = m->actionDelete();
	
	aVertical = m->actionVerticalLayout();
	aHorizontal = m->actionHorizontalLayout();
	aGrid = m->actionGridLayout();
	aSplitH = m->actionSplitHorizontal();
	aSplitV = m->actionSplitVertical();
	aBreak = m->actionBreakLayout();
	aAdjust = m->actionAdjustSize();
	
	aLower = m->actionLower();
	aRaise = m->actionRaise();
	
	aPreview = new QAction(QIcon(":/preview.png"), tr("&Preview"), this);
	
	connect(aPreview, SIGNAL( triggered() ),
			this	, SLOT  ( preview() ) );
	
	menus["&Edit"]->addAction(aUndo);
	menus["&Edit"]->addAction(aRedo);
	menus["&Edit"]->addSeparator();
	menus["&Edit"]->addAction(aCut);
	menus["&Edit"]->addAction(aCopy);
	menus["&Edit"]->addAction(aPaste);
	menus["&Edit"]->addAction(aDelete);
	menus["&Edit"]->addSeparator();
	menus["&Edit"]->addAction(aVertical);
	menus["&Edit"]->addAction(aHorizontal);
	menus["&Edit"]->addAction(aGrid);
	menus["&Edit"]->addAction(aSplitV);
	menus["&Edit"]->addAction(aSplitH);
	menus["&Edit"]->addAction(aBreak);
	menus["&Edit"]->addAction(aAdjust);
	menus["&Edit"]->addSeparator();
	menus["&Edit"]->addAction(aPreview);
	
	toolbars["Edit"]->addAction(aUndo);
	toolbars["Edit"]->addAction(aRedo);
	toolbars["Edit"]->addSeparator();
	toolbars["Edit"]->addAction(aCut);
	toolbars["Edit"]->addAction(aCopy);
	toolbars["Edit"]->addAction(aPaste);
	toolbars["Edit"]->addAction(aDelete);
	toolbars["Edit"]->addSeparator();
	toolbars["Edit"]->addAction(aVertical);
	toolbars["Edit"]->addAction(aHorizontal);
	toolbars["Edit"]->addAction(aGrid);
	toolbars["Edit"]->addAction(aSplitV);
	toolbars["Edit"]->addAction(aSplitH);
	toolbars["Edit"]->addAction(aBreak);
	toolbars["Edit"]->addAction(aAdjust);
	toolbars["Edit"]->addSeparator();
	toolbars["Edit"]->addAction(aPreview);
	
	//t.start(1000, this);
	
	if ( !f.isEmpty() && QFile::exists(f) )
		pInterface->setDirty(false);
}

QDesignerClient::~QDesignerClient()
{
	
}

bool QDesignerClient::eventFilter(QObject *o, QEvent *e)
{
	bool lock = false;
	
	if ( !lock && (o == m_area->activeSubWindow()) && e && (e->type() == QEvent::Close) )
	{
		lock = true;
		close();
		lock = false;
		
		return true;
	}
	
	if ( o == m_previewWidget )
	{
		switch ( e->type() )
		{
			case QEvent::KeyPress:
			case QEvent::ShortcutOverride:
			{
				const QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(e);
				const int key = keyEvent->key();
				
				if (
					(key == Qt::Key_Escape)
				#ifdef Q_WS_MAC
					|| (keyEvent->modifiers() == Qt::ControlModifier && key == Qt::Key_Period)
				#endif
					) {
					m_previewWidget->close();
					return true;
				}
			}
				break;
			default:
				break;
		}
	}
	return qmdiWidget::eventFilter(o, e);
}

void QDesignerClient::save()
{
	if ( fileName().isEmpty() )
	{
		if ( server() )
			return server()->saveClientAs(this);
		
		return;
	}
	
	QFile f( fileName() );
	QTextStream out(&f);
	
	if ( !f.open(QFile::WriteOnly | QFile::Text) )
		return;
	
	out << pInterface->contents();
	
	//t.stop();
	setContentModified(false);
	pInterface->setDirty(false);
	//t.start(1000, this);
}

void QDesignerClient::print()
{
	// get printer
	QPrinter p;
	bool b = false;
 
	// if quick print
	if ( b )
	{
		// check if default printer is set
		if ( p.printerName().isEmpty() )
		{
			//warning( QObject::tr( "Quick Print..." ), QObject::tr( "There is no defaullt printer, please set one before trying quick print" ), pInterface->window() );
			return; 
		}
 
		// print and return
		QPainter pr(&p);
		pr.drawPixmap(0, 0, QPixmap::grabWidget(pInterface));
		return;
	}
	
	// printer dialog
	QPrintDialog d(&p);
 
	// if ok
	if ( d.exec() )
	{
		// print and return
		QPainter pr(&p);
		pr.drawPixmap(0, 0, QPixmap::grabWidget(pInterface));
	}
}

void QDesignerClient::preview()
{
	if ( m_previewWidget )
	{
		m_previewWidget->close();
	}
	
	// Get style stored in action if any
	QString styleName;
	/*
	if ( action )
	{
		const QVariant data = action->data();
		
		if ( data.type() == QVariant::String )
			styleName = data.toString();
	}
	*/
	
	// create and have form builder display errors.
	QBuffer buffer;
	buffer.setData(pInterface->contents().toLocal8Bit());
	
	m_previewWidget = QFormBuilder().load(&buffer);
	
#ifdef Q_WS_WIN
	Qt::WindowFlags windwowFlags =
				(m_previewWidget->windowType() == Qt::Window)
			?
				Qt::Window | Qt::WindowMaximizeButtonHint
			:
				Qt::WindowFlags(Qt::Dialog);
#else
	// Only Dialogs have close buttons on mac
	// On linux we don't want additional item on task bar and don't want minimize button,
	// we want the preview to be on top
	Qt::WindowFlags windwowFlags = Qt::Dialog;
#endif
	// Install filter for Escape key
	m_previewWidget->setParent(pInterface->window(), windwowFlags);
	
	// Cannot do this on the Mac as the dialog would have no close button
	m_previewWidget->setWindowModality(Qt::ApplicationModal);
	
	// Position over form window
	m_previewWidget->setAttribute(Qt::WA_DeleteOnClose, true);
	m_previewWidget->move(pInterface->mapToGlobal(QPoint(10, 10)));
	
	m_previewWidget->installEventFilter(this);
	m_previewWidget->show();
}

QDesignerFormWindowInterface* QDesignerClient::interface() const
{
	return pInterface;
}

void QDesignerClient::updateChanged()
{
	setContentModified(pInterface->isDirty());
}

void QDesignerClient::widgetActivated(QWidget *widget)
{
	const QDesignerTaskMenuExtension *taskMenu =
		qt_extension<QDesignerTaskMenuExtension*>(pInterface->core()->extensionManager(), widget);
	
	/*
	if ( widget )
		pInterface
			->core()
			->propertyEditor()
			->setObject(widget);
	*/
	
	if ( taskMenu )
	{
		QAction *action = taskMenu->preferredEditAction();
		
		if ( !action )
		{
			const QList<QAction*> actions = taskMenu->taskActions();
			
			if ( !actions.isEmpty() )
				action = actions.first();
		}
		
		if ( action )
		{
			QTimer::singleShot(0, action, SIGNAL( triggered() ));
		}
	}
}

void QDesignerClient::geometryChanged()
{
	QDesignerFormEditorInterface *c = pInterface->core();
	
	if ( pInterface->property("lockGeometry").toBool() )
	{
			pInterface->setProperty("lockGeometry", false);
			return;
	}
	
	if ( QObject *o = c->propertyEditor()->object() )
	{
		QDesignerPropertySheetExtension *sheet = 
			qt_extension<QDesignerPropertySheetExtension*>(c->extensionManager(), o);
		
		c->propertyEditor()->setPropertyValue("geometry", sheet->property(sheet->indexOf("geometry")));
	}
}
