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

#include "emacs.h" 

Emacs::Emacs()
{
	
}

Emacs::~Emacs()
{
	
}

QString Emacs::id() const
{
	return "emacs";
}

QString Emacs::name() const
{
	return "emacs";
}

bool Emacs::keyPressEvent(QKeyEvent *event, QEditor *editor)
{
	return false;
}

bool Emacs::inputMethodEvent(QInputMethodEvent* event, QEditor *editor)
{
	return false;
}

bool Emacs::mouseMoveEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}

bool Emacs::mousePressEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}

bool Emacs::mouseReleaseEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}

bool Emacs::mouseDoubleClickEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}
