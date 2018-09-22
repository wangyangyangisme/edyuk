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

#ifndef _QNU_MAKE_BUILDER_H_
#define _QNU_MAKE_BUILDER_H_

#include "qbuilder.h"

class GnuMakeBuilder : public QBuilder
{
	friend class RebuildCommand;
	friend class CompileRunCommand;
	
	public:
		GnuMakeBuilder();
		virtual ~GnuMakeBuilder();
		
		virtual QString name() const;
		virtual QString label() const;
		virtual QString inputType() const;
		virtual QString outputType() const;
		
		virtual Output output(const QString& input, const QString& mode) const;
		virtual QList<Command*> commands() const;
		
		static void setMakeCommand(const QVariant& v);
		
	private:
		static QBuilder::Command
			*m_compile,
			*m_run,
			*m_compileAndRun,
			*m_clean,
			*m_rebuild;
};

namespace Makefile {
	void targets(const QString& f, QList<QStringList>& dest, const QString& scope = QString());
	QString target(const QString& t, const QStringList& cfg = QStringList());
}

#endif // !_QNU_MAKE_BUILDER_H_
