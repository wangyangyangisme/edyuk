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

#include "qproject.h"

/*!
	\file qproject.cpp
	\brief Implementation of the QProject class.
*/

#include "qprojectmodel.h"

#include <QDir>
#include <QStack>
#include <QFileInfo>

/*!
	\class QProject
	\brief The base class for project items
	
	A project object represents the root of any possible type
	of project. It is a QProjectNode itself, alllowing extreme
	flexibility in terms of internal data structure.
	
	Though subclassing of QProject is necessary (as with QProjectNode),
	it isn't a hard way to go since most methods have decent defaults
	which should fit most project types.
*/

/*!
	\brief ctor
*/
QProject::QProject()
 : QProjectNode(Project), m_modified(false)
{
	m_saved = false;
}

/*!
	\brief dtor
*/
QProject::~QProject()
{
	
}

/*!
	\brief Save the content of the project
	
	The default implementation does nothing...
*/
void QProject::save()
{
	m_saved = true;
	setModified(false);
}

/*!
	\brief Popup a project settings dialog
	
	The default implementation does nothing...
*/
void QProject::settings()
{
	
}

/*!
	\return the modification state of the project
*/
bool QProject::isModified() const
{
	return m_modified;
}

/*!
	\brief Set the modification state of the project
	\param modified State to set (true : modified)
*/
void QProject::setModified(bool modified)
{
	m_modified = modified;
}

/*!
	\return A list of files managed by the project
	\param m Computation mode to respect
*/
QStringList QProject::files(ComputationMode m) const
{
	QStringList files;
	QStack<const QProjectNode*> nodes;
	
	foreach ( QProjectNode *c, children() )
		nodes.push(c);
	
	while ( nodes.count() )
	{
		const QProjectNode *n = nodes.pop();
		
		if ( n->type() == File )
		{
			//qDebug("file : %s", qPrintable(n->name()));
			
			files << absoluteFilePath(n->name());
		} else if ( n->type() == Folder ) {
			//qDebug("folder : %s", qPrintable(n->name()));
			
			foreach ( const QProjectNode *c, n->children() )
				nodes.push(c);
			
		} else if ( (n->type() == Project) && (m == Recursive) ) {
			const QProject *p = dynamic_cast<const QProject*>(n);
			
			QStringList sf = p->files(m);
			
			foreach ( QString fn, sf )
				if ( !files.contains(fn) )
					files << fn;
		} else {
			//qDebug("unknown : %s", qPrintable(n->name()));
		}
	}
	
	//qDebug("%i files in %s [%i]", files.count(), qPrintable(name()), m == Recursive);
	
	return files;
}

/*!

*/
void QProject::fileAdded(const QString& file)
{
	//qDebug("file added : %s to %s", qPrintable(file), qPrintable(name()));
	
	QProject *p = project();
	
	if ( p )
		p->fileAdded(file);
	else if ( model() )
		model()->fileAdded(file, this);
}

/*!

*/
void QProject::fileRemoved(const QString& file)
{
	//qDebug("file removed : %s to %s", qPrintable(file), qPrintable(name()));
	
	QProject *p = project();
	
	if ( p )
		p->fileRemoved(file);
	else if ( model() )
		model()->fileRemoved(file, this);
}

/*!
	\return A list of sub projects managed by this project
	\param m Computation mode to respect
*/
QList<QProject*> QProject::subProjects(ComputationMode m) const
{
	QList<QProject*> sub;
	QStack<QProjectNode*> nodes;
	
	foreach ( QProjectNode *c, children() )
		nodes.push(c);
	
	while ( nodes.count() )
	{
		QProject *p = 0;
		QProjectNode *n = nodes.pop();
		
		if ( n->type() == Project )
			p = dynamic_cast<QProject*>(n);
		
		if ( p )
			sub << p;
		
		if ( !p || (p && (m & Recursive)) )
		{
			foreach ( QProjectNode *c, n->children() )
				nodes.push(c);
		}
	}
	
	return sub;
}

/*!
	
*/
QString QProject::backend() const
{
	return "project";
}

/*!
	\return The absolute path of a file managed by the project
	\param relative the relative path of the file to find
	
	The default implementation assumes \a relative to be relative to the project path.
	i.e. there is no dependency computation or akin.
*/
QString QProject::absoluteFilePath(const QString& relative) const
{
	return QDir::cleanPath(QDir(QFileInfo(name()).path()).absoluteFilePath(relative));
}

/*!
	\return The relative path of a file managed by the project
	\param absolute the absolute path of the file to find
*/
QString QProject::relativeFilePath(const QString& absolute) const
{
	return QDir::cleanPath(QDir(QFileInfo(name()).path()).relativeFilePath(absolute));
}

/*!
	\reimp
*/
void QProject::attach(QProjectNode *n, int index)
{
	QProjectNode::attach(n, index);
	
	if ( model() )
		emit model()->projectAdded(this);
	
}

/*!
	\reimp
*/
void QProject::detach()
{
	if ( model() )
		emit model()->projectRemoved(this);
	
	QProjectNode::detach();
}

/*!
	\return the drop mode of this project
*/
QProject::DropMode QProject::dropMode() const
{
	return NoDrops;
}

/*!
	\enum QProject::TargetType
	Defines the type of executable target provided by the project, if any.
*/

/*!
	\return the target type of the project
	\see TargetType
*/
QProject::TargetType QProject::targetType() const
{
	QString s = query("TARGET_TYPE");
	
	if ( s == "binary" ) return Binary;
	if ( s == "script" ) return Script;
	
	return NoTarget;
}

/*!
	\brief Query information from the project
	\param s Query string
	\return a string containing the asked information
	
	Query strings are roughly "variables". Valid values
	that should be provided by ALL projects are :
	
	<ul>
		<li><b>NAME</b> : project name (basename by default)
		<li><b>AUTHOR</b> : project author, if supported by the backend
		<li><b>TARGET_TYPE</b> : "none", "binary" or "script"
		<li><b>TARGET_PATH</b> : filepath of the binary or script if any
		<li><b>LANGUAGE</b> : programming language used (e.g : "C++", "Python"...)
	</ul>
	
	\note a NULL string is returned if the query fails.
	An empty string (if not null) indicates a lack of
	informations of the project (e.g. unset settings...)
*/
QString QProject::query(const QString& s) const
{
	Q_UNUSED(s)
	
	if ( s == "NAME" )
		return QFileInfo(name()).baseName();
	
	return QString();
}
