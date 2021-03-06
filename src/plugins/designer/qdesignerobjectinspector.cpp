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

#include "qdesignerobjectinspector.h"

#include "qdesignerperspective.h"

#include <QDesignerComponents>

#include <QDesignerObjectInspectorInterface>
#include <QDesignerFormEditorInterface>
#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>

QDesignerObjectInspector::QDesignerObjectInspector(QDesignerPerspective *p)
 : QDockWidget(0), pParent(p), pForm(0)
{
	setWindowTitle(tr("Object Inspector"));
	setObjectName("x-designer/objectinspector");
	
	pInterface = QDesignerComponents::createObjectInspector(p->handler(), this);
	
	connect(p->handler()->formWindowManager(),
			SIGNAL( activeFormWindowChanged(QDesignerFormWindowInterface*) ),
			this,
			SLOT  ( formChanged(QDesignerFormWindowInterface*) ) );
	
	
	setWidget(pInterface);
	
	p->handler()->setObjectInspector(pInterface);
}

QDesignerObjectInspector::~QDesignerObjectInspector()
{
	
}

void QDesignerObjectInspector::retranslate()
{
	setWindowTitle(tr("Object Inspector"));
}

void QDesignerObjectInspector::formChanged(QDesignerFormWindowInterface *w)
{
	pForm = w;
	pInterface->setFormWindow(pForm);
}
