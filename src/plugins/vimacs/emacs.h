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

#ifndef _EMACS_H_
#define _EMACS_H_

#include "qeditorinputbinding.h"

class Emacs : public QEditorInputBinding
{
	public:
		Emacs();
		virtual ~Emacs();
		
		virtual QString id() const;
		virtual QString name() const;
		
		virtual bool keyPressEvent(QKeyEvent *event, QEditor *editor);
		virtual bool inputMethodEvent(QInputMethodEvent* event, QEditor *editor);
		virtual bool mouseMoveEvent(QMouseEvent *event, QEditor *editor);
		virtual bool mousePressEvent(QMouseEvent *event, QEditor *editor);
		virtual bool mouseReleaseEvent(QMouseEvent *event, QEditor *editor);
		virtual bool mouseDoubleClickEvent(QMouseEvent *event, QEditor *editor);
};

#endif // _EMACS_H_
