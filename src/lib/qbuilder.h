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

#ifndef _QBUILDER_H_
#define _QBUILDER_H_

/*!
	\file qbuilder.h
	\brief Definition of the QBuilder interface.
*/

#include <QIcon>
#include <QStringList>

class QProcess;

class QBuilder
{
	public:
		struct ParsedLine
		{
			ParsedLine() : line(0), column(0) {}
			
			bool isValid() { return message.count(); }
			
			int line, column;
			QString file, message;
		};
		
		class CommandParser
		{
			public:
				virtual ~CommandParser() {}
				
				virtual QStringList parse(const QString& outLine, ParsedLine& l) = 0;
		};
		
		struct Command
		{
			struct Info
			{
				QString exec;
				QStringList arguments;
				QString output;
			};
			
			virtual ~Command() {}
			
			virtual QIcon icon() const = 0;
			virtual QString label() const = 0;
			virtual bool mayAffectTargetList() const { return false; }
			virtual bool isStandalone() const { return false; }
			
			virtual Info info(const QString& in, const QString& mode) const = 0;
			
			virtual QList<Command*> depends() const = 0;
			
			virtual CommandParser* outputParser() const { return 0; }
		};
		
		struct Output
		{
			QString source;
			QList<QStringList> targets;
		};
		
		virtual ~QBuilder() {}
		
		virtual QString name() const = 0;
		virtual QString label() const = 0;
		virtual QString inputType() const = 0;
		virtual QString outputType() const = 0;
		
		virtual Output output(const QString& input, const QString& mode = QString()) const = 0;
		
		virtual QList<Command*> commands() const = 0;
};

class QBuildChain : public QList<QBuilder*>
{
	public:
		inline QBuildChain() : QList<QBuilder*>() {}
		inline QBuildChain(QBuilder *b) : QList<QBuilder*>() { append(b); }
};

#endif // !_QBUILDER_H_
