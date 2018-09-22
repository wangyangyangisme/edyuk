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

#ifndef _QMDI_WIDGET_H_
#define _QMDI_WIDGET_H_

#include "qmdi.h"

#include <qwidget.h>
#include "qmdiclient.h"


/*!
	\file qmdiwidget.h
	\brief Definition of the qmdiWidget class
	
	\see qmdiWidget
*/

class QMDI_API qmdiWidget : public QWidget, public qmdiClient
{
	Q_OBJECT
	
	public:
		qmdiWidget(QWidget *p);
		qmdiWidget(QWidget *p, qmdiServer *s);
		virtual ~qmdiWidget();
		
	public slots:
		void setTitle(const QString& title);
		
	signals:
		void contentModified(bool y);
		void titleChanged(const QString& title);
		
	protected slots:
		virtual void setContentModified(bool y);
		virtual void setFileName(const QString& f);
		
	protected:
		virtual void closeEvent(QCloseEvent *e);
};

#endif
