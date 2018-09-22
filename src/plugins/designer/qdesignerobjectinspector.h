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

#ifndef _QDESIGNER_OBJECT_INSPECTOR_H_
#define _QDESIGNER_OBJECT_INSPECTOR_H_

#include <QDockWidget>

class QDesignerPerspective;
class QDesignerObjectInspectorInterface;
class QDesignerFormWindowInterface;

class QDesignerObjectInspector : public QDockWidget
{
	Q_OBJECT
	
	public:
		QDesignerObjectInspector(QDesignerPerspective *p);
		virtual ~QDesignerObjectInspector();
		
		virtual void retranslate();
		
	private slots:
		void formChanged(QDesignerFormWindowInterface *w);
		
	private:
		QDesignerPerspective *pParent;
		QDesignerFormWindowInterface *pForm;
		QDesignerObjectInspectorInterface *pInterface;
};

#endif // _QDESIGNER_OBJECT_INSPECTOR_H_
