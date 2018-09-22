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

#include "qmdiwidget.h"

#include "qmdiserver.h"

#include <qstring.h>
#include <qwidget.h>
#include <QCloseEvent>

/*!
	\file qmdiwidget.cpp
	\class qmdiWidget
	\brief Implementation of the qmdiWidget class
	
	modification of Elcuco's qmdilib for use with Edyuk
	
	\see qmdiClient
	\see QWidget
*/

/*!
	\brief Default constructor
*/
qmdiWidget::qmdiWidget(QWidget *p)
 : QWidget(p), qmdiClient(p ? dynamic_cast<qmdiServer*>(p) : 0)
{
	;
}

/*!
	\brief Overloaded constructor provided for convinience
*/
qmdiWidget::qmdiWidget(QWidget *p, qmdiServer *s)
 : QWidget(p), qmdiClient(s)
{
	;
}

/*!
	\brief Destructor
	
	If the client still belongs to a server it removes itself from it
*/
qmdiWidget::~qmdiWidget()
{
	if ( server() )
		notifyDeletion();
}

/*!
	\brief Sets the client as "modified"
	
	\see isContentModified()
*/
void qmdiWidget::setContentModified(bool y)
{
	qmdiClient::setContentModified(y);
	
	setWindowModified(y);
	emit contentModified(y);
}

/*!
	\brief Sets the client as "modified"
	
	\see isContentModified()
*/
void qmdiWidget::setFileName(const QString& f)
{
	qmdiClient::setFileName(f);
	
	setTitle( name() );
}

/*!

*/
void qmdiWidget::setTitle(const QString& title)
{
	QString s(title);
	
	if ( !s.contains("[*]") )
		s.prepend("[*]");
	
	setWindowTitle(s);
	emit titleChanged(title);
}

/*!

*/
void qmdiWidget::closeEvent(QCloseEvent *e)
{
	bool bOK = true;
	
	if ( isContentModified() )
		bOK = server()->maybeSave(this);
	
	if ( bOK )
	{
		e->accept();
		notifyDeletion();
	} else {
		e->ignore();
	}
}
