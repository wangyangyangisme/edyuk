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

#include "qprojectmodel.h"

/*!
	\file qprojectmodel.cpp
	\brief Implementation of the QProjectModel class.
*/

#include "qproject.h"
#include "qprojectnode.h"
#include "qprojectloader.h"

//#include "modeltest.h"

#include <QHash>
#include <QFile>
#include <QStack>
#include <QMimeData>
#include <QStringList>
#include <QMessageBox>
#include <QDataStream>
#include <QFontMetrics>
#include <QApplication>
#include <QFileSystemWatcher>

//#define _TRACE_MODEL_

#define Q_EXTRACT_INDEX(index, abstract) \
	QProjectNode* abstract = \
					static_cast<QProjectNode*>(index.internalPointer()); \

#if QT_POINTER_SIZE == 2
	#define qPointer(ptr) ((quint16)ptr)
#elif QT_POINTER_SIZE == 4
	#define qPointer(ptr) ((quint32)ptr)
#elif QT_POINTER_SIZE == 8
	#define qPointer(ptr) ((quint64)ptr)
#else
	#error unsupported pointer size...
#endif

class QProjectMimeData : public QMimeData
{
	public:
		QList<QProjectNode*> m_nodes;
};

/*!
	\class QProjectModel
	\brief A model dedicated to project management
	
	QProjectModel is one of the central class of @QProjectModel ;)
	It is a polymorphic tree model class which manages a set of
	QProject and display them through an abstract node type :
	QProjectNode.
	
	The abstraction of QProjectModel make it suited for representing
	several projects of completely different internal structure since
	the model class itself doesn't konw anything about the real model
	structure.
	
	Though it is possible to do minimal subclassing and to create
	QProjectNode-based objects owning the real data, in many cases,
	the QProjectNode-based items will just be hierarchised wrappers
	over an internal data structure, hierarchic or not.
	
	To maximize flexibility, the model is able to perform project opening
	in a transparent way, once a QProjectLoader is set and filled with
	proper QProjectParser objects which will generate the project tree.
	
	\see QProject
	
	\see QProjectView
	
	\see QProjectLoader
	\see QProjectParser
*/

/*!
	\brief ctor
*/
QProjectModel::QProjectModel(QObject *p)
 : QAbstractItemModel(p), m_loader(0), m_watcher(new QFileSystemWatcher(this))
{
	//new ModelTest(this, this);
	
	connect(m_watcher	, SIGNAL( fileChanged(QString) ),
			this		, SLOT  ( fileChanged(QString) ) );
	
}

/*!
	\brief dtor
*/
QProjectModel::~QProjectModel()
{
	QProjectNode::flushDelayedDeletions();
}

void QProjectModel::addEditorWrapper(EditorWrapper *e)
{
	m_editors.append(e);
}

void QProjectModel::removeEditorWrapper(EditorWrapper *e)
{
	m_editors.removeAll(e);
}

void QProjectModel::beginInsertRows(const QModelIndex idx, int beg, int end)
{
	QAbstractItemModel::beginInsertRows(idx, beg, end);
}

void QProjectModel::beginRemoveRows(const QModelIndex idx, int beg, int end)
{
	QAbstractItemModel::beginRemoveRows(idx, beg, end);
}

void QProjectModel::endInsertRows()
{
	QAbstractItemModel::endInsertRows();
}

void QProjectModel::endRemoveRows()
{
	QAbstractItemModel::endRemoveRows();
	
	QProjectNode::flushDelayedDeletions();
}

bool QProjectModel::hasChildren(const QModelIndex &parent) const
{
	return rowCount(parent);
}

int QProjectModel::rowCount(const QModelIndex &parent) const
{
	if ( !parent.isValid() )
		return m_topLevel.count();
	
	Q_EXTRACT_INDEX(parent, item)
	
	if ( !item )
		qDebug("invalid index...");
	
	return item ? item->rowCount() : 0;
}

int QProjectModel::columnCount(const QModelIndex& parent) const
{
	Q_UNUSED(parent)
	
	return 1;
}

QVariant QProjectModel::data(const QModelIndex &index, int role) const
{
	if ( !index.isValid() || index.column() )
		return QVariant();
	
	Q_EXTRACT_INDEX(index, item)
	
	/*
	qDebug("%s->data(%i)",
		qPrintable(item->data(Qt::DisplayRole).toString()),
		role);
	*/
	
	return item ? item->data(role) : QVariant();
}

bool QProjectModel::setData(const QModelIndex& index, const QVariant& d, int role)
{
	if ( !index.isValid() || index.column() )
		return false;
	
	Q_EXTRACT_INDEX(index, item)
	
	return item ? item->setData(d, role) : false;
}

Qt::ItemFlags QProjectModel::flags(const QModelIndex &index) const
{
	if ( !index.isValid() )
		return Qt::ItemIsEnabled;
	
	Q_EXTRACT_INDEX(index, item)
	
	if ( !item )
		return Qt::ItemIsEnabled;
	
	Qt::ItemFlags f = 
			(
			 	Qt::ItemIsEnabled
			|
				Qt::ItemIsSelectable
			);
	
	int t = item->type();
	
	if ( t == QProjectNode::Folder )
		f |= Qt::ItemIsDropEnabled;
	
	if ( t == QProjectNode::File )
		f |= Qt::ItemIsDragEnabled;
	
	if ( item->defaultActions() & QProjectNode::Rename )
		f |= Qt::ItemIsEditable;
	
	return f;
}

QVariant QProjectModel::headerData(int, Qt::Orientation, int) const
{
	return QVariant();
}

QModelIndex QProjectModel::index(QProjectNode *n) const
{
	return n ? createIndex(n->row(), 0, n) : QModelIndex();
}

QModelIndex QProjectModel::index(int row, int column, const QModelIndex &parent) const
{
	if ( (row < 0) || column )
		return QModelIndex();
	
	Q_EXTRACT_INDEX(parent, item)
	QProjectNode *abs = 0;
	
	//qDebug("asking index...");
	
	if ( !parent.isValid() ) //&& (row < m_topLevel.count()) )
	{
		//qDebug("top level : %i", row);
		abs = (row < m_topLevel.count()) ? m_topLevel.at(row) : 0;
	} else if ( item ) {
		//qDebug("sub %i", row);
		abs = item->childAt(row);
	}
	
	#ifdef _TRACE_MODEL_
	qDebug("%s(%i, %i) : %s",
		item ? qPrintable(item->data(Qt::DisplayRole).toString()) : "root",
		row, column,
		abs ? qPrintable(abs->data(Qt::DisplayRole).toString()) : "!none!");
	#endif
	
	return abs ? createIndex(row, column, abs) : QModelIndex();
}

QModelIndex QProjectModel::parent(const QModelIndex &index) const
{
	if ( !index.isValid() )
	{
		return QModelIndex();
	}
	
	QProjectNode *parent = 0;
	Q_EXTRACT_INDEX(index, child)
	
	if ( child )
		parent = child->parent();
	
	#ifdef _TRACE_MODEL_
	qDebug("%s->parent() = %s",
		child ? qPrintable(child->data(Qt::DisplayRole).toString()) : "@invalid@",
		parent ? qPrintable(parent->data(Qt::DisplayRole).toString()) : "!none!");
	
	#endif
	
	if ( !parent )
		return QModelIndex();
	
	const int row = parent->row();
	
	return createIndex(row, 0, parent);
}

void QProjectModel::invalidateIndexes(QProjectNode *nd)
{
	QModelIndexList pl = persistentIndexList();
	
	for ( int i = 0; i < pl.count(); ++i )
	{
		QModelIndex idx = pl.at(i);
		QProjectNode *n = node(idx);
		
		if ( n != nd )
			continue;
		
		changePersistentIndex(idx, QModelIndex());
	}
}

Qt::DropActions QProjectModel::supportedDropActions() const
{
	return Qt::MoveAction | Qt::CopyAction;
}

QStringList QProjectModel::mimeTypes() const
{
	QStringList l;
	
	l << "x-abstract/qproject-nodes-pointer";
	
	return l;
}

QMimeData* QProjectModel::mimeData(const QModelIndexList &indexes) const
{
	Q_UNUSED(indexes)
	
	QList<QProjectNode*> nodes;
	
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);
	
	foreach ( QModelIndex index, indexes )
	{
		QProjectNode *n = node(index);
		
		if ( index.isValid() && n )
		{
			nodes << n;
			stream << n;
		}
	}
	
	QProjectMimeData *d = new QProjectMimeData;
	
	d->setData("x-abstract/qproject-nodes-pointer", data);
	
	d->m_nodes = nodes;
	
	return d;
}

bool QProjectModel::dropMimeData(	const QMimeData *data, Qt::DropAction action,
									int row, int column, const QModelIndex& parent)
{
	#ifdef _DEBUG_
	//qDebug("\tAction %i\n\tColumn : %i\n\tRow : %i\n\t", action, column, row);
	#endif
	
	
	if ( action == Qt::IgnoreAction )
		return true;
	else if ( !(action & supportedDropActions()) || (column > 0) )
		return false;
	
	const QProjectMimeData *d = static_cast<const QProjectMimeData*>(data);
	
	if ( !d )
		return false;
	
	if ( !d->hasFormat("x-abstract/qproject-nodes-pointer") )
	{
		qDebug(	"MIME data error : format mismatch!\n"
				"Expected  : x-abstract/qproject-nodes-pointer\n");
		
		return false;
	}
	
	QProjectNode *p = parent.isValid() ?
				dynamic_cast<QProjectNode*>(node(parent)) :
				dynamic_cast<QProjectNode*>(m_topLevel.at(row));
	
	if ( !p )
		return false;
	
	#ifdef _DEBUG_
	//qDebug("node to insert in : %s", qPrintable(p->name()));
	#endif
	
	foreach ( QProjectNode *c, d->m_nodes )
	{
		c->clone()->attach(p);
		
		if ( action == Qt::MoveAction )
		{
			c->destroy();
		}
	}
	
	return true;
}

/*!
	\return The node represented by the model index \a idx
	\param idx model index from which to extract project node
*/
QProjectNode* QProjectModel::node(const QModelIndex& idx) const
{
	Q_EXTRACT_INDEX(idx, node)
	
	return node;
}

/*!
	\return The number of opened projects
	\note This includes both top level projects and their subprojects...
*/
int QProjectModel::projectCount(bool recursive) const
{
	return recursive ? projects(recursive).count() : m_topLevel.count();
}

/*!
	\return All the opened projects
	\note This includes both top level projects and their subprojects...
*/
QList<QProject*> QProjectModel::projects(bool recursive) const
{
	QList<QProject*> l;
	
	foreach ( QProject *p, m_topLevel )
	{
		l << p;
		
		if ( recursive )
			l << p->subProjects(QProject::Recursive);
	}
	
	return l;
}

/*!
	\return The project named \a n
	\param n Name (that is filename) of the project to find
	\note Searches among both top level projects and their subprojects...
*/
QProject* QProjectModel::project(const QString& n) const
{
	if ( n.isEmpty() )
		return 0;
	
	QList<QProject*> pro = projects(true);
	
	foreach ( QProject *p, pro )
		if ( p->name() == n )
			return p;
	
	return 0;
}

/*!
	\return The QProjectLoader object attached to that project
	\note A QProjectLoader object can be attached to several
	models at a time.
	
	\see setProjectLoader(QProjectLoader*)
*/
QProjectLoader* QProjectModel::projectLoader() const
{
	return m_loader;
}

/*!
	\brief Attach a QProjectLoader object to the model
	\note A QProjectLoader object can be attached to several
	models at a time.
	
	\see projectLoader()
*/
void QProjectModel::setProjectLoader(QProjectLoader *l)
{
	m_loader = l;
}

/*!
	\brief Adds a top level project
	\note This is not meant to be used directly...
*/
void QProjectModel::addProject(QProject *p)
{
	if ( !p || m_topLevel.contains(p) || p->parent() )
		return;
	
	foreach ( const LoadInterception& i, m_intercept )
	{
		if ( i.file == p->name() )
		{
			if ( i.parent )
			{
				p->attach(i.parent, i.index);
				
			} else if ( m_topLevel.count() >= i.index ) {
				
				p->m_model = this;
				p->m_parent = 0;
				
				beginInsertRows(QModelIndex(), i.index, i.index + p->rowSpan() - 1);
				m_topLevel.insert(i.index, p);
				endInsertRows();
				
				emit projectReloaded(i.old, p);
			}
			
			memorize(p);
			return;
		}
	}
	
	beginInsertRows(QModelIndex(), m_topLevel.count(), m_topLevel.count());
	
	m_topLevel << p;
	
	memorize(p);
	
	emit projectAdded(p);
	emit projectAdded(p->name());
	
	endInsertRows();
}

/*!
	\brief Removes a top level project
	\note This is not meant to be used directly...
 */
void QProjectModel::removeProject(QProject *p)
{
	const int idx = m_topLevel.indexOf(p);
	
	if ( idx == -1 )
		return;
	
	beginRemoveRows(QModelIndex(), idx, idx);
	
	m_topLevel.removeAt(idx);
	
	forget(p);
	
	emit projectRemoved(p);
	emit projectRemoved(p->name());
	
	endRemoveRows();
}

/*!
	\brief Creates some cache/watch concerning a given project
*/
void QProjectModel::memorize(QProject *p)
{
	QStack<QProjectNode*> nodes;
	nodes.push(p);
	
	while ( nodes.count() )
	{
		QProjectNode *n = nodes.pop();
		
		QProject *p = dynamic_cast<QProject*>(n);
		
		if ( p )
		{
			p->setModified(false);
			m_watcher->addPath(p->name());
		}
		
		n->m_model = this;
		
		foreach ( QProjectNode *c, n->children() )
			nodes.push(c);
	}
}

/*!
	\brief Discard all cache/watch concerning a given project
*/
void QProjectModel::forget(QProject *p)
{
	QStack<QProjectNode*> nodes;
	nodes.push(p);
	
	while ( nodes.count() )
	{
		QProjectNode *n = nodes.pop();
		
		n->m_model = 0;
		
		QProject *proj = dynamic_cast<QProject*>(n);
		
		if ( proj )
		{
			m_watcher->removePath(proj->name());
		}
		
		foreach ( QProjectNode *c, n->children() )
			nodes.push(c);
	}
}

/*!
	\brief Reload a project on change notification
*/
void QProjectModel::fileChanged(const QString& file)
{
	if ( sender() != m_watcher )
		return;
	
	QProject *p = project(file);
	
	while ( p && p->parent() )
		p = p->project();
	
	if ( !p )
		return;
	
	QString filename = p->name();
	
	emit reloadingProject(p);
	
	if ( p->m_saved )
	{
		// avoid duplicate messages...
		qApp->processEvents();
		p->m_saved = false;
		return;
	}
	
	//qDebug("file save detected : %s", qPrintable(file));
	
	LoadInterception i;
	i.old = p;
	i.file = file;
	i.parent = p->parent();
	i.index = i.parent ? i.parent->children().indexOf(p) : m_topLevel.indexOf(p);
	
	m_intercept << i;
	
	//p->detach();
	if ( !i.parent && (i.index >= 0) )
	{
		beginRemoveRows(QModelIndex(), i.index, i.index);
		m_topLevel.removeAt(i.index);
		endRemoveRows();
	}
	
	forget(p);
	delete p;
	
	m_loader->open(filename, this);
}

/*!
	\brief Reload a project on change notification
*/
void QProjectModel::dataChanged(QProjectNode *n)
{
	QModelIndex idx = index(n);
	emit QAbstractItemModel::dataChanged(idx, idx);
}

/*!
	\brief Attempt to open a project
	\return true on success
	\param s filename of project to open
*/
bool QProjectModel::openProject(const QString& s)
{
	if ( !m_loader || !QFile::exists(s) )
		return false;
	
	foreach ( QProject *p, m_topLevel )
		if ( p->name() == s )
			return true;
	
	return m_loader->open(s, this);
}

/*!
	\brief Close a project
	\return Whether the project was closed.
	\param s Name of the project ot close
	
	\note A sub project CAN NOT be closed through this method
	\warning 
*/
bool QProjectModel::closeProject(const QString& s)
{
	foreach ( QProject *p, m_topLevel )
	{
		if ( p->name() == s )
		{
			if ( tryCommit(p) )
				return false;
			
			removeProject(p);
			delete p;
			return true;
		}
	}
	
	return false;
}

/*!
	\brief Save a project
	\param s Name of project to save
	
	\note A sub project CAN be saved through this method
*/
void QProjectModel::saveProject(const QString& s)
{
	QList<QProject*> pro = projects(QProject::Recursive);
	
	foreach ( QProject *p, pro )
		if ( p->name() == s )
			p->save();
}

/*!
	\brief Save all projects
*/
void QProjectModel::saveAll()
{
	QList<QProject*> pro = projects(QProject::Recursive);
	
	foreach ( QProject *p, pro )
		if ( p->isModified() )
			p->save();
}

/*!
	\return Whether the user agree to close all opened projects
*/
bool QProjectModel::tryCommitAll()
{
	QList<QProject*> lp = projects(QProject::Recursive);
	
	foreach ( QProject *p, lp )
		if ( tryCommit(p) )
			return true;
	
	return false;
}

/*!
	\return Whether closure succeeded
	\param Whether to force closure
	\note if force is set to false (the default) tryCommitAll() is called
	to ensure no undesired closure happen.
*/
bool QProjectModel::closeAll(bool force)
{
	if ( m_topLevel.isEmpty() )
		return true;
	
	if ( !force )
	{
		if ( tryCommitAll() )
			return false;
	}
	
	beginRemoveRows(QModelIndex(), 0, m_topLevel.count() - 1);
	
	QList<QProject*> l = m_topLevel;
	m_topLevel.clear();
	
	foreach ( QProject *p, l )
	{
		forget(p);
		
		emit projectRemoved(p);
		emit projectRemoved(p->name());
		
		delete p;
	}
	
	endRemoveRows();
	
	return true;
}

/*!
	\brief Check whether a project is modified and ask user to commit changes if any
	\return true if the user decided to cancel its action (useful upon closure...)
	\param p project to check
*/
bool QProjectModel::tryCommit(QProject *p)
{
	if ( !p )
		return false;
	
	if ( p->isModified() )
	{
		int ret = QMessageBox::question(0,
										tr("Commit changes?"),
										tr(
											"The project %1 has been modified.\n"
											"Would you like to commit your changes?"
										).arg(p->name()),
										QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
										);
		
		if ( ret == QMessageBox::Yes )
			p->save();
		
		return ret == QMessageBox::Cancel;
	} else {
		QList<QProject*> l = p->subProjects();
		
		foreach ( QProject *sp, l )
			if ( tryCommit(sp) )
				return true;
	}
	
	return false;
}

/*!
	\brief Edit a node
*/
void QProjectModel::edit(QProjectNode *n)
{
	QModelIndex idx = index(n);
	
	if ( !idx.isValid() )
		return;
	
	emit tryEdit(idx);
}

void QProjectModel::tryEdit(const QModelIndex& idx)
{
	if ( m_editors.count() )
	{
		m_editors.last()->edit(idx);
		
		return;
	}
	
	emit requestEdit(idx);
}
