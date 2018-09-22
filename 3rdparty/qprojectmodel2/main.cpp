
#include "qproject.h"
#include "qprojectview.h"
#include "qprojectmodel.h"
#include "qprojectparser.h"
#include "qprojectloader.h"

#include "qmakeparser.h"

#include <QStack>
#include <QAtomic>
#include <QFileInfo>
#include <QApplication>

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	
	QProjectView view;
	QProjectModel model;
	
	model.setProjectLoader(new QProjectLoader(&model));
	model.projectLoader()->addParser(new QMakeParser);
	
	for ( int i = 1; i < argc; ++i )
		model.openProject(argv[i]);
	
	view.setModel(&model);
	view.show();
	
	return app.exec();
}

#if 0
// <borrowed origin="QMake sources">

#include "qmake/project.h"
#include "qmake/property.h"
#include "qmake/option.h"
#include "qmake/cachekeys.h"
#include <qnamespace.h>
#include <qregexp.h>
#include <qdir.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

QString project_builtin_regx() //calculate the builtin regular expression..
{
	QString ret;
	QStringList builtin_exts;
	builtin_exts << Option::c_ext << Option::ui_ext << Option::yacc_ext << Option::lex_ext << ".ts" << ".qrc";
	builtin_exts += Option::h_ext + Option::cpp_ext;
	for(int i = 0; i < builtin_exts.size(); ++i) {
		if(!ret.isEmpty())
			ret += "; ";
		ret += QString("*") + builtin_exts[i];
	}
	return ret;
}

/* This is to work around lame implementation on Darwin. It has been noted that the getpwd(3) function
   is much too slow, and called much too often inside of Qt (every fileFixify). With this we use a locally
   cached copy because I can control all the times it is set (because Qt never sets the pwd under me).
*/
static QString pwd;
QString qmake_getpwd()
{
	if(pwd.isNull())
		pwd = QDir::currentPath();
	return pwd;
}
bool qmake_setpwd(const QString &p)
{
	if(QDir::setCurrent(p)) {
		pwd = QDir::currentPath();
		return true;
	}
	return false;
}

// </borrowed>
#endif

#if 0
namespace QMakeModel
{
	static char _cmd[] = "qmake"; // { 'q', 'm', 'a', 'k', 'e', '\0' };
	static char* _argv[] = {
		_cmd,
		0
	};
	
	static QStringList m_displayed = QStringList()
		<< "HEADERS"
		<< "SOURCES"
		<< "FORMS"
		<< "RESOURCES"
		<< "TRANSLATIONS"
		<< "TEXTS"
		<< "PRECOMPILED_HEADER"
		;
	
	class NodeData
	{
		public:
			enum Type
			{
				Scope,
				Variable,
				Value,
				Other
			};
			
			NodeData(Type type) : m_type(type) {}
			
			inline void ref(QProjectNode *n) { m_owners << n; }
			inline void deref(QProjectNode *n) { m_owners.removeAll(n); }
			
			void append(QProjectNode *pn, QProjectNode *from);
			void remove(QProjectNode *pn, QProjectNode *from);
			
			bool m_lock;
			Type m_type;
			QString m_id, m_name;
			QList<NodeData*> m_children;
			QList<QProjectNode*> m_owners;
	};
	
	class Provider
	{
		public:
			~Provider() { qDeleteAll(m_data); }
			
			NodeData* getData(NodeData::Type t, const QString& id, const QString& name)
			{
				foreach ( NodeData *n, m_data )
				{
					if ( (n->m_type == t) && (n->m_id == id) )
					{
						if ( n->m_name != name )
							qWarning("id confilct...");
						
						return n;
					}
				}
				
				//qDebug("creating data for : %s", qPrintable(name));
				
				NodeData *n = new NodeData(t);
				n->m_id = id;
				n->m_name = name;
				
				return n;
			}
			
		private:
			QList<NodeData*> m_data;
	};
	
	class Project : public QProject
	{
		friend void NodeData::append(QProjectNode* pn, QProjectNode *from);
		friend void NodeData::remove(QProjectNode* pn, QProjectNode *from);
		
		public:
			Project(NodeData *nd)
			 : QProject(), d(nd)
			{ if ( d ) d->ref(this); }
			
			~Project()
			{ if ( d ) d->deref(this); }
			
			virtual QProjectNode* clone() const
			{
				return new Project(d);
			}
			
			virtual QString name() const
			{ return d ? d->m_name : QString(); }
			
			virtual void appendChild(QProjectNode *pn)
			{
				if ( d ) d->append(pn, this);
				
				QProject::appendChild(pn);
			}
			
			virtual void removeChild(QProjectNode *pn)
			{
				if ( d ) d->remove(pn, this);
				
				QProject::removeChild(pn);
			}
			
		private:
			NodeData *d;
	};
	
	
	class Node : public QProjectNode
	{
		friend void NodeData::append(QProjectNode *pn, QProjectNode *from);
		friend void NodeData::remove(QProjectNode *pn, QProjectNode *from);
		
		public:
			Node(NodeType t, NodeData *nd)
			: QProjectNode(t), d(nd)
			{}
			
			virtual QProjectNode* clone() const
			{
				return new Node(type(), d);
			}
			
			virtual QString name() const
			{
				QString n = d ? d->m_name : QString();
				
				if ( n.count() && (type() == File) )
				{
					QProject *p = project();
					
					if ( p )
						n = p->absoluteFilePath(n);
					
				}
				
				return n;
			}
			
			virtual int rowSpan() const
			{
				if ( d->m_type == NodeData::Variable )
				{
					if ( m_displayed.contains(name()) )
						return 1;
					else
						return 0;
				}
				
				return QProjectNode::rowSpan();
			}
			
			virtual int rowCount() const
			{
				if ( d->m_type == NodeData::Variable )
				{
					if ( m_displayed.contains(name()) )
						return QProjectNode::rowCount();
					else
						return 0;
				}
				
				return QProjectNode::rowCount();
			}
			
			virtual void appendChild(QProjectNode *pn)
			{
				if ( d ) d->append(pn, this);
				
				QProjectNode::appendChild(pn);
			}
			
			virtual void removeChild(QProjectNode *pn)
			{
				if ( d ) d->remove(pn, this);
				
				QProjectNode::removeChild(pn);
			}
			
		private:
			NodeData *d;
	};
	
	void NodeData::append(QProjectNode *pn, QProjectNode *from)
	{
		if ( m_lock )
			return;
		
		m_lock = true;
		
		Node *n = dynamic_cast<Node*>(pn);
		Project *p = dynamic_cast<Project*>(pn);
		
		if ( n && n->d )
			m_children.append(n->d);
		else if ( p && p->d )
			m_children.append(p->d);
		
		foreach ( QProjectNode *o, m_owners )
		{
			if ( o == from )
				continue;
			
			QProjectNode *c = pn->clone();
			c->attach(o);
		}
		
		m_lock = false;
	}
	
	void NodeData::remove(QProjectNode *pn, QProjectNode *from)
	{
		if ( m_lock )
			return;
		
		m_lock = true;
		
		NodeData *d = 0;
		Node *n = dynamic_cast<Node*>(pn);
		Project *p = dynamic_cast<Project*>(pn);
		
		if ( n )
			d = n->d;
		else if ( p )
			d = p->d;
		
		m_children.removeAll(d);
		
		foreach ( QProjectNode *o, m_owners )
		{
			if ( o == from )
				continue;
			
			foreach ( QProjectNode *n, d->m_owners )
				n->detach();
		}
		
		m_lock = false;
	}
	
	class Parser : public QProjectParser
	{
		public:
			virtual bool canOpen(const QString& file) const
			{
				return QFileInfo(file).suffix() == "pro";
			}
			
			virtual QProject* open(const QString& fn)
			{
				QString file = QFileInfo(fn).absoluteFilePath();
				
				QProject *pnode = 0;
				
				if ( _argv[1] )
					delete _argv[1];
				
				_argv[1] = new char[file.count()];
				qstrcpy(_argv[1], qPrintable(file));
				
				// <borrowed origin="QMake sources">
				
				int ret = Option::init(2, _argv);
				
				if ( ret != Option::QMAKE_CMDLINE_SUCCESS )
				{
					if ((ret & Option::QMAKE_CMDLINE_ERROR) != 0)
					{
						return 0;
					}
					
					return 0;
				}
				
				// rectify things...
				Option::qmake_mode = Option::QMAKE_GENERATE_MAKEFILE;
				Option::mkfile::project_files = QStringList() << file;
				
				QString oldpwd = qmake_getpwd();
				#ifdef Q_WS_WIN
				if(!(oldpwd.length() == 3 && oldpwd[0].isLetter() && oldpwd.endsWith(":/")))
				#endif
				{
					if(oldpwd.right(1) != QString(QChar(QDir::separator())))
						oldpwd += QDir::separator();
				}
				
				Option::output_dir = oldpwd; //for now this is the output dir
				
				if ( Option::output.fileName() != "-" )
				{
					QFileInfo fi(Option::output);
					QString dir;
					if ( fi.isDir() )
					{
						dir = fi.filePath();
					} else {
						QString tmp_dir = fi.path();
						if(!tmp_dir.isEmpty() && QFile::exists(tmp_dir))
							dir = tmp_dir;
					}
					
					if ( !dir.isNull() && dir != "." )
						Option::output_dir = dir;
					
					if ( QDir::isRelativePath(Option::output_dir) )
						Option::output_dir.prepend(oldpwd);
					
					Option::output_dir = QDir::cleanPath(Option::output_dir);
				}
				
				QMakeProperty prop;
				QMakeProject project(&prop);
				int exit_val = 0;
				QStringList files;
				files = Option::mkfile::project_files;
				
				for ( QStringList::Iterator pfile = files.begin(); pfile != files.end(); ++pfile )
				{
					QString fn = Option::fixPathToLocalOS((*pfile));
					
					if ( !QFile::exists(fn) )
					{
						fprintf(stderr, "Cannot find file: %s.\n", fn.toLatin1().constData());
						exit_val = 2;
						continue;
					}
					
					//setup pwd properly
					debug_msg(1, "Resetting dir to: %s", oldpwd.toLatin1().constData());
					qmake_setpwd(oldpwd); //reset the old pwd
					int di = fn.lastIndexOf(Option::dir_sep);
					if ( di != -1 )
					{
						debug_msg(1, "Changing dir to: %s", fn.left(di).toLatin1().constData());
						if(!qmake_setpwd(fn.left(di)))
							fprintf(stderr, "Cannot find directory: %s\n", fn.left(di).toLatin1().constData());
						fn = fn.right(fn.length() - di - 1);
					}
					
					// read project..
					if( !project.read(fn) )
					{
						fprintf(stderr, "Error processing project file: %s\n",
								fn == "-" ? "(stdin)" : (*pfile).toLatin1().constData());
						exit_val = 3;
						continue;
					}
					
					if ( Option::mkfile::do_preprocess ) //no need to create makefile
						continue;
					
					// </borrowed>
					
					if ( !pnode )
					{
						pnode = new Project(m_provider.getData(NodeData::Scope, file, file));
					} else {
						qWarning("Several projects loaded???");
					}
					
					// fill the tree with computed vars
					QMap<QString, QStringList> vars = project.variables();
					QMap<QString, QStringList>::const_iterator it = vars.constBegin();
					
					while ( it != vars.constEnd() )
					{
						QString base = file + "/" + it.key();
						
						Node *n = new Node(QProjectNode::Other,
										   m_provider.getData(NodeData::Variable, base, it.key())
										  );
						
						n->attach(pnode);
						
						base += "/";
						
						foreach ( QString v, *it )
						{
							Node *c =
								new Node(
											m_displayed.contains(it.key())
										?
											QProjectNode::File
										:
											QProjectNode::Other,
										m_provider.getData(
											NodeData::Value,
											base + v,
											v
											)
										);
							
							c->attach(n);
						}
						
						++it;
					}
				}
				
				qmakeClearCaches();
				
				return pnode;
			}
			
		private:
			Provider m_provider;
	};
}
#endif
