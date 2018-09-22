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

#include "qprojectloader.h"

/*!
	\file qprojectloader.cpp
	\brief Implementation of the QProjectLoader class.
*/

#include "qproject.h"
#include "qprojectmodel.h"
#include "qprojectparser.h"

/*!
	\class QProjectLoader
	\brief A parsers management class
	
	QProjectLoader owns a collection of QProjectParser object
	and request them to generate project trees from files.
*/

/*!
	\brief ctor
*/
QProjectLoader::QProjectLoader(QObject *p)
 : QThread(p)
{
	static int metaTypeId = qRegisterMetaType<QProject*>("QProject*");
}

/*!
	\brief dtor
*/
QProjectLoader::~QProjectLoader()
{
	//qDeleteAll(m_parsers);
}

/*!
	\return A list of filters supported by the parser
	This function is an helper for smart open dialogs.
*/
QStringList QProjectLoader::projectFilters() const
{
	QStringList l;
	
	foreach ( QProjectParser *p, m_parsers )
	{
		l << p->projectFilters();
	}
	
	return l;
}

/*!
	\brief Add a parser to the collection
	
	\warning The parser is not taken over. It won't get deleted
	from QProjectLoader dtor.
*/
void QProjectLoader::addParser(QProjectParser *p)
{
	if ( !m_parsers.contains(p) )
		m_parsers << p;
}

/*!
	\brief Remove a parser from the collection
	
	\warning The parser is NOT deleted.
*/
void QProjectLoader::removeParser(QProjectParser *p)
{
	m_parsers.removeAll(p);
}

/*!
	\brief Attempt to open a project file
	\return true on success
	\param file File to load
	\param dest Destination model
*/
bool QProjectLoader::open(const QString& file, QProjectModel *dest)
{
	foreach ( QProjectParser *p, m_parsers )
	{
		if ( !p->canOpen(file) )
			continue;
		
		/*
		QProject *project = p->open(file);
		
		if ( dest )
			dest->addProject(project);
		*/
		
		OpenRequest req;
		
		req.p = p;
		req.file = file;
		req.model = dest;
		
		QWriteLocker lock(&m_lock);
		
		m_request << req;
		
		if ( !isRunning() )
			start();
		
		return true;
	}
	
	return false;
}

/*!
	\internal
*/
void QProjectLoader::run()
{
	process();
}

/*!
	\internal
*/
void QProjectLoader::process()
{
	while ( m_request.count() )
	{
		m_lock.lockForRead();
		OpenRequest req = m_request.takeAt(0);
		m_lock.unlock();
		
		QProject *project = req.p->open(req.file);
		
		if ( req.model )
		{
			//qDebug("loaded project %s", qPrintable(req.file));
			QMetaObject::invokeMethod(req.model, "addProject", Q_ARG(QProject*, project));
		}
	}
}
