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

#include "qmakebackend.h"

#include "uisubclass.h"

#include "qmakeparser.h"
#include "qmakesettings.h"

#include "gnumakebuilder.h"

#include "qprojectmodel.h"

#include "plugin.h"

#include <QDir>
#include <QIcon>
#include <QHash>
#include <QLabel>
#include <QCache>
#include <QDialog>
#include <QLibrary>
#include <QUiLoader>
#include <QFileInfo>
#include <QCheckBox>
#include <QGridLayout>
#include <QTextStream>
#include <QLibraryInfo>
#include <QDialogButtonBox>

class WatchHooker
{
	public:
		WatchHooker(const QString& k, PropertyWatch w)
		{
			DefaultPlugin::addWatch(k, w);
		}
};

static void setFlatVariables(const QVariant& v)
{
	bool on = v.toBool();
	
	if ( on )
		QMakeModel::displayMode |= QMakeModel::FlatVariables;
	else
		QMakeModel::displayMode &= ~QMakeModel::FlatVariables;
}

static const WatchHooker hookFlatVariables("QProjectParser/QMakeParser/flat", setFlatVariables);

static QProjectNode::NodeType m_nextNodeType;

void setNextNodeType(QProjectNode::NodeType t)
{ m_nextNodeType = t; }

QProjectNode* NodeProvider(QMakeModel::INode *n, bool dup)
{
	return new QMakeModel::Node(m_nextNodeType, n, dup);
}

QProjectNode* ProjectProvider(QMakeModel::INode *n, bool dup)
{
	return new QMakeModel::Project(n, dup);
}

QString QMake::property(const QString& v)
{
	if(v == "QT_INSTALL_PREFIX")
		return QLibraryInfo::location(QLibraryInfo::PrefixPath);
	else if(v == "QT_INSTALL_DATA")
		return QLibraryInfo::location(QLibraryInfo::DataPath);
	else if(v == "QT_INSTALL_DOCS")
		return QLibraryInfo::location(QLibraryInfo::DocumentationPath);
	else if(v == "QT_INSTALL_HEADERS")
		return QLibraryInfo::location(QLibraryInfo::HeadersPath);
	else if(v == "QT_INSTALL_LIBS")
		return QLibraryInfo::location(QLibraryInfo::LibrariesPath);
	else if(v == "QT_INSTALL_BINS")
		return QLibraryInfo::location(QLibraryInfo::BinariesPath);
	else if(v == "QT_INSTALL_PLUGINS")
		return QLibraryInfo::location(QLibraryInfo::PluginsPath);
	else if(v == "QT_INSTALL_TRANSLATIONS")
		return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
	else if(v == "QT_INSTALL_CONFIGURATION")
		return QLibraryInfo::location(QLibraryInfo::SettingsPath);
	else if(v == "QT_INSTALL_EXAMPLES")
		return QLibraryInfo::location(QLibraryInfo::ExamplesPath);
	else if(v == "QT_INSTALL_DEMOS")
		return QLibraryInfo::location(QLibraryInfo::DemosPath);
	else if(v == "QMAKE_MKSPECS")
		return "";
	else if(v == "QMAKE_VERSION")
		return "";
	else if(v == "QT_VERSION")
		return QT_VERSION_STR;
	
	return QString();
}

QString QMake::variableForFile(const QString& s)
{
	QFileInfo info(s);
	
	for ( int i = 0; i < extensions.count(); ++i )
		if ( extensions.at(i).contains(info.suffix()) )
			return variables.at(i);
	
	return "TEXTS";
}

QMakeSettings* QMakeModel::projectSettings()
{
	static QPointer<QMakeSettings> s;
	
	if ( !s )
	{
		s = new QMakeSettings;
		s->setAttribute(Qt::WA_DeleteOnClose, true);
	}
	
	return s;
}

static bool needsQuote(const QString& s)
{
	static const QRegExp spaces("\\s");
	
	return s.contains(spaces) && !s.startsWith('\"') && !s.startsWith("$$quote(");
}

static bool looksLikeElseChain(QString prev, QString cur)
{
	bool prevHasNeg = prev.count() ? prev.at(0) == QLatin1Char('!') : false,
		 thisHasNeg = cur.count() ? cur.at(0) == QLatin1Char('!') : false;
	
	if ( prevHasNeg == thisHasNeg )
		return false;
	
	return prevHasNeg
		?
			(cur == QStringRef(&prev, 1, prev.count() - 1))
		:
			(prev == QStringRef(&cur, 1, cur.count() - 1))
		;
}


QMakeModel::INode::INode()
 : type(Null), state(0), parent(0)
{ INodeBackingStore::instance()->m_bucket.append(this); }

QMakeModel::INode::INode(DataType t, const QString& d)
 : type(t), data(d), state(0), parent(0)
{ INodeBackingStore::instance()->m_bucket.append(this); }

QMakeModel::INode::~INode()
{
	QLinkedList<INode*>::iterator i = INodeBackingStore::instance()->m_bucket.begin();
	
	while ( i != INodeBackingStore::instance()->m_bucket.end() )
		if ( *i == this )
			i = INodeBackingStore::instance()->m_bucket.erase(i);
		else
			++i;
	
}

void QMakeModel::INode::write(QTextStream& out, int indent, QProject *p)
{
	QString leading(indent, '\t');
	
	switch ( type )
	{
		case Comment :
		{
			out << leading << "#" << data << "\n";
			
			int idx = parent ? parent->children.indexOf(const_cast<INode*>(this)) : -1,
				max = parent ? parent->children.count() : -1;
			
			INode *next = ((idx != -1) && ((idx +1) < max)) ? parent->children.at(idx + 1) : 0;
			
			if ( next && (next->type != Comment) )
				out << "\n";
			
			break;
		}
			
		case Variable :
		{
			if ( children.isEmpty() || (data.indexOf(' ') == -1) )
				break;
			
			if ( p && (data.left(data.indexOf(' ')) == "SUBDIRS") )
			{
				out << leading << data << " " << p->relativeFilePath(children.at(0)->data);
				
				for ( int i = 1; i < children.count(); ++i )
				{
					out << " \\\n" << leading << "\t" << p->relativeFilePath(children.at(i)->data);
					
					QList<QProjectNode*> l = INodeBackingStore::instance()->mappings(children.at(i));
					
					if ( l.count() )
					{
						QProject *p = dynamic_cast<QProject*>(l.at(0));
						
						if ( p->isModified() )
							p->save();
					}
				}
			} else {
				out << leading << data << " ";
				
				if ( needsQuote(children.at(0)->data) )
					out << "$$quote(" << children.at(0)->data << ")";
				else
					out << children.at(0)->data;
				
				for ( int i = 1; i < children.count(); ++i )
				{
					out << " \\\n" << leading << "\t";
					
					if ( needsQuote(children.at(i)->data) )
						out << "$$quote(" << children.at(i)->data << ")";
					else
						out << children.at(i)->data;
				}
			}
			
			out << "\n";
			
			int idx = parent ? parent->children.indexOf(const_cast<INode*>(this)) : -1,
				max = parent ? parent->children.count() : -1;
			
			INode *next = ((idx != -1) && ((idx +1) < max)) ? parent->children.at(idx + 1) : 0;
			
			if ( next )
				out << "\n";
			
			break;
		}
			
		case Scope :
		{
			int idx = parent ? parent->children.indexOf(const_cast<INode*>(this)) : -1,
				max = parent ? parent->children.count() : -1;
			
			INode *prev = (idx > 0) ? parent->children.at(idx - 1) : 0;
			
			QString sn = (prev && looksLikeElseChain(prev->data, data)) ? "else" : data;
			
			out << leading << sn << " {\n";
			
			foreach ( INode *n, children )
			{
				n->write(out, indent + 1, p);
			}
			
			out << leading << "}";
			
			INode *next = ((idx +1) < max) ? parent->children.at(idx + 1) : 0;
			
			out << ((next && (looksLikeElseChain(next->data, data) || (next->data == "else"))) ? " " : "\n\n");
			
			break;
		}
			
		case Function :
		{
			out << leading << data << "\n";
			
			int idx = parent ? parent->children.indexOf(const_cast<INode*>(this)) : -1,
				max = parent ? parent->children.count() : -1;
			
			INode *next = ((idx != -1) && ((idx +1) < max)) ? parent->children.at(idx + 1) : 0;
			
			if ( next && (next->type != Function) )
				out << "\n";
			
			break;
		}
			
		case Include :
		{
			out << leading << "include(" << (p ? p->relativeFilePath(data) : data) << ")\n";
			
			int idx = parent ? parent->children.indexOf(const_cast<INode*>(this)) : -1,
				max = parent ? parent->children.count() : -1;
			
			INode *next = ((idx != -1) && ((idx +1) < max)) ? parent->children.at(idx + 1) : 0;
			
			if ( next && (next->type != Include) )
				out << "\n";
			
			// save the include file itself?
			//if ( !(state & Modified) )
			//	break;
			
			//qDebug("saving modified include : %s", qPrintable(data));
			
			QFile file(data);
			
			if ( file.open(QFile::WriteOnly | QFile::Text) )
			{
				QTextStream out(&file);
				
				foreach ( INode *c, children )
					if ( c )
						c->write(out, 0, 0);
				
			} else {
				qWarning("Failed to open file %s for writing", qPrintable(data));
			}
			
			setClean();
			
			break;
		}
			
		default:
			
			break;
	}
}

uint QMakeModel::qHash(const CacheKey& u)
{
	return qHash(QString::number((unsigned long)u.node) + ":" + u.variable);
}

QCache<QMakeModel::CacheKey, QStringList> QMakeModel::Search::m_cache;

QMakeModel::Search::Operation QMakeModel::Search::operation(const QString& s)
{
	if ( s == "=" )
		return Set;
	if ( s == "+=" )
		return AddAlways;
	if ( s == "*=" )
		return AddUnique;
	if ( s == "-=" )
		return RemoveAll;
	if ( s == "~=" )
		return ReplaceAll;
	
	return Unknown;
}

QStringList QMakeModel::Search::compute(const QProjectNode *n, const QString& variable,
							const QStringList& config, Depth d,
							const QProjectNode *until, Substitution sub)
{
	CacheKey k(n, variable);
	QStringList *c = m_cache.object(k);
	
	if ( c && c->count() && !until && (sub == SubstituteAll) )
		return *c;
	
	QStringList l;
	
	//qDebug("%s->computing(%s) {", qPrintable(n->name()), qPrintable(variable));
	compute(n, variable, l, config, d, n, until, sub);
	//qDebug("} => (%s)", qPrintable(l.join(", ")));
	
	if ( variable == "CONFIG" )
	{
		QString plat =
		#if defined(Q_WS_X11)
			"unix"
		#elif defined(Q_WS_MAC)
			"macx"
		#elif defined(Q_WS_WIN32)
			"win32"
		#else
			""
		#endif
			;
		
		if ( plat.count() && !l.contains(plat) )
			l << plat;
		
	}
	
	if ( !until && (sub == SubstituteAll) )
		m_cache.insert(k, new QStringList(l));
	
	return l;
}

QStringList QMakeModel::Search::substitute(const QString& s, const QProjectNode *n,
										Depth d, const QStringList& config,
										const QString& variable,
										const QProjectNode *from,
										const QProjectNode *until,
										Substitution sub)
{
	int dollar = s.indexOf("$$");
	
	if ( (dollar == -1) || !sub )
		return QStringList(s);
	
	//qDebug("substitution of : %s", qPrintable(s));
	
	QStringList tmp, l;
	QString v = s.mid(dollar);
	
	l << s.left(dollar);
	
	const QProjectNode *proj = from ? from : n;
	
	/*
	if ( d == Project )
	{
		proj = n->project();
	} else if ( d == Scope ) {
		proj = n->parent();
	} else if ( d == Whole ) {
		proj = n->project();
		
		while ( QProject *pp = proj->project() )
			proj = pp;
	}
	
	if ( !proj )
		proj = n;
	
	*/
	
	while ( (dollar = v.indexOf("$$")) != -1 )
	{
		if ( dollar )
			for ( int i = 0; i < l.count(); ++i )
				l[i] += v.left(dollar);
		
		v.remove(0, dollar + 2);
		
		int start = 0;
		const QChar c = v.at(start);
		const int paren = v.indexOf('(');
		
		if ( (c == '[') && (sub & SubstituteProperties) )
		{
			// QMake property
			++start;
			
			const int count = v.indexOf(']', start) - start;
			QString var = v.mid(start, count > 0 ? count : -1);
			
			//v.replace(dollar, var.count() + 4, QMake::property(var));
			QString prop = QMake::property(var);
			
			if ( prop.count() )
			{
				for ( int i = 0; i < l.count(); ++i )
					l[i] += prop;
			}
			
			if ( count >= 0 )
				v.remove(0, count + 2);
			else
				v.clear();
			
			//qDebug("QMake prop : %s => %s", qPrintable(var), qPrintable(v));
			
		} else if ( (c == '{') && (sub & SubstituteVariables) ) {
			// Project var
			++start;
			
			const int count = v.indexOf('}', start) - start;
			QString var = v.mid(start, count > 0 ? count : -1);
			
			//foreach ( QString val, resolve(var, cfg, f) )
			//	cache << prefix + val + suf;
			
			QStringList ext = compute(proj, var, config, d, until, sub);
			
			if ( ext.count() )
			{
				tmp.clear();
				
				foreach ( const QString& s, l )
					foreach ( const QString& p, ext )
						tmp << s + p;
				
				l = tmp;
				
				tmp.clear();
				//qDebug("project var : %s => %s", qPrintable(var), qPrintable(v));
			}
			
			if ( count >= 0 )
				v.remove(0, count + 2);
			else
				v.clear();
			
		} else if ( (c == '(') && (sub & SubstituteEnvironment) ) {
			// Env var
			++start;
			
			const int count = v.indexOf(')', start) - start;
			QString var = v.mid(start, count > 0 ? count : -1);
			
			//v.replace(dollar, var.count() + 4, QMake::property(var));
			QString env = qgetenv(var.toLocal8Bit());
			
			if ( env.count() )
				for ( int i = 0; i < l.count(); ++i )
					l[i] += env;
			
			if ( count >= 0 )
				v.remove(0, count + 2);
			else
				v.clear();
			
			//qDebug("Env var : %s => %s", qPrintable(var), qPrintable(v));
			
		} else if ( (paren != -1) && (sub & SubstituteFunctions) ) {
			// function : try some...
			int nest = 1,
				index = paren,
				count = paren - start,
				end = v.length(); //v.indexOf(')', paren);
			
			QStringList args;
			QString atmp, fct = v.mid(start, count);
			
			/*
			qDebug("trying to resolve function : %s (%s)",
					qPrintable(fct),
					qPrintable(v));
			*/
			
			while ( ++index < end )
			{
				QChar ch = v.at(index);
				
				if ( ch == '(' )
				{
					++nest;
				} else if ( ch == ')' ) {
					--nest;
					
					if ( !nest )
					{
						end = index;
						break;
					}
				} else if ( (ch == ',') && (nest == 1) ) {
					args << atmp;
					atmp.clear();
				} else {
					atmp += ch;
				}
			}
			
			if ( atmp.count() )
				args << atmp;
			
			//qDebug("fct : %s(%s)", qPrintable(fct), qPrintable(args.join(", ")));
			
			v.remove(0, end + 1);
			
			if ( fct == "join" )
			{
				//qDebug("join(%s)", qPrintable(args.join(",")));
				
				//qDebug("%s", qNodeName(proj));
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				//qDebug("%s = {%s}", qPrintable(vname), qPrintable(values.join(", ")));
				
				QString stick = args.at(2) + values.join(args.at(1)) + args.at(3);
				
				if ( stick.count() )
					for ( int i = 0; i < l.count(); ++i )
						l[i] += stick;
				
			} else if ( fct == "member" ) {
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				//qDebug("values : {%s}", qPrintable(values.join(", ")));
				
				QString str = values.isEmpty()
							?
								QString()
							:
								values.at(
									qBound(
										0,
										(
											(args.count() > 1)
										?
											args.at(1).toInt()
										:
											0
										),
										values.count() - 1
									)
								);
				
				if ( str.count() )
					for ( int i = 0; i < l.count(); ++i )
						l[i] += str;
				
			} else if ( fct == "find" ) {
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				QString pattern;
				
				if ( args.count() > 1 )
					pattern = args.at(1);
				
				if ( pattern.count() && values.count() )
				{
					QRegExp rx(pattern);
					
					tmp.clear();
					
					foreach ( const QString& s, l )
						foreach ( const QString& p, values )
							if ( rx.exactMatch(p) )
								tmp << s + p;
					
					l = tmp;
					
					tmp.clear();
				}
			} else if ( fct == "basename" ) {
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				//qDebug("values : {%s}", qPrintable(values.join(", ")));
				
				if ( values.count() )
				{
					tmp.clear();
					
					foreach ( const QString& s, l )
						foreach ( const QString& p, values )
							tmp << s + QFileInfo(p).baseName();
					
					l = tmp;
					
					tmp.clear();
				}
			} else if ( fct == "dirname" ) {
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				//qDebug("values : {%s}", qPrintable(values.join(", ")));
				
				if ( values.count() )
				{
					tmp.clear();
					
					foreach ( const QString& s, l )
						foreach ( const QString& p, values )
							tmp << s + QFileInfo(p).path();
					
					l = tmp;
					
					tmp.clear();
				}
			} else if ( fct == "unique" ) {
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				//qDebug("values : {%s}", qPrintable(values.join(", ")));
				
				tmp.clear();
				
				foreach ( const QString& val, values )
					if ( !tmp.contains(val) )
						tmp << val;
				
				values = tmp;
				
				if ( values.count() )
				{
					tmp.clear();
					
					foreach ( const QString& s, l )
						foreach ( const QString& p, values )
							tmp << s + p;
					
					l = tmp;
					
					tmp.clear();
				}
				
			} else if ( fct == "replace" ) {
				QString vname = substitute(
									args.at(0),
									n,
									d,
									config,
									variable,
									from,
									until,
									sub
								).at(0);
				
				QStringList values = compute(proj, vname, config, d, until, sub);
				
				QString ss, sr;
				
				if ( args.count() > 1 )
					ss = args.at(1);
				
				if ( args.count() > 2 )
					ss = args.at(2);
				
				if ( values.count() )
				{
					tmp.clear();
					
					foreach ( const QString& s, l )
						foreach ( QString p, values )
							tmp << s + p.replace(ss, sr);
					
					l = tmp;
					
					tmp.clear();
				}
			} else if ( fct == "quote" ) {
				
				for ( int i = 0; i < l.count(); ++i )
					l[i] += args.at(0);
				
				/*
				v.replace(	dollar,
							end - dollar + 1,
							QString("\"")
							+ substitute(args.at(0))
							+ '\"'
						);
				*/
			}
		} else if ( sub & SubstituteVariables ) {
			int len = 0;
			
			while (
						(len < v.length())
					&&
						(
							v.at(len).isLetterOrNumber()
						||
							(v.at(len) == '_')
						)
					)
				++len;
			
			QString var = v.left(len);
			v.remove(0, len);
			
			//foreach ( QString val, resolve(var, cfg, f) )
			//	cache << prefix + val;
			
			QStringList ext = compute(proj, var, config, d, until, sub);
			
			if ( ext.count() )
			{
				tmp.clear();
				
				foreach ( const QString& s, l )
					foreach ( const QString& p, ext )
						tmp << s + p;
				
				l = tmp;
				
				tmp.clear();
			}
			//qDebug("project var : %s => %s", qPrintable(var), qPrintable(v));
		} else {
			// put unsubstituted stuff in list...
			
			int end = 0;
			
			if ( paren != -1 )
			{
				int nest = 1,
					index = paren;
				
				end = v.length();
				
				while ( ++index < end )
				{
					QChar ch = v.at(index);
					
					if ( ch == '(' )
					{
						++nest;
					} else if ( ch == ')' ) {
						--nest;
						
						if ( !nest )
						{
							end = index;
							break;
						}
					}
				}
			} else if ( c == '{' ) {
				end = v.indexOf('}', start) - start;
			} else if ( c == '[' ) {
				end = v.indexOf(']', start) - start;
			} else if ( c == '(' ) {
				end = v.indexOf(')', start) - start;
			} else {
				
				while (
							(end < v.length())
						&&
							(
								v.at(end).isLetterOrNumber()
							||
								(v.at(end) == '_')
							)
						)
					++end;
				
			}
			
			QString s("$$");
			s += v.left(end);
			
			for ( int i = 0; i < l.count(); ++i )
				l[i] += s;
			
		}
	}
	
	if ( v.count() )
	{
		if ( l.isEmpty() )
			l << v;
		else
			for ( int i = 0; i < l.count(); ++i )
				l[i] += v;
	}
	
	//qDebug("%s = {%s}", qPrintable(s), qPrintable(l.join(" ,")));
	
	return l;
}

bool QMakeModel::Search::hasValue(const QProjectNode *n,
								const QString& var,
								const QString& op,
								const QString& val,
								Depth d,
								const QStringList& config,
								Substitution sub)
{
	const INode *nd = INode::fromNode(n);
	
	if ( nd && (nd->type == INode::Variable) )
	{
		int idx = nd->data.indexOf(' ');
		QString vname = nd->data.left(idx),
				voprt = nd->data.mid (idx + 1);
		
		if ( (var != vname) || (op != voprt) )
		{
			return false;
		}
		
		/*
		qDebug("%sSearch:{tg=%s, in=%x}",
				qPrintable(ident),
				qPrintable(variable),
				nd
			);
		*/
		
		foreach ( INode *value, nd->children )
		{
			if ( value->type != INode::Value )
				continue;
			
			QStringList resolved = substitute(
										value->data,
										n->parent(),
										d,
										config,
										var,
										0,
										n,
										sub
									);
			
			if ( resolved.contains(val) )
				return true;
		}
	} else {
		bool prevRecurse = false;
		QList<QProjectNode*> cnodes = n->children();
		
		foreach ( QProjectNode *c, cnodes )
		{
			bool recurse = false;
			INode *cn = INode::fromNode(c);
			
			if ( d == Whole )
			{
				recurse = true;
			} else if ( d == Project ) {
				recurse = cn->type != INode::Project;
			} else if ( d == File ) {
				recurse = c->type() != QProjectNode::Project;
			} else if ( d == Scope ) {
				recurse =
							(cn->type != INode::Scope)
						&&
							(c->type() != QProjectNode::Project);
				
			}
			
			if ( cn && recurse && cn && (cn->type == INode::Scope) )
			{
				QString sn = c->name();
				bool notScope = sn.at(0) == QLatin1Char('!');
				
				if ( sn  == "else" )
				{
					recurse = !prevRecurse;
				} else if ( !sn.contains('(') ) {
					recurse = config.contains(sn);
				} else {
					QMakeParser::TokenList tokens =
							QMakeParser::lex(
												sn.constData() + (notScope & 1),
												sn.length() - (notScope & 1)
											);
					
					if ( tokens.count() != 1 )
					{
						qWarning("%s : Inconsistent function name caused lexer error.",
								 __FUNCTION__);
						
						prevRecurse = false;
						tokens.cleanup();
						continue;
					}
					
					QMakeParser::Token *t = tokens.at(0);
					
					QString fct = t->text;
					QStringList arguments;
					
					// skip
					t = t->next;
					
					while ( t->next )
					{
						t = t->next;
						
						if ( t->text == QMakeTokens::functionCloseMark )
							break;
						
						if ( t->text != "," )
							arguments << t->text;
					}
					
					bool hasArgs = arguments.count();
					
					//qDebug("args: {%s}", qPrintable(arguments.join(", ")));
					
					if ( hasArgs && (fct == "CONFIG") )
					{
						recurse = config.contains(arguments.at(0));
						
					} else if ( (arguments.count() > 1) && (fct == "contains") ) {
						QString vn = arguments.at(0);
						
						// TODO : adjust search target according to depth...
						const QProjectNode *np = n;
						
						if (
								(
									(d == Project)
								||
									(d == Whole)
								)
							&&
								(n->type() != QProjectNode::Project)
							)
						{
							np = n->project();
							//qDebug("project : %s", qPrintable(np->name()));
						}
						
						QStringList values = compute(np, vn, config, d, c);
						
						recurse = values.contains(arguments.at(1));
						
//						qDebug("%s contains(%s, %s) { ... }",
//								recurse ? "stepping in" : "skipping",
//								qPrintable(vn),
//								qPrintable(arguments.at(1))
//								);
						
					} else {
						recurse = notScope;
					}
					
					tokens.cleanup();
				}
				
				if ( notScope )
					recurse = !recurse;
				
			}
			
			prevRecurse = recurse;
			
			if ( !recurse )
				continue;
			
			if ( hasValue(c, var, op, val, d, config, sub) )
				return true;
		}
	}
	
	return false;
}

// return true if "until" node reached (hence the ned to stop the search)
bool QMakeModel::Search::compute(const QProjectNode *n, const QString& variable,
					QStringList& back,
					const QStringList& config,
					Depth d,
					const QProjectNode *from,
					const QProjectNode *until,
					Substitution sub)
{
	if ( !n || variable.isEmpty() )
		return false;
	
	if ( n == until )
		return true;
	
	if ( variable == "PWD" )
	{
		QProjectNode *p = n->parent();
		
		while ( p )
		{
			if ( p->type() == QProjectNode::Project )
				break;
			
			const INode *pd = INode::fromNode(p);
			
			if ( pd->type == INode::Include )
				break;
			
			if ( p->parent() )
				p = p->parent();
			else
				break;
		}
		
		// p should never be NULL but keep this in case a model is malformed...
		if ( p )
		{
			//qDebug("PWD of : %s", qPrintable(p->name()));
			back << QFileInfo(p->name()).path();
		}
		
		return false;
	}
	
	static QString ident;
	
	ident += " ";
	
	const INode *nd = INode::fromNode(n);
	
	if ( nd && (nd->type == INode::Variable) )
	{
		int idx = nd->data.indexOf(' ');
		QString vname = nd->data.left(idx),
				voprt = nd->data.mid (idx + 1);
		
		if ( variable != vname )
		{
			//qWarning("skipped : \"%s\"", qPrintable(vname));
			ident.chop(1);
			return false;
		}
		
		Operation op = operation(voprt);
		
		if ( op == Set )
		{
			back.clear();
			op = AddAlways;
		} else if ( op == Unknown ) {
			//qWarning("Unknown operator : \"%s\"", qPrintable(voprt));
			ident.chop(1);
			return false;
		}
		
		foreach ( INode *value, nd->children )
		{
			if ( value->type != INode::Value )
				continue;
			
			QStringList resolved = substitute(
										value->data,
										n->parent(),
										d,
										config,
										variable,
										from,
										n, //until,
										sub
									);
			
			switch ( op )
			{
				case AddAlways :
					back << resolved;
					break;
					
				case AddUnique :
					foreach ( const QString& v, resolved )
						if ( !back.contains(v) )
							back << v;
					break;
					
				case RemoveAll :
					foreach ( const QString& v, resolved )
						back.removeAll(v);
					break;
					
				case ReplaceAll :
					//back.replaceInStrings(value->data);
					break;
					
				default:
					break;
			}
		}
		
		
		//qDebug("%s = %s",
		//		qPrintable(vname),
		//		qPrintable(back.join(" "))
		//	);
		
		//qDebug("=> {%s}", qPrintable());
		
	} else {
		bool prevRecurse = false;
		QList<QProjectNode*> cnodes = n->children();
		
		//foreach ( QProjectNode *c, cnodes )
		for ( int idx = 0; idx < cnodes.count(); ++idx )
		{
			QProjectNode *c = cnodes.at(idx);
			
			if ( c == until )
			{
				ident.chop(1);
				return true;
			}
			
			bool recurse = false;
			INode *cn = INode::fromNode(c);
			
			if (
					!cn
				||
					(cn->type == INode::Comment)
				)
				continue;
			
			if (
					(cn->type == INode::Variable)
				&&
					(c->name() != variable)
				)
			{
				//if ( variable != "DEPENDPATH" )
				//	qDebug("skipping var %s [%i]", qPrintable(c->name()), idx);
				
				continue;
			}
			
			if ( d == Whole )
			{
				recurse = true;
			} else if ( d == Project ) {
				recurse = cn->type != INode::Project;
			} else if ( d == File ) {
				recurse = c->type() != QProjectNode::Project;
			} else if ( d == Scope ) {
				recurse =
							(cn->type != INode::Scope)
						&&
							(cn->type != INode::Include)
						&&
							(cn->type != INode::Project);
				
			}
			
			if ( recurse && (cn->type == INode::Scope) )
			{
				QString sn = c->name();
				bool notScope = sn.at(0) == QLatin1Char('!');
				
				if ( sn  == "else" )
				{
					recurse = !prevRecurse;
				} else if ( !sn.contains('(') ) {
					recurse = config.contains(sn);
				} else {
					QMakeParser::TokenList tokens =
							QMakeParser::lex(
												sn.constData() + (notScope & 1),
												sn.length() - (notScope & 1)
											);
					
					if ( tokens.count() != 1 )
					{
						qWarning("%s : Inconsistent function name caused lexer error.",
								 __FUNCTION__);
						
						prevRecurse = false;
						tokens.cleanup();
						continue;
					}
					
					QMakeParser::Token *t = tokens.at(0);
					
					QString fct = t->text;
					QStringList arguments;
					
					// skip
					t = t->next;
					
					while ( t->next )
					{
						t = t->next;
						
						if ( t->text == QMakeTokens::functionCloseMark )
							break;
						
						if ( t->text != "," )
							arguments << t->text;
					}
					
					//qDebug("args: {%s}", qPrintable(arguments.join(", ")));
					
					bool hasArgs = arguments.count();
					
					if ( hasArgs && (fct == "CONFIG") )
					{
						recurse = config.contains(arguments.at(0));
						
					} else if ( (arguments.count() > 1) && (fct == "contains") ) {
						QString var = arguments.at(0);
						
						if ( var == variable )
						{
							recurse = back.contains(arguments.at(1));
						} else {
							// TODO : adjust search target according to depth...
							const QProjectNode *np = from ? from : n->parent();
							
							QStringList values = compute(np, var, config, d, c);
							
							recurse = values.contains(arguments.at(1));
								
//							qDebug("%s contains(%s, %s) { ... }",
//									recurse ? "stepping in" : "skipping",
//									qPrintable(var),
//									qPrintable(arguments.at(1))
//									);
							
						}
						
					} else {
						recurse = notScope;
					}
					
					tokens.cleanup();
				}
				
				if ( notScope )
					recurse = !recurse;
				
			}
			
			//if ( recurse && c->type() == QProjectNode::Folder )
			//	qDebug("entering scope %s [%i]", qPrintable(c->name()), idx);
			
			prevRecurse = recurse;
			
			if ( !recurse )
			{
				//qDebug("skipping %s", qPrintable(c->name()));
				continue;
			}
			
			if ( compute(c, variable, back, config, d, from, until, sub) )
			{
				ident.chop(1);
				return true;
			}
		}
	}
	
	ident.chop(1);
	
	return false;
}

QMakeModel::INode* QMakeModel::INodeBackingStore::getFileNode(INode::DataType t,
												const QString& fn, bool *shared)
{
	foreach ( INode *n, m_files )
	{
		if ( n->data == fn )
		{
			if ( shared )
				*shared = m_mappings.value(n).count();
			
			n->parent = 0;
			return n;
		}
	}
	
	INode *n = new INode(t, fn);
	m_files << n;
	
	if ( shared )
		*shared = false;
	
	return n;
}

void QMakeModel::INodeBackingStore::hook(QProjectNode *p, INode *n)
{
	m_mappings[n] << p;
}

void QMakeModel::INodeBackingStore::unhook(QProjectNode *p, INode *n)
{
	m_mappings[n].removeAll(p);
	
	if ( m_mappings.value(n).isEmpty() )
	{
		m_mappings.remove(n);
		m_files.removeAll(n);
		
		foreach ( INode *ip, m_bucket )
			ip->children.removeAll(n);
		
		delete n;
	}
}

void QMakeModel::INodeBackingStore::appendChild(INode *n, INode *c,
												INode::Provider provider,
												bool dup)
{
	if ( !n || !c || n->children.contains(c) || !m_mappings.contains(n) )
		return;
	
	if ( c->parent )
	{
		removeChild(c->parent, c);
	}
	
	c->parent = n;
	n->children.append(c);
	n->state |= INode::Modified;
	
	bool flat = n->type == INode::Variable && (displayMode & FlatVariables);
	
	QString nn = n->data.left(n->data.indexOf(' '));
	
	if ( flat )
	{
		flat = QMakeTokens::displayedVariables.contains(nn);
		
		//if ( flat )
		//	qDebug("flatty add : %s", qPrintable(nn));
	}
	
	QList<QProjectNode*> map = m_mappings[n];
	
	//qDebug("appending %s to %s", qPrintable(c->data), qPrintable(n->data));
	
	foreach ( QProjectNode *pn, map )
	{
		QProjectNode* cpn = provider(c, dup);
		
		if ( flat && pn->model() && pn->parent() )
			cpn->attach(pn, pn->parent());
		else
			cpn->attach(pn);
		
		//qDebug("=> attached.");
	}
	
	if ( n->type == INode::Variable )
	{
		Search::invalidateCache(n->parent, nn);
	}
	
	if ( c->type == INode::Variable )
	{
		Search::invalidateCache(n, c->data.left(c->data.indexOf(' ')));
	}
}

void QMakeModel::INodeBackingStore::removeChild(INode *n, INode *c, QProjectNode *m)
{
	if ( !n || !c || !n->children.contains(c) || !m_mappings.contains(n) )
		return;
	
	n->children.removeAll(c);
	n->state |= INode::Modified;
	
	if ( n->type == INode::Variable )
	{
		Search::invalidateCache(n->parent, n->data.left(n->data.indexOf(' ')));
	}
	
	if ( c->type == INode::Variable )
	{
		Search::invalidateCache(n, c->data.left(n->data.indexOf(' ')));
	}
	
	QList<QProjectNode*> map = m_mappings[n];
	
	foreach ( QProjectNode *pn, map )
	{
// 		if ( pn == m )
// 			continue;
		
		QList<QProjectNode*> children = pn->children();
		
		foreach ( QProjectNode *pc, children )
		{
			INode *cc = INode::fromNode(pc);
			
			if ( !cc )
			{
				qWarning(	"Invalid child removal attempt from INode mappings"
							" in %s [%s:%i]",
							__FUNCTION__, __FILE__, __LINE__);
				
			} else if ( (cc == c) && (pc != m) ) {
				pc->detach();
				delete pc;
				
				// shouldn't be more than one... might crash besides
				break;
			}
		}
	}
}

QList<QProjectNode*> QMakeModel::INodeBackingStore::mappings(INode *n) const
{
	return m_mappings.value(n);
}

QProjectNode* QMakeModel::INodeBackingStore::mappingClone(INode *n) const
{
	QHash<INode*, QList<QProjectNode*> >::const_iterator it;
	it = m_mappings.constFind(n);
	
	if ( it == m_mappings.constEnd() )
		return 0;
	
	return it->at(0)->clone();
}

QMakeModel::INodeBackingStore* QMakeModel::INodeBackingStore::instance()
{
	static INodeBackingStore m_instance;
	
	return &m_instance;
}

void QMakeModel::Search::invalidateCache(INode *n, const QString& var)
{
	QList<QProjectNode*> nodes = INodeBackingStore::instance()->mappings(n);
	
	foreach ( const QProjectNode *map, nodes )
	{
		CacheKey k(map, var);
		
		m_cache.remove(k);
	}
}

QIcon QMakeModel::icon(int idx)
{
	static QIcon m_icons[] = {
		// none
		QIcon(),
		// comment
		QIcon(":/comment.png"),
		// project
		QIcon(":/project.png"),
		// include
		QIcon(":/project.png"),
		// scope
		QIcon(":/folder.png"),
		// function
		QIcon(":/function.png"),
		// variable
		QIcon(":/variable.png"),
		// value
		QIcon(":/value.png"),
		QIcon(":/h.png"),
		QIcon(":/cpp.png"),
		QIcon(":/form.png"),
		QIcon(":/qrc.png"),
		QIcon(":/ts.png"),
		QIcon(":/text.png")
	};
	
	static int max = sizeof(m_icons) / sizeof(QIcon);
	
	return (idx >= 0) && (idx < max) ? m_icons[idx] : QIcon();
}

void QMakeModel::duplicate(QProjectNode *n, INode *d)
{
	if ( !d || !n )
		return;
	
	//qDebug("duplicating node : %s", qPrintable(d->data));
	
	foreach ( INode *c, d->children )
	{
		QProjectNode *child = INodeBackingStore::instance()->mappingClone(c);
		
		if ( !child )
			continue;
		
		duplicate(child, c);
		
		child->attach(n);
	}
	
	//qDebug("duplicated node : %s", qPrintable(d->data));
}

QMakeModel::Node::Node(NodeType t, INode *n, bool dup)
 : QProjectNode(t), d(n)
{
	INodeBackingStore::instance()->hook(this, d);
	
	if ( dup && d )
	{
		//qDebug("Instanciating shared node : %s", qPrintable(d->data));
		
		duplicate(this, n);
		
		//qDebug("Instanciated shared node : %s", qPrintable(d->data));
	}
}

QMakeModel::Node::~Node()
{
	INodeBackingStore::instance()->unhook(this, d);
}

QProjectNode* QMakeModel::Node::clone() const
{
	return new Node(type(), d);
}

QVariant QMakeModel::Node::data(int role) const
{
	if ( !d ) return QVariant();
	
	if ( role == Qt::ToolTipRole )
	{
		QProject *p = project();
		
		if ( p && (type() == File) && (d->type == INode::Value) )
			return p->absoluteFilePath(d->data);
	} else if ( role == Qt::DecorationRole ) {
		
		int idx = d->type;
		
		if ( d->type == INode::Value )
		{
			idx += QMakeTokens::displayedVariables.indexOf(QProjectNode::parent()->name()) + 1;
		}
		
		return icon(idx);
	} else if ( role == QProjectModel::DetailLevelRole ) {
		if ( d )
		{
			switch ( d->type )
			{
				case INode::Variable :
					if (
						!
							(
								displayMode & ShowAllVariables
							||
								QMakeTokens::displayedVariables.contains(name())
							)
						)
						return 1;
					
				case INode::Value :
				case INode::Project :
				case INode::Include :
				case INode::Scope :
					return 0;
					
				case INode::Comment :
					return (displayMode &  ShowComments) ? 0 : 1;
					
				case INode::Function :
					return (displayMode &  ShowFunctions) ? 0 : 1;
					
				default:
					return 1;
			}
		} else {
			return -1;
		}
	}
	
	return QProjectNode::data(role);
}

bool QMakeModel::Node::setData(const QVariant& v, int role)
{
	if ( d && (role == Qt::EditRole) && (v != data(Qt::EditRole)) )
	{
		int t = d->type;
		QProject *p = project();
		QString vs = v.toString();
		bool isAbs = QFileInfo(vs).isAbsolute();
		
		switch ( t )
		{
			case INode::Variable :
				d->data = vs + d->data.mid(d->data.indexOf(' '));
				break;
				
			case INode::Value :
			{
				QString oldfn = p->absoluteFilePath(name()),
						newfn = isAbs ? vs : p->absoluteFilePath(vs);
				
				bool oldExists = QFile::exists(oldfn),
					newExists = QFile::exists(newfn);
				
				int row = 0;
				QDialog dlg;
				QGridLayout *l = new QGridLayout(&dlg);
				QCheckBox *eraseSource = 0, *overwriteDest = 0;
				
				l->addWidget(new QLabel(QMakeParser::tr("Renaming : ")), row, 0);
				l->addWidget(new QLabel(oldfn), row, 1);
				++row;
				
				l->addWidget(new QLabel(QMakeParser::tr("to : ")), row, 0);
				l->addWidget(new QLabel(newfn), row, 1);
				++row;
				
				if ( oldExists )
				{
					eraseSource = new QCheckBox(QMakeParser::tr("Erase source file"),
												&dlg);
					
					l->addWidget(eraseSource, row, 0, 1, 2);
					
					++row;
					
					if ( newExists )
					{
						overwriteDest = new QCheckBox(
											QMakeParser::tr("Overwrite destination file"),
											&dlg
										);
						
						l->addWidget(overwriteDest, row, 0, 1, 2);
						
						++row;
					}
				} else {
					
				}
				
				int ret = QDialog::Accepted;
				
				if ( row > 1 )
				{
					QDialogButtonBox *box = new QDialogButtonBox(&dlg);
					box->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
					
					dlg.connect(box, SIGNAL( rejected() ), SLOT  ( reject() ) );
					dlg.connect(box, SIGNAL( accepted() ), SLOT  ( accept() ) );
					
					l->addWidget(box, row, 0, 1, 2);
					
					ret = dlg.exec();
				}
				
				if ( ret == QDialog::Rejected )
					return false;
				
				if ( overwriteDest && overwriteDest->isChecked() )
				{
					QFile::remove(newfn);
				}
				
				if ( eraseSource )
				{
					if ( eraseSource->isChecked() )
						QFile::rename(oldfn, newfn);
					else
						QFile::copy(oldfn, newfn);
				}
				
				d->data = isAbs ? p->relativeFilePath(vs) : vs;
				break;
			}
				
			default:
				d->data = vs;
				break;
		}
		
		if ( p )
			p->setModified(true);
		
		return true;
	}
	
	return false;
}

QString QMakeModel::Node::name() const
{
	if ( !d ) return QString();
	
	if ( d->type == INode::Variable )
		return d->data.left(d->data.indexOf(' '));
	
	if ( d->type == INode::Include )
		return QFileInfo(d->data).fileName();
	
	return d->data;
}

int QMakeModel::Node::row() const
{
	QProjectNode *p = QProjectNode::parent();
	
	if ( d && (d->type == INode::Value) && (displayMode & FlatVariables) && QMakeTokens::displayedVariables.contains(p->name()) )
	{
		int pr = p->row(), vr = QProjectNode::row();
		
		qDebug("pr = %i, vr = %i [%s]", pr, vr, qPrintable(name()));
		
		return pr + vr;
	}
	
	return QProjectNode::row();
}

QProjectNode* QMakeModel::Node::parent() const
{
	QProjectNode *p = QProjectNode::parent();
	
	if ( d && (d->type == INode::Value) && (displayMode & FlatVariables) && QMakeTokens::displayedVariables.contains(p->name()) )
		return p->parent();
	
	return p;
}

bool QMakeModel::Node::isFragmented() const
{
	if ( !d )
		return false;
	
	if ( (d->type == INode::Variable) && (displayMode & FlatVariables) )
		return QMakeTokens::displayedVariables.contains(name());
	
	return false;
}

int QMakeModel::Node::rowSpan() const
{
	if ( !d ) return 0;
	
	//if ( model() && model()->isDetailed() )
	//	return QProjectNode::rowSpan();
	
	switch ( d->type )
	{
		case INode::Variable :
			//qDebug("heck if I guess who called me...");
			//return 1;
			/*
			if (
				!
					(
						displayMode & ShowAllVariables
					||
						QMakeTokens::displayedVariables.contains(name())
					)
				)
				return 0;
			*/
			if ( (displayMode & FlatVariables) && QMakeTokens::displayedVariables.contains(name()) )
				return QProjectNode::rowCount();
			
		case INode::Project :
		case INode::Include :
		case INode::Value :
		case INode::Scope :
		case INode::Comment :
		case INode::Function :
			return QProjectNode::rowSpan();
			
			/*
		case INode::Comment :
			return (displayMode &  ShowComments) ? QProjectNode::rowSpan() : 0;
			
		case INode::Function :
			return (displayMode &  ShowFunctions) ? QProjectNode::rowSpan() : 0;
			*/
			
		default:
			return 0;
	}
	
	return QProjectNode::rowSpan();
}

int QMakeModel::Node::rowCount() const
{
	if ( !d ) return 0;
	
	//if ( model() && model()->isDetailed() )
	//	return QProjectNode::rowCount();
	
	switch ( d->type )
	{
		case INode::Variable :
			if ( (displayMode & FlatVariables) && QMakeTokens::displayedVariables.contains(name()) )
				return 0;
			
		case INode::Project :
		case INode::Include :
		case INode::Scope :
			return QProjectNode::rowCount();
			
		default:
			return 0;
	}
	
	return QProjectNode::rowCount();
}

QProjectNode::DefaultActions QMakeModel::Node::defaultActions() const
{
	if ( !d )
		return QProjectNode::defaultActions();
	
	switch ( d->type )
	{
		case INode::Variable :
			if ( QMakeTokens::displayedVariables.contains(name()) )
				return QProjectNode::defaultActions();
			
			return Remove | Rename;
			
		default:
			break;
	}
	
	return QProjectNode::defaultActions();
}

QProjectNode::ActionList QMakeModel::Node::actions() const
{
	ActionList l = QProjectNode::actions();
	
	switch ( d->type )
	{
		case INode::Value :
			if ( QProjectNode::parent()->name() == "FORMS" )
			{
				l.insert(1, Action(QIcon(":/preview.png"), DefaultPlugin::tr("Preview form")));
				l.insert(2, Action(QIcon(":/subclass.png"), DefaultPlugin::tr("Subclass form")));
			}
			
			break;
			
		default:
			break;
	}
	
	return l;
}

void QMakeModel::Node::actionTriggered(const QString& label)
{
	QProject *p = project();
	QString filename = name();
	QString absfilename = p ? p->absoluteFilePath(filename) : filename;
	
	if ( label == DefaultPlugin::tr("Preview form") )
	{
		QUiLoader loader;
		QFile file(absfilename);
		file.open(QFile::ReadOnly);
		QWidget *preview = loader.load(&file, 0);
		file.close();
		
		preview->setAttribute(Qt::WA_DeleteOnClose, true);
		preview->show();
	} else if ( label == DefaultPlugin::tr("Subclass form") ) {
		UiSubclass subclass(absfilename);
		
		subclass.exec();
		
		QProjectNode *p = QProjectNode::parent();
		p = p ? p->QProjectNode::parent() : 0;
		
		if ( !p )
			return;
		
		QStringList fl = subclass.createdFiles();
		
		foreach ( const QString& f, fl )
		{
			//qDebug("adding file %s", qPrintable(f));
			p->addFile(f);
		}
	} else {
		QProjectNode::actionTriggered(label);
	}
}

void QMakeModel::Node::addFile(const QString& file)
{
	if ( d )
		d->addFile(this, file);
	
	QProjectNode::addFile(file);
}

void QMakeModel::Node::addFolder(const QString& folder)
{
	if ( !d )
		return;
	
	d->addFolder(folder);
	
	if ( model() && children().count() )
		model()->edit(children().last());
}

void QMakeModel::Node::removeChild(QProjectNode *n)
{
	INode *nd = INode::fromNode(n);
	
	if ( d && nd )
	{
		INodeBackingStore::instance()->removeChild(d, nd, n);
	}
	
	QProjectNode::removeChild(n);
}

void QMakeModel::Node::write(QTextStream& out, int indent) const
{
	if ( d )
		d->write(out, indent, project());
}

QMakeModel::Project::Project(INode *n, bool dup)
 : QProject(), d(n)
{
	//qDebug("Instanciated project : %s", qPrintable(d->data));
	
	INodeBackingStore::instance()->hook(this, d);
	
	if ( dup && d )
		duplicate(this, n);
	
}

QMakeModel::Project::~Project()
{
	//qDebug("Deleted project : %s", qPrintable(d->data));
	
	INodeBackingStore::instance()->unhook(this, d);
}

QProjectNode* QMakeModel::Project::clone() const
{
	return new Project(d);
}

QProjectNode* QMakeModel::Project::parent() const
{
	QProjectNode *p = QProject::parent();
	
	if ( p && (displayMode & FlatVariables) )
	{
		return p->parent();
	}
	
	return p;
}

int QMakeModel::Project::row() const
{
	QProjectNode *p = QProject::parent();
	
	if ( p && (displayMode & FlatVariables) )
		return p->row() + QProject::row();
	
	return QProject::row();
}

QVariant QMakeModel::Project::data(int role) const
{
	if ( !d ) return QVariant();
	
	if ( role == Qt::DecorationRole )
	{
		return icon(INode::Project);
	}
	
	return QProject::data(role);
}

QString QMakeModel::Project::name() const
{
	if ( !d ) return QString();
	
	return d->data;
}

QString QMakeModel::Project::backend() const
{
	return "qmake project";
}

QString QMakeModel::Project::absoluteFilePath(const QString& fn) const
{
	QString dpfn, dfn = QProject::absoluteFilePath(fn);
	
	if ( QFile::exists(dfn) )
		return dfn;
	
	QStringList defaultCfg = Search::compute(
												this,
												"CONFIG",
												QStringList(),
												Search::Project
											),
				dependPath = Search::compute(
												this,
												"DEPENDPATH",
												defaultCfg,
												Search::Project
											);
	
	//qDebug("depend : \n\t%s", qPrintable(dependPath.join("\n\t")));
	
	foreach ( const QString& p, dependPath )
	{
		dpfn = QProject::absoluteFilePath(p + QDir::separator() + fn);
		
		if ( QFile::exists(dpfn) )
			return dpfn;
	}
	
	return dfn;
}

void QMakeModel::Project::addFile(const QString& file)
{
	if ( d )
		d->addFile(this, file);
	
	QProject::addFile(file);
}

void QMakeModel::Project::addFolder(const QString& folder)
{
	if ( !d )
		return;
	
	d->addFolder(folder);
	
	if ( model() && children().count() )
		model()->edit(children().last());
}

void QMakeModel::Project::removeChild(QProjectNode *n)
{
	INode *nd = INode::fromNode(n);
	
	if ( d && nd )
	{
		INodeBackingStore::instance()->removeChild(d, nd, n);
	}
	
	QProject::removeChild(n);
}

void QMakeModel::Project::write(QTextStream& out, int indent) const
{
	Q_UNUSED(out)
	Q_UNUSED(indent)
}

void QMakeModel::Project::save()
{
	QProject::save();
	
	QFile file(name());
	
	if ( !file.open(QFile::WriteOnly | QFile::Text) )
	{
		qWarning("Failed to open file %s for writing", qPrintable(name()));
		return;
	}
	
	QTextStream out(&file);
	
	foreach ( INode *c, d->children )
		if ( c )
			c->write(out, 0, this);
	
}

void QMakeModel::Project::settings()
{
	projectSettings()->setProject(this);
	
	if ( !projectSettings()->isVisible() )
		projectSettings()->exec();
}

QString QMakeModel::Project::query(const QString& s) const
{
	static QStringList propToVar = QStringList()
		<< "NAME"
		<< "AUTHOR"
		<< "LICENSE"
		<< "COPYRIGHT"
		;
	
	QString ans = QProject::query(s);
	
	if ( s == "TARGET_TYPE" )
	{
		return "binary";
	} else if ( s.startsWith("TARGET_PATH") ) {
		QStringList targets, cfg;
		QString mode = s.mid(11),
				mkfile = QFileInfo(name()).path() + "/Makefile";
		
		if ( mode == "_DEBUG" )
		{
			cfg << "debug";
		} else if ( mode == "_RELEASE" ) {
			cfg << "release";
		} else {
			cfg << "debug" << "release";
		}
		
		//Makefile::targets(mkfile, targets, cfg);
		
		return targets.join(",");
	} else if ( s == "LANGUAGE" ) {
		// TODO: check for other language...
		return "C++";
	} else if ( propToVar.contains(s) ) {
		QStringList l = Search::compute(this, "APP_" + s, QStringList(), Search::Project);
		
		if ( l.count() )
			return l.at(0);
		
	} else if ( ans.isEmpty() ) { //
		QStringList l = Search::compute(this, s, QStringList(), Search::Project);
		
		if ( l.count() )
			return l.at(0);
	}
	
	return ans;
}

QStringList QMakeModel::Project::files(ComputationMode m) const
{
	QStringList l = QProject::files(m);
	
	Search::Depth d = m ? Search::Whole : Search::Project;
	
	QStringList cfg = Search::compute(this, "CONFIG", QStringList(), d);
	QStringList uidir = Search::compute(this, "UI_DIR", cfg, d);
	
//	qDebug("%s[ui dir] = %s", qPrintable(name()), qPrintable(uidir.join(":")));
	
	uidir.prepend(QString());
	
	QStringList dest = l;
	
	foreach ( QString f, l )
	{
		QFileInfo info(f);
		
		if ( info.suffix() != "ui" )
			continue;
		
		f = "ui_" + info.baseName() + ".h";
		
		foreach ( const QString& d, uidir )
		{
			QString fn = d.count() ? d + QDir::separator() + f : f;
			fn = absoluteFilePath(fn);
			
			if ( !QFile::exists(fn) )
				continue;
			
			dest << fn;
			break;
		}
	}
	
	return dest;
}

void QMakeModel::Project::setModified(bool m)
{
	QProject::setModified(m);
	
	if ( d && !m )
		d->setClean();
}

QProject* QMakeModel::getProjectNode(const QString& fn)
{
	bool shared = false;
	
	INode *n = INodeBackingStore::instance()->getFileNode(INode::Project, fn, &shared);
	
	QProject *project = new Project(n, shared);
	
	if ( shared )
	{
		// must duplicate infrastructure...
		//duplicate(project, n);
	} else {
		//qDebug("parsing...");
		
		QMakeParser::parse(project, n, fn);
	}
	
	return project;
}

QMakeModel::INode* QMakeModel::INode::fromNode(QProjectNode *pn)
{
	if ( pn->type() == QProjectNode::Project )
	{
		QMakeModel::Project *p = dynamic_cast<QMakeModel::Project*>(pn);
		return p ? p->d : 0;
	}
	
	Node *n = dynamic_cast<Node*>(pn);
	
	return n ? n->d : 0;
}

const QMakeModel::INode* QMakeModel::INode::fromNode(const QProjectNode *pn)
{
	if ( pn->type() == QProjectNode::Project )
	{
		const QMakeModel::Project *p = dynamic_cast<const QMakeModel::Project*>(pn);
		return p ? p->d : 0;
	}
	
	const Node *n = dynamic_cast<const Node*>(pn);
	
	return n ? n->d : 0;
}

void QMakeModel::INode::addFile(QProjectNode *n, const QString& file)
{
	if ( !QFile::exists(file) )
		return;
	
	QProject *p = dynamic_cast<QProject*>(n);
	
	if ( !p )
		p = n->project();
	
	if ( type == Variable )
	{
		QString v = data.left(data.indexOf(' '));
		bool subdirs = v == QLatin1String("SUBDIRS");
		
		QProjectNode::NodeType nt = QProjectNode::Other;
		
		if ( subdirs )
			nt = QProjectNode::Project;
		else if ( QMakeTokens::displayedVariables.contains(v) )
			nt = QProjectNode::File;
		
		setNextNodeType(nt);
		
		if ( subdirs )
		{
			//qDebug("subdir : %s", qPrintable(fn));
			
			bool shared = false;
			
			INode *sub = INodeBackingStore::instance()
							->getFileNode(INode::Project, file, &shared);
			
			QList<QProject*> before = p->subProjects();
			
			INodeBackingStore::instance()->appendChild(	this,
														sub,
														ProjectProvider
														);
			
			QList<QProject*> after = p->subProjects();
			
			foreach ( QProject *sp, before )
				after.removeAll(sp);
			
			if ( !shared )
			{
				if ( after.count() != 1 )
					qWarning("quirk...");
				
				QMakeParser::parse(after.at(0), sub, file);
				//parse(after.at(0), sub, fn);
			}
		} else {
			INode *n = new INode(INode::Value, p->relativeFilePath(file));
			
			INodeBackingStore::instance()->appendChild(this, n, NodeProvider);
		}
	} else if ( (type == Scope) || (type == Project) || (type == Include) ) {
		if ( file.endsWith(".pri") )
		{
			// add include...
			bool shared = false;
			setNextNodeType(QProjectNode::Folder);
			
			INode *n = INodeBackingStore::instance()->
					getFileNode(INode::Include, file, &shared);
			
			INodeBackingStore::instance()->appendChild(this, n, NodeProvider, shared);
			//scope = n;
			
			if ( !shared )
			{
				QMakeParser::parse(p, n, file);
			}
		} else {
			QString var = QMake::variableForFile(file);
			
			foreach ( INode *c, children )
			{
				int idx = c->data.indexOf(' ');
				
				if (
						(idx != -1)
					&&
						(c->type == Variable)
					&&
						(c->data.left(idx) == var)
					&&
						(c->data.mid(idx + 1) != "-=")
					&&
						(c->data.mid(idx + 1) != "~=")
					)
				{
					c->addFile(n, file);
					return;
				}
			}
			
			setNextNodeType(QProjectNode::Folder);
			
			INode *v = new INode(INode::Variable, var + " " + "+=");
			INodeBackingStore::instance()->appendChild(this, v, NodeProvider);
			
			v->addFile(n, file);
		}
	}
}

void QMakeModel::INode::addFolder(const QString& f)
{
	setNextNodeType(QProjectNode::Folder);
	INode *s = new INode(INode::Scope, f);
	INodeBackingStore::instance()->appendChild(this, s, NodeProvider);
}
