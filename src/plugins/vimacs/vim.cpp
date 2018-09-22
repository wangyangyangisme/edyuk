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

#include "vim.h" 

Vim::Vim()
{
	
}

Vim::~Vim()
{
	
}

QString Vim::id() const
{
	return "vim";
}

QString Vim::name() const
{
	return "vim";
}

bool Vim::keyPressEvent(QKeyEvent *event, QEditor *editor)
{
	return true;
}

bool Vim::inputMethodEvent(QInputMethodEvent* event, QEditor *editor)
{
	return false;
}

bool Vim::mouseMoveEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}

bool Vim::mousePressEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}

bool Vim::mouseReleaseEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}

bool Vim::mouseDoubleClickEvent(QMouseEvent *event, QEditor *editor)
{
	return false;
}
