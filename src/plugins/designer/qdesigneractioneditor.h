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

#ifndef _QDESIGNER_ACTION_EDITOR_H_
#define _QDESIGNER_ACTION_EDITOR_H_

#include <QDockWidget>

class QDesignerPerspective;
class QDesignerActionEditorInterface;
class QDesignerFormWindowInterface;

class QDesignerActionEditor : public QDockWidget
{
	Q_OBJECT
	
	public:
		QDesignerActionEditor(QDesignerPerspective *p);
		virtual ~QDesignerActionEditor();
		
		virtual void retranslate();
		
	private slots:
		void formChanged(QDesignerFormWindowInterface *w);
		
	private:
		QDesignerPerspective *pParent;
		QDesignerFormWindowInterface *pForm;
		QDesignerActionEditorInterface *pInterface;
};

#endif // _QDESIGNER_ACTION_EDITOR_H_
