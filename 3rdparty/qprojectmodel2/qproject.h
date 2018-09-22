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

#ifndef _QPROJECT_H_
#define _QPROJECT_H_

#include "qpm-config.h"

/*!
	\file qproject.h
	\brief Definition of the QProject class.
*/

#include "qprojectnode.h"

#include <QStringList>

class QBuildEngine;

class QPM_EXPORT QProject : public QProjectNode
{
	friend class QProjectModel;
	
	public:
		enum ComputationFlags
		{
			NonRecursive	= 0x00,
			Recursive		= 0x01
		};
		
		enum DropMode
		{
			NoDrops			= 0x00,
			InternalMoves	= 0x01,
			ExternalMoves	= 0x02
		};
		
		enum TargetType
		{
			NoTarget,
			Binary,
			Script
		};
		
		Q_DECLARE_FLAGS(ComputationMode, ComputationFlags)
		
		virtual ~QProject();
		
		virtual QStringList files(ComputationMode m = NonRecursive) const;
		virtual QList<QProject*> subProjects(ComputationMode m = NonRecursive) const;
		
		virtual void save();
		
		virtual void settings();
		
		virtual bool isModified() const;
		virtual void setModified(bool modified);
		
		virtual QString backend() const;
		
		virtual QString absoluteFilePath(const QString& relative) const;
		virtual QString relativeFilePath(const QString& absolute) const;
		
		virtual void attach(QProjectNode *n, int index = -1);
		virtual void detach();
		
		virtual DropMode dropMode() const;
		
		virtual TargetType targetType() const;
		
		virtual QString query(const QString& s) const;
		
		virtual void fileAdded(const QString& file);
		virtual void fileRemoved(const QString& file);
		
	protected:
		QProject();
		
		bool m_saved, m_modified;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QProject::ComputationMode)

#endif // !_QPROJECT_H_
