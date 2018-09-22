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

#include "qprojectnode.h"

/*!
	\file qprojectnode.cpp
	\brief Implementation of the QProjectNode class.
*/

#include "qproject.h"
#include "qprojectmodel.h"

#include <QFileInfo>
#include <QFileDialog>

QList<QProjectNode*> QProjectNode::_delayed;

void QProjectNode::flushDelayedDeletions()
{
	qDeleteAll(_delayed);
	_delayed.clear();
}

void QProjectNode::delayedDeletion(QProjectNode *n)
{
	_delayed << n;
}

/*!
	\class QProjectNode
	\brief Abstract base class for all projects node
	
	QProjectNode is actually meant to be (or at least to allow being used as)
	some sort of visitor that would travel accross project internal data. It
	is in this aspect comparable to QModelIndex, except that it is probably
	more "persistent"...
*/

/*!
	\brief Ctor
*/
QProjectNode::QProjectNode(NodeType t)
 : m_type(t), m_model(0), m_parent(0)
{
	
}

/*!
	\brief Dtor
*/
QProjectNode::~QProjectNode()
{
	//qDebug("~:0x%x", this);
	
	if ( m_model )
	{
		m_model->invalidateIndexes(this);
		
		detach(true);
	}
	
	clear();
}

/*!
	\brief Convenience function that allow simple and safe node deletion
	\note prefer calling this fuction every time you would delete a node
*/
void QProjectNode::destroy()
{
	if ( m_model )
		m_model->invalidateIndexes(this);
	
	detach();
	//clear();
	
	//qDebug("~(delayed):0x%x", this);
	
	// deleteLater...
	//delayedDeletion(this);
	delete this;
}

/*!
	\brief Clear the node
	\note All child nodes are DELETED.
*/
void QProjectNode::clear()
{
	beginRemoveRows(this, 0, rowCount() - 1);
	
	foreach ( QProjectNode *n, m_children )
	{
		n->m_model = 0;
		n->m_parent = 0;
		
		delete n;
	}
	
	m_children.clear();
	
	endRemoveRows();
}

/*!
	\return type of this node
*/
QProjectNode::NodeType QProjectNode::type() const
{
	return m_type;
}

/*!
	\return visual row of this node within its parent
*/
int QProjectNode::row() const
{
	return m_parent
		?
			m_parent->visualRow(this)
		:
			(
				m_model
			?
				m_model->
					m_topLevel.indexOf(
						dynamic_cast<QProject*>(
							const_cast<QProjectNode*>(this)
											   )
									  )
			:
				-1
			)
		;
}

/*!
	\return model to which this node belongs
*/
QProjectModel* QProjectNode::model() const
{
	return m_model;
}

/*!
	\return project to which this node belongs
	\note That method (and it shall work the same
	when reimplemented) MUST NOT return "this"...
*/
QProject* QProjectNode::project() const
{
	QProjectNode *n = m_parent;
	
	while ( n )
	{
		if ( n->m_type == Project )
			return dynamic_cast<QProject*>(n);
		
		n = n->m_parent;
	}
	
	return 0;
}

/*!
	\return parent of this node
*/
QProjectNode* QProjectNode::parent() const
{
	return m_parent;
}

/*!
	\return data corresponding to an item role
	\note subclassing this function should not be needed, relying
	on specialized handlers is prefered.
*/
QVariant QProjectNode::data(int role) const
{
	if ( role == Qt::DisplayRole )
	{
		NodeType t = type();
		return ((t == File) || (t == Project)) ? QFileInfo(name()).fileName() : name();
	} else if ( role == Qt::EditRole ) {
		return name();
	} else if ( role == QProjectModel::DetailLevelRole ) {
		return 0;
	}
	
	return QVariant();
}

/*!
	\brief set the data corresponding to an item role
	\note this function is only relevant for editable nodes (i.e. those which can be
	renamed)...
*/
bool QProjectNode::setData(const QVariant& v, int role)
{
	Q_UNUSED(v)
	Q_UNUSED(role)
	/*
	if ( role == Qt::EditRole )
	{
		qDebug("setting edit role");
		
		return true;
	}
	*/
	
	return false;
}

/*!
	\return number of model rows occupied by a node
	
	A value of 0 means an "hidden" node
	A value of 1 means a "regular" node
	A greater value means a fragmented node (unused but kept for possible evolution)
	
	\see isFragmented()
*/
int QProjectNode::rowSpan() const
{
	return 1;
}

/*!
	\return whether the node is fragmented
	
	\note fragmented nodes are a special kind that can not have children
	but owns a collection of internal nodes
	
	\see rowSpan()
	
	\deprecated
*/
bool QProjectNode::isFragmented() const
{
	return rowSpan() > 1;
}

/*!
	\return The number of visual rows occupied by child nodes
*/
int QProjectNode::rowCount() const
{
	int visual = 0;
	
	//foreach ( QProjectNode *n, m_children )
	for ( int i = 0; i < m_children.count(); ++i )
	{
		QProjectNode *n = m_children.at(i);
		
		//qDebug("%s[%i] : spans 0", qPrintable(name()), i++, n->rowSpan());
		visual += n->rowSpan();
	}
	
	return visual;
}

/*!
	\return The visual row at which is placed the n-th child node
*/
int QProjectNode::visualRow(int index) const
{
	int visual = 0;
	index = qMin(index, m_children.count());
	
	for ( int i = 0; i < index; ++i )
	{
		visual += m_children.at(i)->rowSpan();
	}
	
	return visual;
}

/*!
	\return The visual row at which is placed the child node \a c or -1 if it is not a child...
*/
int QProjectNode::visualRow(const QProjectNode *c) const
{
	if ( !c->rowSpan() )
		return -1;
	
	int visual = 0;
	
	for ( int i = 0; i < m_children.count(); ++i )
	{
		if ( c == m_children.at(i) )
			return visual;
		
		visual += m_children.at(i)->rowSpan();
	}
	
	return -1;
}

/*!
	\return The child node at visual row \a row
*/
QProjectNode* QProjectNode::childAt(int row) const
{
	if ( row < 0 )
	{
		//qDebug("invalid index : %s[%i] = 0 ", qPrintable(name()), row);
		return 0;
	}
	
	int visual = 0;
	
	for ( int i = 0; i < m_children.count(); ++i )
	{
		QProjectNode *n = m_children.at(i);
		
		if ( !n->rowSpan()  )
			continue;
		
		if ( (visual == row) && !n->isFragmented() )
		{
			//qDebug("%s[%i] = %s", qPrintable(name()), row, qPrintable(n->name()));
			return n;
		} else if ( (visual <= row) && (row < (visual + n->rowSpan())) ) {
			QProjectNode *c = n->childAt(row - visual);
			
			if ( !c )
				qDebug("%s[%i] = 0", qPrintable(name()), row);
			
			return c;
		} else {
			
		}
		
		visual += n->rowSpan();
	}
	
	//qWarning("invalid index : %s[%i] = 0", qPrintable(name()), row);
	
	return 0;
}

/*!
	\return The list of child actual nodes
	\note
	<ul>
		<li>actual nodes with a rowspan of 0 are not seen as visual nodes but appear in this list
		<li>fragmented nodes appear in this list while their visual fragments do not
	</ul>
*/
QList<QProjectNode*> QProjectNode::children() const
{
	return m_children;
}

/*!
	\brief Writes the contents of a node to a file (subset for project saving)
	
	The default implementation calls write() for each of its children.
*/
void QProjectNode::write(QTextStream& out, int indent) const
{
	foreach ( QProjectNode *n, m_children )
		n->write(out, indent);
}

/*!
	\brief Detach the node from its parent
*/
void QProjectNode::detach(bool blockFileSig)
{
	if ( m_parent )
	{
		QProject *p = project();
		
		if ( p )
			p->setModified(true);
		
		const int row = this->row(),
					span = rowSpan();
		
		if ( span )
			beginRemoveRows(m_parent, row, row + rowSpan() - 1);
		
		if ( p && !blockFileSig && (type() == File) )
			p->fileRemoved(p->absoluteFilePath(name()));
		
		m_parent->removeChild(this);
		m_parent = 0;
		
		if ( span )
			endRemoveRows();
	}
	
	m_model = 0;
}

/*!
	\brief Attach the node to a new parent
*/
void QProjectNode::attach(QProjectNode *p, int index)
{
	const int rs = rowSpan();
	
	detach();
	
	if ( !p )
		return;
	
	m_model = p->m_model;
	int cc = p->children().count();
	index = ((index < 0) && (index < cc)) ? cc : index;
	const int row = p->visualRow(index);
	
	if ( rs )
		beginInsertRows(p, row, row + rowSpan() - 1);
	
	m_parent = p;
	p->insertChild(this, index);
	
	if ( rs )
		endInsertRows();
	
	QProject *pn = project();
	
	if ( pn )
		pn->setModified(true);
}

/*!
	\brief Attach the node to a new parent
*/
void QProjectNode::attach(QProjectNode *p, QProjectNode *proxy, int index)
{
	const int rs = rowSpan() && proxy;
	
	detach();
	
	if ( !p )
		return;
	
	if ( !proxy )
		proxy = p;
	
	m_model = p->m_model;
	int cc = proxy->children().count();
	const int row = proxy->rowCount(); //->visualRow(cc - 1);
	
	if ( rs )
		beginInsertRows(proxy, row, row + rowSpan() - 1);
	
	m_parent = p;
	p->insertChild(this, index);
	
	if ( rs )
		endInsertRows();
	
	QProject *pn = project();
	
	if ( pn )
		pn->setModified(true);
}

/*!
	\brief Action handler
	\see actions()
 */
QProjectNode::ActionList QProjectNode::actions() const
{
	ActionList l;
	
	DefaultActions a = defaultActions();
	
	if ( dynamic_cast<const QProject*>(this) && !parent() )
	{
		l << Action(QIcon(":/activate.png"), QProjectModel::tr("Set as active project"));
	}
	
	if ( a & Open )
	{
		l << Action(QIcon(":/open.png"), QProjectModel::tr("Open"));
	}
	
	if ( a & Save )
	{
		l << Action(QIcon(":/save.png"), QProjectModel::tr("Save"));
	}
	
	if ( a & Rename )
	{
		l << Action(QIcon(":/rename.png"), QProjectModel::tr("Rename"));
	}
	
	if ( a & Remove )
	{
		l << (
					parent()
				?
					Action(QIcon(":/remove.png"), QProjectModel::tr("Remove"))
				:
					Action(QIcon(":/close.png"), QProjectModel::tr("Close"))
			);
	}
	
	if ( a & AddFile )
	{
		l << Action(QIcon(":/add.png"), QProjectModel::tr("Add file(s)"));
	}
	
	if ( a & AddFolder )
	{
		l << Action(QIcon(":/foldernew.png"), QProjectModel::tr("New folder"));
	}
	
	if ( a & Settings )
	{
		l << Action(QIcon(":/settings.png"), QProjectModel::tr("Settings"));
	}
	
	return l;
}

/*!
	\return The default actions supported by the node
*/
QProjectNode::DefaultActions QProjectNode::defaultActions() const
{
	int t = type();
	
	if ( t == File )
	{
		return Open | Remove | Rename;
	} else if ( t == Folder ) {
		return Remove | Rename | AddFile | AddFolder;
	} else if ( t == Project ) {
		return Open | Save | Remove | AddFile | AddFolder | Settings;
	}
	
	return Remove | Rename;
}

/*!
	\brief Action handler
	\see actions()
*/
void QProjectNode::actionTriggered(const QString& label)
{
	QProject *p = project();
	
	if ( label == QProjectModel::tr("Close") )
	{
		if ( m_model )
			m_model->closeProject(name());
		
	} else if ( label == QProjectModel::tr("Save") ) {
		dynamic_cast<QProject*>(this)->save();
	} else if ( label == QProjectModel::tr("Remove") ) {
		destroy();
	} else if ( label == QProjectModel::tr("Rename") ) {
		if ( m_model )
			m_model->edit(this);
		
	} else if ( label == QProjectModel::tr("Add file(s)") ) {
		
		QStringList fl = QFileDialog::getOpenFileNames(
										0,
										QProjectModel::tr(""),
										QFileInfo(p ? p->name() : name()).path()
									);
		
		foreach ( const QString& fn, fl )
			addFile(fn);
		
	} else if ( label == QProjectModel::tr("New folder") ) {
		addFolder("New folder");
	} else if ( label == QProjectModel::tr("New file") ) {
		
	} else if ( label == QProjectModel::tr("Open") ) {
		if ( m_model )
			emit m_model->fileActivated(p ? p->absoluteFilePath(name()) : name());
		
	} else if ( label == QProjectModel::tr("Settings") ) {
		dynamic_cast<QProject*>(this)->settings();
	} else if ( label == QProjectModel::tr("Set as active project") ) {
		if ( m_model )
			emit m_model->requestActivation(dynamic_cast<QProject*>(const_cast<QProjectNode*>(this)));
	}
}

/*!
	\brief Adds a file to the node (if possible)
	\param file file to add
	\note the default implementation does nothing
*/
void QProjectNode::addFile(const QString& file)
{
	QProject *p = project();
	
	if ( !p )
		p = dynamic_cast<QProject*>(this);
	
	//if ( p )
	//	p->fileAdded(file);
	
}

/*!
	\brief Adds a subfolder to the node (if possible)
	\param folder name of the folder to create
	\note the default implementation does nothing
*/
void QProjectNode::addFolder(const QString& folder)
{
	Q_UNUSED(folder)
}

/*!
	\brief Overwrites the a child with another
	\param idx child pos to overwrite
	\param p new child to place at pos \a idx
	\return old child or zero if index out of bounds
*/
QProjectNode* QProjectNode::overwrite(int idx, QProjectNode *p)
{
	if ( idx > m_children.count() )
		return 0;
	
	QProjectNode *old = m_children.at(idx);
	int nrow = old->row();
	
	if ( old->rowSpan() )
		beginRemoveRows(this, nrow, nrow + old->rowSpan() - 1);
	
	m_children.removeAt(idx);
	
	old->m_parent = 0;
	old->m_model = 0;
	
	if ( old->rowSpan() )
		endRemoveRows();
	
	if ( p->rowSpan() )
		beginInsertRows(this, nrow, nrow + rowSpan() - 1);
	
	p->m_parent = this;
	p->m_model = m_model;
	
	m_children.insert(idx, p);
	
	if ( p->rowSpan() )
		endInsertRows();
	
	return old;
}

/*!
	\brief Managed insertion handler
*/
void QProjectNode::appendChild(QProjectNode *p)
{
	//if ( !m_children.contains(p) )
		m_children.append(p);
	
	QProject *pro = project();
	
	if ( !pro )
		pro = dynamic_cast<QProject*>(this);
	
	if ( pro && (p->type() == File) )
		pro->fileAdded(pro->absoluteFilePath(p->name()));
}

/*!
	\brief Managed removal handler
*/
void QProjectNode::removeChild(QProjectNode *p)
{
	m_children.removeAll(p);
}

/*!

*/
void QProjectNode::insertChild(QProjectNode *p, int index)
{
	//if ( !m_children.contains(p) )
		m_children.insert(index, p);
	
	QProject *pro = project();
	
	if ( !pro )
		pro = dynamic_cast<QProject*>(this);
	
	if ( pro && (p->type() == File) )
		pro->fileAdded(pro->absoluteFilePath(p->name()));
}

/*!
	\brief helper to manage insertion of children
*/
void QProjectNode::beginInsertRows(QProjectNode *p, int beg, int end)
{
	if ( m_model )
		m_model->beginInsertRows(m_model->index(p), beg, end);
}

/*!
	\brief helper to manage removal of children
*/
void QProjectNode::beginRemoveRows(QProjectNode *p, int beg, int end)
{
	if ( m_model )
		m_model->beginRemoveRows(m_model->index(p), beg, end);
}

/*!
	\brief helper to manage insertion of children
*/
void QProjectNode::endInsertRows()
{
	if ( m_model )
		m_model->endInsertRows();
}

/*!
	\brief helper to manage removal of children
*/
void QProjectNode::endRemoveRows()
{
	if ( m_model )
		m_model->endRemoveRows();
}
