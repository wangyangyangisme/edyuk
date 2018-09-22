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

#include "qdebugginginteractionproxy.h"

#include "qdebuggingengine.h"

#include <QMessageBox>

QDebuggingInteractionProxy::QDebuggingInteractionProxy(QObject *p)
 : QObject(p)
{
	
}

QDebuggingInteractionProxy::~QDebuggingInteractionProxy()
{
	
}

void QDebuggingInteractionProxy::error(const QString& msg) const
{
	emit QDebuggingEngine::instance()->log(msg);
	
	QMessageBox::critical(0,
						QDebuggingEngine::tr("Debugging error"),
						msg
						);
	
}

void QDebuggingInteractionProxy::information(const QString& msg) const
{
	emit QDebuggingEngine::instance()->log(msg);
	
	QMessageBox::information(0,
						QDebuggingEngine::tr("Debugging information"),
						msg
						);
	
}

void QDebuggingInteractionProxy::question(const QString& msg) const
{
	//emit QDebuggingEngine::instance()->log(msg);
	
	QMessageBox::question(0,
						QDebuggingEngine::tr("Debugging question"),
						msg
						);
	
}

void QDebuggingInteractionProxy::warning(const QString& msg) const
{
	emit QDebuggingEngine::instance()->log(msg);
	
	QMessageBox::warning(0,
						QDebuggingEngine::tr("Debugging warning"),
						msg
						);
	
}

void QDebuggingInteractionProxy::error(const QString& msg, MessageCallback *cb, int buttons, int defaultButton) const
{
	emit QDebuggingEngine::instance()->log(msg);
	
	int ret =
	QMessageBox::critical(0,
						QDebuggingEngine::tr("Debugging error"),
						msg,
						buttons ? QMessageBox::StandardButtons(buttons) : QMessageBox::Ok,
						QMessageBox::StandardButton(defaultButton)
						);
	
	cb->response(ret);
}

void QDebuggingInteractionProxy::information(const QString& msg, MessageCallback *cb, int buttons, int defaultButton) const
{
	emit QDebuggingEngine::instance()->log(msg);
	
	int ret =
	QMessageBox::information(0,
						QDebuggingEngine::tr("Debugging information"),
						msg,
						buttons ? QMessageBox::StandardButtons(buttons) : QMessageBox::Ok,
						QMessageBox::StandardButton(defaultButton)
						);
	
	cb->response(ret);
}

void QDebuggingInteractionProxy::question(const QString& msg, MessageCallback *cb, int buttons, int defaultButton) const
{
	//emit QDebuggingEngine::instance()->log(msg);
	
	int ret =
	QMessageBox::question(0,
						QDebuggingEngine::tr("Debugging question"),
						msg,
						buttons ? QMessageBox::StandardButtons(buttons) : QMessageBox::Ok,
						QMessageBox::StandardButton(defaultButton)
						);
	
	cb->response(ret);
}

void QDebuggingInteractionProxy::warning(const QString& msg, MessageCallback *cb, int buttons, int defaultButton) const
{
	emit QDebuggingEngine::instance()->log(msg);
	
	int ret =
	QMessageBox::warning(0,
						QDebuggingEngine::tr("Debugging warning"),
						msg,
						buttons ? QMessageBox::StandardButtons(buttons) : QMessageBox::Ok,
						QMessageBox::StandardButton(defaultButton)
						);
	
	cb->response(ret);
}
