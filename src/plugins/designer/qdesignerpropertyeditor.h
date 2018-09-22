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

#ifndef _QDESIGNER_PROPERTY_EDITOR_H_
#define _QDESIGNER_PROPERTY_EDITOR_H_

#include <QDockWidget>

class QString;
class QVariant;

class QDesignerPerspective;
class QDesignerPropertyEditorInterface;
class QDesignerFormWindowInterface;

class QDesignerPropertyEditor : public QDockWidget
{
	Q_OBJECT
	
	public:
		QDesignerPropertyEditor(QDesignerPerspective *p);
		virtual ~QDesignerPropertyEditor();
		
		virtual void retranslate();
		
	private slots:
		void selectionChanged();
		void formChanged(QDesignerFormWindowInterface *w);
		
	private:
		QDesignerPerspective *pParent;
		QDesignerFormWindowInterface *pForm;
		QDesignerPropertyEditorInterface *pInterface;
};

#endif // _QDESIGNER_PROPERTY_EDITOR_H_
