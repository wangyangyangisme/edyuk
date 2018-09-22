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

#ifndef _QDESIGNER_PERSPECTIVE_H_
#define _QDESIGNER_PERSPECTIVE_H_

#include "qmdiperspective.h"

#include <QPointer>
#include <QBasicTimer>

class QAction;
class QActionGroup;

class QDockWidget;

class QDesignerWidgetBox;
class QDesignerActionEditor;
class QDesignerPropertyEditor;
class QDesignerObjectInspector;
class QDesignerSignalSlotEditor;

class QDesignerFormEditorInterface;
class QDesignerFormWindowInterface;

class QDesignerPerspective : public qmdiPerspective
{
	Q_OBJECT
	
	public:
		QDesignerPerspective();
		virtual ~QDesignerPerspective();
		
		virtual void retranslate();
		
		virtual QIcon icon() const;
		virtual QString name() const;
		
		virtual Affinity affinity(qmdiClient *c) const;
		
		virtual QStringList filters() const;
		virtual qmdiClient* open(const QString& filename);
		virtual bool canOpen(const QString& filename) const;
		
		virtual qmdiClient* createEmptyClient(qmdiClientFactory *f);
		
		QDesignerFormEditorInterface* handler();
		QDesignerFormWindowInterface* createForm();
		
	protected:
		virtual void setMainWindow(qmdiMainWindow *w);
		
	private slots:
		void editWidgets();
		void selectionChanged();
		void formChanged(QWidget *w);
		void activeFormWindowChanged(QDesignerFormWindowInterface* w);
		
	private:
		QAction *aEditWidgets;
		QActionGroup *aModes;
		
		QDesignerFormEditorInterface *pDesigner;
		
		QDesignerWidgetBox *pWidgetBox;
		QDesignerActionEditor *pActionEditor;
		QDesignerPropertyEditor *pPropertyEditor;
		QDesignerObjectInspector *pObjectInspector;
		QDesignerSignalSlotEditor *pSignalSlotEditor;
};

#endif // _QDESIGNER_PERSPECTIVE_H_
