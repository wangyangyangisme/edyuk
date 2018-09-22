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

#ifndef _QDEBUGGER_H_
#define _QDEBUGGER_H_

#include "edyuk.h"

/*!
	\file qdebugger.h
	\brief Definition of the QDebugger interface.
*/

#include "qmdiclient.h"

class QDebuggingInteractionProxy;

class EDYUK_EXPORT QDebugger : public qmdiClient
{
	public:
		QDebugger();
		virtual ~QDebugger();
		
		virtual QString name() const = 0;
		virtual QString label() const = 0;
		
		virtual bool isRunning() const = 0;
		
		virtual QWidget* customDock() const = 0;
		
		virtual void setBreakpoint(const QString& filename, int line, bool on) = 0;
		
		virtual bool isSupportedInput(
							const QString& filename,
							const QString& language) const = 0;
		
		virtual void terminate() = 0;
		
		QDebuggingInteractionProxy* interactionProxy() const;
		
	protected:
		QString source() const;
		QString target() const;
		
		void started();
		
		void sendLog(const QString& line) const;
		void setLocation(const QString& fn, int line, bool activeBkpt = true);
		
		void toggleBreakpointOnCurrentLine();
		void getCurrentLocation(QString& file, int& line);
		void setVisualBreakpoint(const QString& filename, int line, bool on);
		
	private:
		QPair<QString, int> m_break;
};

#endif // !_QDEBUGGER_H_
