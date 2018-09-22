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

#ifndef _QMAKE_BACKEND_H_
#define _QMAKE_BACKEND_H_

#include "qproject.h"

#include <QCache>
#include <QLinkedList>
#include <QStringList>

class QIcon;

class QMakeSettings;

namespace QMake {
	QString property(const QString& v);
	
	static QStringList variables = QStringList()
		<< "SOURCES"
		<< "HEADERS"
		<< "FORMS"
		<< "RESOURCES"
		<< "TRANSLATIONS"
		<< "SUBDIRS"
		;
	
	static QList<QStringList> extensions = QList<QStringList>()
		<< (QStringList("cpp") << "cxx" << "c" << "cc")
		<< (QStringList("h") << "hxx" << "hpp")
		<< (QStringList("ui"))
		<< (QStringList("qrc"))
		<< (QStringList("ts"))
		<< (QStringList("pro"))
		;
	
	QString variableForFile(const QString& s);
}

namespace QMakeTokens
{
	static QLatin1String
		commentMark("#"),
		
		scopeOpenMark("{"),
		scopeCloseMark("}"),
		scopeNestMark(":"),
		scopeAlterMark("|"),
		
		functionOpenMark("("),
		functionCloseMark(")")
		;
	
	static QStringList variableMarks = QStringList()
		<< QLatin1String("=")
		<< QLatin1String("+=")
		<< QLatin1String("-=")
		<< QLatin1String("*=")
		<< QLatin1String("~=")
		,
	
	displayedVariables = QStringList()
		<< QLatin1String("HEADERS")
		<< QLatin1String("SOURCES")
		<< QLatin1String("FORMS")
		<< QLatin1String("RESOURCES")
		<< QLatin1String("TRANSLATIONS")
		<< QLatin1String("TEXTS")
		<< QLatin1String("PRECOMPILED_HEADER")
		<< QLatin1String("SUBDIRS")
		<< QLatin1String("QPLUGIN_SCHEMES")
		;
	
}

namespace QMakeModel
{
	class Node;
	class Project;
	
	enum DisplayFlag
	{
		None				= 0x00,
		FlatVariables		= 0x01,
		ShowAllVariables	= 0x02,
		ShowComments		= 0x04,
		ShowFunctions		= 0x08,
		
		All					= 0x0e
	};
	
	Q_DECLARE_FLAGS(DisplayMode, DisplayFlag)
	
	QMakeSettings* projectSettings();
	
	// FIXME : Flat variables cause troubles...
	// => probably something to fix in QPM core about rowSpan() and fragmentation...
	static DisplayMode displayMode = FlatVariables;
	
	struct INode
	{
		enum DataType
		{
			Null,
			Comment,
			Project,
			Include,
			Scope,
			Function,
			Variable,
			Value
		};
		
		enum State
		{
			Clean,
			Modified	= 0x01
		};
		
		typedef QProjectNode* (*Provider)(INode *n, bool dup);
		
		static INode* fromNode(QProjectNode *n);
		static const INode* fromNode(const QProjectNode *n);
		
		INode();
		INode(INode::DataType t, const QString& d);
		~INode();
		
		void setClean()
		{
			state &= ~Modified;
			
			foreach ( INode *c, children )
				if ( c )
					c->setClean();
		}
		
		void addFolder(const QString& file);
		void addFile(QProjectNode *n, const QString& file);
		
		void write(QTextStream& out, int indent, QProject *p);
		
		quint8 type;
		QString data;
		quint8 state;
		
		INode *parent;
		QList<INode*> children;
	};
	
	struct CacheKey
	{
		inline CacheKey() : node(0) {}
		inline CacheKey(const QProjectNode *n, const QString& v)
		: node(n), variable(v) {}
		
		inline bool isNull() const
		{ return !node; }
		
		inline bool isValid() const
		{ return node; }
		
		inline bool operator == (const CacheKey& o) const
		{ return (node == o.node) && (variable == o.variable); }
		
		inline bool operator != (const CacheKey& o) const
		{ return (node != o.node) || (variable != o.variable); }
		
		const QProjectNode *node;
		QString variable;
	};
	
	uint qHash(const CacheKey& u);
	
	class Search
	{
		private:
			static QCache<CacheKey, QStringList> m_cache;
			
		public:
			enum Substitution
			{
				NoSubstitution			= 0x00,
				SubstituteVariables		= 0x01,
				SubstituteProperties	= 0x02,
				SubstituteEnvironment	= 0x04,
				SubstituteFunctions		= 0x08,
				
				SubstituteAll 			= 0xff
			};
			
			enum Depth
			{
				Scope,
				File,
				Project,
				Whole
			};
			
			enum Operation
			{
				Unknown,
				Set,
				AddAlways,
				AddUnique,
				RemoveAll,
				ReplaceAll
			};
			
			static Operation operation(const QString& s);
			
			static bool hasValue(const QProjectNode *n,
								const QString& var,
								const QString& op,
								const QString& val,
								Depth d,
								const QStringList& config = QStringList(),
								Substitution sub = SubstituteAll);
			
			static QStringList substitute(const QString& s, const QProjectNode *n,
										Depth d,
										const QStringList& config = QStringList(),
										const QString& variable = QString(),
										const QProjectNode *from = 0,
										const QProjectNode *until = 0,
										Substitution sub = SubstituteAll);
			
			static QStringList compute(const QProjectNode *n, const QString& variable,
										const QStringList& config, Depth d,
										const QProjectNode *until = 0,
										Substitution sub = SubstituteAll);
			
			static bool compute(const QProjectNode *n, const QString& variable,
								QStringList& back, const QStringList& config, Depth d,
								const QProjectNode *from = 0,
								const QProjectNode *until = 0,
								Substitution sub = SubstituteAll);
			
			static void invalidateCache(INode *n, const QString& var);
	};
	
	class INodeBackingStore
	{
		friend INode::INode();
		friend INode::~INode();
		friend INode::INode(INode::DataType t, const QString& d);
		
		public:
			static INodeBackingStore* instance();
			
			INode* getFileNode(INode::DataType t, const QString& fn, bool *shared);
			
			void hook(QProjectNode *p, INode *n);
			void unhook(QProjectNode *p, INode *n);
			
			void appendChild(INode *n, INode *c, INode::Provider provider, bool dup = false);
			void removeChild(INode *n, INode *c, QProjectNode *m = 0);
			
			QList<QProjectNode*> mappings(INode *n) const;
			QProjectNode* mappingClone(INode *n) const;
			
		private:
			QList<INode*> m_files;
			QLinkedList<INode*> m_bucket;
			QHash<INode*, QList<QProjectNode*> > m_mappings;
			
			static INodeBackingStore *m_instance;
	};
	
	QIcon icon(int idx);
	
	void duplicate(QProjectNode *p, INode *d);
	
	class Node : public QProjectNode
	{
		friend struct INode;
		
		public:
			Node(NodeType t, INode *n, bool dup = false);
			virtual ~Node();
			
			virtual QProjectNode* clone() const;
			
			virtual QVariant data(int role) const;
			virtual bool setData(const QVariant& v, int role);
			
			virtual QString name() const;
			
			virtual QProjectNode* parent() const;
			
			virtual int row() const;
			virtual int rowSpan() const;
			virtual int rowCount() const;
			virtual bool isFragmented() const;
			
			virtual ActionList actions() const;
			virtual void actionTriggered(const QString& label);
			
			virtual void addFile(const QString& file);
			virtual void addFolder(const QString& folder);
			
			virtual void removeChild(QProjectNode *n);
			
			virtual void write(QTextStream& out, int indent) const;
			
			virtual QProjectNode::DefaultActions defaultActions() const;
			
		private:
			INode *d;
	};
	
	class Project : public QProject
	{
		friend struct INode;
		
		public:
			Project(INode *n, bool dup = false);
			virtual ~Project();
			
			virtual QProjectNode* clone() const;
			
			virtual int row() const;
			virtual QProjectNode* parent() const;
			
			virtual QVariant data(int role) const;
			
			virtual QString name() const;
			virtual QString backend() const;
			
			virtual QString absoluteFilePath(const QString& fn) const;
			
			virtual void addFile(const QString& file);
			virtual void addFolder(const QString& folder);
			
			virtual void removeChild(QProjectNode *n);
			
			virtual void write(QTextStream& out, int indent) const;
			
			virtual void save();
			
			virtual void settings();
			
			virtual QString query(const QString& s) const;
			
			virtual QStringList files(ComputationMode m = NonRecursive) const;
			
			virtual void setModified(bool m);
			
		private:
			INode *d;
	};
	
	QProject* getProjectNode(const QString& fn);
}

QProjectNode* NodeProvider(QMakeModel::INode *n, bool dup);
QProjectNode* ProjectProvider(QMakeModel::INode *n, bool dup);

void setNextNodeType(QProjectNode::NodeType t);

Q_DECLARE_OPERATORS_FOR_FLAGS(QMakeModel::DisplayMode)

#endif // !_QMAKE_BACKEND_H_
