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

#include "qdesignerwidgetbox.h"

#include "qdesignerperspective.h"

#include <QDir>

#include <QDesignerComponents>

#include <QDesignerWidgetBoxInterface>
#include <QDesignerFormEditorInterface>

QDesignerWidgetBox::QDesignerWidgetBox(QDesignerPerspective *p)
 : QDockWidget(0), pParent(p)
{
	setWindowTitle(tr("Widget Box"));
	setObjectName("x-designer/widgetbox");
	
	pInterface = QDesignerComponents::createWidgetBox(p->handler(), this);
	
	pInterface->setFileName(":/trolltech/widgetbox/widgetbox.xml");
	pInterface->load();
	
	pInterface->setFileName(QDir::homePath() + "./designer/widgetbox.xml");
	pInterface->load();
	
	setWidget(pInterface);
	
	p->handler()->setWidgetBox(pInterface);
}

QDesignerWidgetBox::~QDesignerWidgetBox()
{
	
}

void QDesignerWidgetBox::retranslate()
{
	setWindowTitle(tr("Widget Box"));
}
