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

#ifndef _QMDI_STATUS_BAR_H_
#define _QMDI_STATUS_BAR_H_

#include "qmdi.h"

/*!
	\file qmdistatusbar.h
	
	\brief Definition of the qmdiStatusBar class.
*/

#include <QStatusBar>

class QAbstractButton;

class QMDI_API qmdiStatusBar : public QStatusBar
{
	public:
		qmdiStatusBar(QWidget *p = 0);
		virtual ~qmdiStatusBar();
		
	public slots:
		void addButton(QAbstractButton *b);
		void removeButton(QAbstractButton *b);
		
	private:
		QList<QAbstractButton*> buttons;
};

#endif // _QMDI_STATUS_BAR_H_
