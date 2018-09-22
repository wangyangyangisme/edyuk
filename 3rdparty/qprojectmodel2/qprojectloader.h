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

#ifndef _QPROJECT_LOADER_H_
#define _QPROJECT_LOADER_H_

#include "qpm-config.h"

/*!
	\file qprojectloader.h
	\brief Definition of the QProjectLoader class.
*/

#include <QList>
#include <QQueue>
#include <QThread>
#include <QReadWriteLock>

class QProjectModel;
class QProjectParser;

class QPM_EXPORT QProjectLoader : public QThread
{
	public:
		QProjectLoader(QObject *p = 0);
		virtual ~QProjectLoader();
		
		QStringList projectFilters() const;
		
	public slots:
		void process();
		bool open(const QString& file, QProjectModel *dest);
		
		void addParser(QProjectParser *p);
		void removeParser(QProjectParser *p);
		
	protected:
		virtual void run();
		
	private:
		struct OpenRequest
		{
			QString file;
			QProjectParser *p;
			QProjectModel *model;
		};
		
		QReadWriteLock m_lock;
		QQueue<OpenRequest> m_request;
		QList<QProjectParser*> m_parsers;
};

#endif // !_QPROJECT_LOADER_H_
