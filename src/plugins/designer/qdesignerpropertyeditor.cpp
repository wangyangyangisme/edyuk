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

#include "qdesignerpropertyeditor.h"

#include "qdesignerperspective.h"

#include <QDesignerComponents>

#include <QDesignerPropertyEditorInterface>
#include <QDesignerFormEditorInterface>
#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowCursorInterface>
#include <QDesignerFormWindowManagerInterface>

QDesignerPropertyEditor::QDesignerPropertyEditor(QDesignerPerspective *p)
 : QDockWidget(0), pParent(p), pForm(0)
{
	setWindowTitle(tr("Properties"));
	setObjectName("x-designer/propertyeditor");
	
	pInterface = QDesignerComponents::createPropertyEditor(p->handler(), this);
	
	connect(p->handler()->formWindowManager(),
			SIGNAL( activeFormWindowChanged(QDesignerFormWindowInterface*) ),
			this,
			SLOT  ( formChanged(QDesignerFormWindowInterface*) ) );
	
	setWidget(pInterface);
	
	p->handler()->setPropertyEditor(pInterface);
}

QDesignerPropertyEditor::~QDesignerPropertyEditor()
{
	
}

void QDesignerPropertyEditor::retranslate()
{
	setWindowTitle(tr("Properties"));
}

void QDesignerPropertyEditor::selectionChanged()
{
	if ( !pForm )
		return;
	
	QDesignerFormWindowCursorInterface *c = pForm->cursor();
	
	if ( c->hasSelection() )
		pInterface->setObject(c->selectedWidget(0));
	else
		pInterface->setObject(pForm);
}

void QDesignerPropertyEditor::formChanged(QDesignerFormWindowInterface *w)
{
	if ( pForm )
	{
		disconnect(	pForm	, SIGNAL( selectionChanged() ),
					this	, SLOT  ( selectionChanged() ) );
		
	}
	
	pForm = w;
	pInterface->setObject(pForm);
	
	if ( pForm )
	{
		connect(pForm	, SIGNAL( selectionChanged() ),
				this	, SLOT  ( selectionChanged() ) );
		
	}
}
