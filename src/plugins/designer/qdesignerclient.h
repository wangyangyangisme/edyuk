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

#ifndef _DESIGNER_CLIENT_H_
#define _DESIGNER_CLIENT_H_

#include "qmdiwidget.h"

#include <QPointer>

class QString;
class QMdiArea;
class QFormBuilder;
class QDesignerFormWindowInterface;

class QDesignerClient : public qmdiWidget
{
	Q_OBJECT
	
	public:
		QDesignerClient(QDesignerFormWindowInterface *i, const QString& f,
						QWidget *w = 0);
		virtual ~QDesignerClient();
		
		QDesignerFormWindowInterface* interface() const;
		
	public slots:
		void preview();
		virtual void save();
		virtual void print();
		
	protected:
		virtual bool eventFilter(QObject *o, QEvent *e);
		
	private slots:
		void updateChanged();
		void geometryChanged();
		void widgetActivated(QWidget *widget);
		
	private:
		QSize m_geom;
		QPointer<QWidget> m_previewWidget;
		
		QAction *aUndo, *aRedo,
				*aCut, *aCopy, *aPaste, *aDelete,
				*aVertical, *aHorizontal, *aGrid, *aBreak,
				*aAdjust,
				*aLower, *aRaise,
				*aSplitH, *aSplitV,
				*aPreview;
		
		QMdiArea *m_area;
		QDesignerFormWindowInterface *pInterface;
};

#endif // _DESIGNER_CLIENT_H_
