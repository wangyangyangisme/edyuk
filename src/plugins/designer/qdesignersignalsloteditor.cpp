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

#include "qdesignersignalsloteditor.h"

#include "qdesignerperspective.h"

#include <QDesignerComponents>

#include <QDesignerFormEditorInterface>
#include <QDesignerFormWindowInterface>
#include <QDesignerFormWindowManagerInterface>

QDesignerSignalSlotEditor::QDesignerSignalSlotEditor(QDesignerPerspective *p)
 : QDockWidget(0), pParent(p)
{
	setObjectName("x-designer/signalsloteditor");
	setWindowTitle(tr("Signal and slots"));
	
	pInterface = QDesignerComponents::createSignalSlotEditor(p->handler(), this);
	setWidget(pInterface);
}

QDesignerSignalSlotEditor::~QDesignerSignalSlotEditor()
{
	
}

void QDesignerSignalSlotEditor::retranslate()
{
	setWindowTitle(tr("Signals and slots"));
}
