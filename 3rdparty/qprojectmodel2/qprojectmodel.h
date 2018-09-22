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

#ifndef _QPROJECT_MODEL_H_
#define _QPROJECT_MODEL_H_

#include "qpm-config.h"

/*!
	\file qprojectmodel.h
	\brief Definition of the QProjectModel class.
*/

#include <QHash>
#include <QList>
#include <QPointer>
#include <QAbstractItemModel>

class QProject;
class QProjectNode;
class QProjectLoader;
class QFileSystemWatcher;

class QPM_EXPORT QProjectModel : public QAbstractItemModel
{
	friend class QProject;
	friend class QProjectNode;
	friend class QProjectView;
	
	Q_OBJECT
	
	public:
		enum ExtraRoles
		{
			DetailLevelRole = Qt::UserRole + 1
		};
		
		class EditorWrapper
		{
			public:
				virtual ~EditorWrapper() {}
				virtual void edit(const QModelIndex& idx) = 0;
		};
		
		QProjectModel(QObject *p = 0);
		virtual ~QProjectModel();
		
		virtual QVariant data(const QModelIndex& index, int role) const;
		virtual bool setData(const QModelIndex& index, const QVariant& d, int role);
		
		virtual Qt::ItemFlags flags(const QModelIndex& index) const;
		
		virtual QVariant headerData(int section, Qt::Orientation orientation,
									int role = Qt::DisplayRole) const;
		
		virtual QModelIndex index(QProjectNode *n) const;
		
		virtual QModelIndex index(	int row, int column,
									const QModelIndex& parent = QModelIndex()) const;
		
		virtual QModelIndex parent(const QModelIndex &index) const;
		
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
		virtual bool hasChildren(const QModelIndex& parent = QModelIndex()) const;
		
		virtual Qt::DropActions supportedDropActions() const;
		
		virtual QStringList mimeTypes() const;
		virtual QMimeData* mimeData(const QModelIndexList& indexes) const;
		
		virtual void invalidateIndexes(QProjectNode *n);
		
		virtual bool dropMimeData(	const QMimeData *data, Qt::DropAction action,
									int row, int column, const QModelIndex& parent);
		
		QProjectNode* node(const QModelIndex& idx) const;
		
		int projectCount(bool recursive = false) const;
		QList<QProject*> projects(bool recursive = false) const;
		QProject* project(const QString& n) const;
		
		QProjectLoader* projectLoader() const;
		void setProjectLoader(QProjectLoader *l);
		
		bool tryCommit(QProject *p);
		
		void saveAll();
		bool tryCommitAll();
		bool closeAll(bool force = false);
		
		void addEditorWrapper(EditorWrapper *w);
		void removeEditorWrapper(EditorWrapper *w);
		
	public slots:
		void addProject(QProject *p);
		void removeProject(QProject *p);
		
		virtual bool openProject(const QString& s);
		virtual void saveProject(const QString& s);
		virtual bool closeProject(const QString& s);
		
		void fileChanged(const QString& file);
		
		void edit(QProjectNode *n);
		
	signals:
		void requestActivation(QProject *p);
		void requestEdit(const QModelIndex& idx);
		
		void projectAdded(QProject *p);
		void projectRemoved(QProject *p);
		void projectReloaded(QProject *p, QProject *n);
		
		void projectAdded(const QString& f);
		void projectRemoved(const QString& f);
		
		void fileAdded(const QString& file, QProject *project);
		void fileRemoved(const QString& file, QProject *project);
		
		void fileActivated(const QString& file);
		
		void reloadingProject(QProject *p);
		
	protected:
		void beginInsertRows(const QModelIndex idx, int beg, int end);
		void beginRemoveRows(const QModelIndex idx, int beg, int end);
		void endInsertRows();
		void endRemoveRows();
		
		void dataChanged(QProjectNode *n);
		
		void forget(QProject *p);
		void memorize(QProject *p);
		
		void tryEdit(const QModelIndex& idx);
		
	private:
		struct LoadInterception
		{
			QString file;
			
			int index;
			QProject *old;
			QProjectNode *parent;
		};
		
		QProjectLoader *m_loader;
		QList<QProject*> m_topLevel;
		QFileSystemWatcher *m_watcher;
		QList<EditorWrapper*> m_editors;
		QList<LoadInterception> m_intercept;
};

#endif // !_QPROJECT_MODEL_H_
