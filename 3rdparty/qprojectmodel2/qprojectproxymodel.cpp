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

#include "qprojectproxymodel.h"

/*!
	\file qprojectproxymodel.cpp
	\brief Implementation of the QProjectProxyModel class.
*/

QProjectProxyModel::QProjectProxyModel(QObject *p)
 : QSortFilterProxyModel(p), m_detailLevel(0), m_model(0)
{
	
}

int QProjectProxyModel::detailLevel() const
{
	return m_detailLevel;
}

void QProjectProxyModel::setDetailLevel(int lvl)
{
	emit detailLevelChanged(m_detailLevel, lvl);
	
	m_detailLevel = lvl;
	
	invalidateFilter();
}

void QProjectProxyModel::setSourceModel(QAbstractItemModel *m)
{
	QAbstractItemModel *o = sourceModel();
	
	if ( o )
	{
		m_model = 0;
		
		disconnect( o	, SIGNAL( requestEdit(QModelIndex) ),
					this, SLOT  ( forwardEdit(QModelIndex) ) );
	}
	
	QSortFilterProxyModel::setSourceModel(m);
	
	if ( m )
	{
		m_model = qobject_cast<QProjectModel*>(m);
		
		connect(m	, SIGNAL( requestEdit(QModelIndex) ),
				this, SLOT  ( forwardEdit(QModelIndex) ) );
	}
}

void QProjectProxyModel::edit(const QModelIndex& idx)
{
	emit requestEdit(mapFromSource(idx));
}

void QProjectProxyModel::forwardEdit(const QModelIndex& idx)
{
	emit requestEdit(mapFromSource(idx));
}

QProjectNode* QProjectProxyModel::node(const QModelIndex& idx) const
{
	return m_model->node(mapToSource(idx));
}

QModelIndex QProjectProxyModel::mapToSource(const QModelIndex& proxyIndex) const
{
	return QSortFilterProxyModel::mapToSource(proxyIndex);
}

QModelIndex QProjectProxyModel::mapFromSource(const QModelIndex& sourceIndex) const
{
	return QSortFilterProxyModel::mapFromSource(sourceIndex);
}

bool QProjectProxyModel::filterAcceptsRow(int row, const QModelIndex& parent) const
{
	if ( m_detailLevel < 0 )
		return true;
	
	QModelIndex index = sourceModel()->index(row, 0, parent);
	
	return sourceModel()->data(index, QProjectModel::DetailLevelRole).toInt() <= m_detailLevel;
}
