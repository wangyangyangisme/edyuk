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

#include "qmdistatusbar.h"

/*!
	\file qmdistatusbar.cpp
	\brief Implementation of the DevStatus class.
	
	\see qmdiStatusBar
*/

#include <QAbstractButton>

/*!
	\ingroup mdi
	@{
	
	\class qmdiStatusBar
	\brief A custom status bar used by qmdiMainWindow to store perspective's dock
	widgets toggleViewAction().
	
	\see qmdiMainWindow
	\see qmdiPerspective
*/

/*!
	\brief Constructor
*/
qmdiStatusBar::qmdiStatusBar(QWidget *p)
 : QStatusBar(p)
{
}

/*!
	\brief Destructor
*/
qmdiStatusBar::~qmdiStatusBar()
{
	buttons.clear();
}

/*!
	\brief Removes a persistent button from the right of the status bar
*/
void qmdiStatusBar::removeButton(QAbstractButton *b)
{
	if ( !b )
		return;
	
	removeWidget(b);
	buttons.removeAll(b);
	b->setParent(0);
	b->hide();
}

/*!
	\brief Adds a persistent button at the right of the status bar
*/
void qmdiStatusBar::addButton(QAbstractButton *b)
{
	if ( !b )
		return;
	
	buttons.prepend(b);
	addPermanentWidget(b);
	b->setParent(this);
	b->show();
}

/*! @} */

