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

#ifndef _QPROJECT_PARSER_H_
#define _QPROJECT_PARSER_H_

#include "qpm-config.h"

/*!
	\file qprojectparser.h
	\brief Definition of the QProjectParser class.
*/

class QString;
class QProject;
class QStringList;

class QPM_EXPORT QProjectParser
{
	public:
		virtual ~QProjectParser() {}
		
		virtual QStringList projectFilters() const = 0;
		
		virtual bool canOpen(const QString& file) const = 0;
		virtual QProject* open(const QString& file) = 0;
};

#endif // !_QPROJECT_PARSER_H_
