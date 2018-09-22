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

#ifndef _QPROJECT_NODE_H_
#define _QPROJECT_NODE_H_

#include "qpm-config.h"

/*!
	\file qprojectnode.h
	\brief Definition of the QProjectNode class.
*/

#include <QList>
#include <QIcon>
#include <QString>
#include <QVariant>

class QTextStream;

class QProject;
class QProjectModel;

class QPM_EXPORT QProjectNode
{
	friend class QProjectView;
	friend class QProjectModel;
	
	public:
		enum NodeType
		{
			File,
			Folder,
			Project,
			Other
		};
		
		enum DefaultAction
		{
			None		= 0x00,
			Open		= 0x01,
			Save		= 0x02,
			Remove		= 0x04,
			Rename		= 0x08,
			AddFile		= 0x10,
			AddFolder	= 0x20,
			Settings	= 0x40
		};
		
		Q_DECLARE_FLAGS(DefaultActions, DefaultAction)
		
		struct Action
		{
			inline Action(const QString& l)
			 : label(l) {}
			
			inline Action(const QIcon& i, const QString& l)
			 : icon(i), label(l) {}
			
			QIcon icon;
			QString label;
		};
		
		typedef QList<Action> ActionList;
		
		virtual ~QProjectNode();
		
		virtual void clear();
		virtual void destroy();
		
		NodeType type() const;
		
		virtual int row() const;
		
		virtual QProjectNode* clone() const = 0;
		
		QProjectModel* model() const;
		virtual QProject* project() const;
		virtual QProjectNode* parent() const;
		
		virtual QVariant data(int role) const;
		virtual bool setData(const QVariant& v, int role);
		
		virtual QString name() const = 0;
		
		virtual int rowSpan() const;
		virtual int rowCount() const;
		virtual bool isFragmented() const;
		
		virtual QList<QProjectNode*> children() const;
		
		virtual int visualRow(int index) const;
		virtual int visualRow(const QProjectNode *c) const;
		virtual QProjectNode* childAt(int visaulRow) const;
		
		virtual void detach(bool blockFileSig = false);
		virtual void attach(QProjectNode *p, int index = -1);
		virtual void attach(QProjectNode *p, QProjectNode *proxy, int index = -1);
		
		virtual void write(QTextStream& out, int indent) const;
		
		virtual ActionList actions() const;
		virtual DefaultActions defaultActions() const;
		
		virtual void addFile(const QString& s);
		virtual void addFolder(const QString& s);
		
		virtual void actionTriggered(const QString& label);
		
		static void flushDelayedDeletions();
		static void delayedDeletion(QProjectNode *n);
		
	protected:
		QProjectNode(NodeType t);
		
		void beginInsertRows(QProjectNode *p, int beg, int end);
		void beginRemoveRows(QProjectNode *p, int beg, int end);
		void endInsertRows();
		void endRemoveRows();
		
		virtual QProjectNode* overwrite(int idx, QProjectNode *p);
		
		virtual void appendChild(QProjectNode *p);
		virtual void removeChild(QProjectNode *p);
		virtual void insertChild(QProjectNode *p, int index);
		
	private:
		NodeType m_type;
		QProjectModel *m_model;
		QProjectNode *m_parent;
		QList<QProjectNode*> m_children;
		
		static QList<QProjectNode*> _delayed;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QProjectNode::DefaultActions)

#endif // !_QPROJECT_NODE_H_
