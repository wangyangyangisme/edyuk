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

#ifndef _QPROJECT_PROXY_MODEL_H_
#define _QPROJECT_PROXY_MODEL_H_

#include "qpm-config.h"

/*!
	\file qprojectproxymodel.h
	\brief Definition of the QProjectProxyModel class.
*/

#include <QSortFilterProxyModel>

#include "qprojectmodel.h"

class QPM_EXPORT QProjectProxyModel : public QSortFilterProxyModel, public QProjectModel::EditorWrapper
{
	Q_OBJECT
	
	public:
		QProjectProxyModel(QObject *p = 0);
		
		QProjectNode* node(const QModelIndex& idx) const;
		
		virtual void edit(const QModelIndex& idx);
		virtual void setSourceModel(QAbstractItemModel *m);
		
		virtual QModelIndex mapToSource(const QModelIndex& proxyIndex) const;
		virtual QModelIndex mapFromSource(const QModelIndex& sourceIndex) const;
		
		int detailLevel() const;
		
	public slots:
		void setDetailLevel(int lvl);
		
	signals:
		void requestEdit(const QModelIndex& idx);
		
		void detailLevelChanged(int o, int n);
		
	protected:
		virtual bool filterAcceptsRow(int row, const QModelIndex& parent) const;
		
	private slots:
		void forwardEdit(const QModelIndex& idx);
		
	private:
		int m_detailLevel;
		QProjectModel *m_model;
};

#endif // !_QPROJECT_PROXY_MODEL_H_
