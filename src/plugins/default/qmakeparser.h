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

#ifndef _QMAKE_PARSER_H_
#define _QMAKE_PARSER_H_

/*!
	\file qmakeparser.h
	\brief Definition of the QMakeParser class.
*/

#include "qprojectparser.h"

#include <QStringList>
#include <QCoreApplication>

namespace QMakeModel
{
	struct INode;
}

class QProjectNode;

class QMakeParser : public QProjectParser
{
	Q_DECLARE_TR_FUNCTIONS(QMakeParser)
	
	public:
		struct Token
		{
			inline Token() : next(0) {}
			inline Token(const QString& s) : text(s), next(0) {}
			~Token() { delete next; }
			
			QString text;
			Token *next;
			
			Token* advance(const QString& s)
			{
				next = new Token(s);
				return next;
			}
		};
		
		class TokenList : public QList<Token*>
		{
			public:
				void cleanup() { qDeleteAll(*this); clear(); }
		};
		
		QMakeParser();
		virtual ~QMakeParser();
		
		virtual QStringList projectFilters() const;
		
		virtual bool canOpen(const QString& file) const;
		virtual QProject* open(const QString& file);
		
		static bool parse(QProject *p, QMakeModel::INode *n, const QString& file);
		
		static TokenList lex(const QChar *data, int length);
		
		static void parse(QProject *p, QMakeModel::INode *n,
						  TokenList& tokens, int index = 0, int max = -1);
		
};

#endif // !_QMAKE_PARSER_H_
