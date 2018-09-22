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

#ifndef _QCPP_LEXER_H_
#define _QCPP_LEXER_H_

#include "qcodelexer.h"

#include <QHash>

class QCppLexer : public QCodeLexer
{
	public:
		QCppLexer();
		QCppLexer(QCodeStream *s);
		
		bool keepMacroBlocks() const;
		void setKeepMacroBlocks(bool keep);
		
		virtual void setInput(QCodeStream *s, LexMode m = Normal);
		
		virtual QToken nextToken();
		virtual QToken previousToken();
		
		virtual QTokenList tokens();
		virtual int lineForToken(int tokenId, int minLine = 0) const;
		
	protected:
		void initMacros();
		void refreshTokens(LexMode m);
		
		QList<int> m_jumps;
		QList<int> m_suspicious;
		QTokenList tokenBuffer;
		bool bTokenized, bKeepMacros;
		static QHash<QToken, QToken> m_macros;
		QTokenList::const_iterator tokenPointer;
};

#endif // _QCPP_LEXER_H_
