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

#ifndef _QDEBUGGING_INTERACTION_PROXY_H_
#define _QDEBUGGING_INTERACTION_PROXY_H_

#include "edyuk.h"

/*!
	\file qdebugginginteractionproxy.h
	\brief Definition of the QDebuggingInteractionProxy interface.
*/

#include <QObject>
#include <QString>

class MessageCallback
{
	public:
		virtual ~MessageCallback() {}
		virtual void response(int button) = 0;
};

class EDYUK_EXPORT QDebuggingInteractionProxy : public QObject
{
	Q_OBJECT
	
	public:
		QDebuggingInteractionProxy(QObject *p = 0);
		virtual ~QDebuggingInteractionProxy();
		
	public slots:
		virtual void error(const QString& msg) const;
		virtual void warning(const QString& msg) const;
		virtual void question(const QString& msg) const;
		virtual void information(const QString& msg) const;
		
		virtual void error(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		virtual void warning(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		virtual void question(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
		virtual void information(const QString& msg, MessageCallback *cb, int buttons, int defaultButton = 0) const;
};

#endif // ! _QDEBUGGING_INTERACTION_PROXY_H_
