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

#include "gnumakebuilder.h"

#include "edyuk.h"
#include "plugin.h"

#include "qmakebackend.h"

#include <QDir>
#include <QFile>
#include <QDebug>
#include <QLibrary>
#include <QFileInfo>
#include <QByteArray>
#include <QInputDialog>

#ifdef Q_WS_WIN
#define GNU_MAKE_DEFAULT "mingw32-make"
#else
#define GNU_MAKE_DEFAULT "make"
#endif

class GppParser : public QBuilder::CommandParser
{
	public:
		virtual QStringList parse(const QString& outputLine, QBuilder::ParsedLine& line)
		{
			QStringList modifiedFiles;
			
			line.line = 0;
			line.column = 0;
			line.file.clear();
			line.message.clear();
			
			static QString path;
			static QRegExp
				special("\\[([^]:]+)(?::([^]:]+))*\\]"),
				enterleave("((?:\\w:)?(?:[/\\\\][^/\\\\]+)+[/\\\\]?).$");
			
			QMap<QString, QRegExp> pattern;
			
			pattern[DefaultPlugin::tr("compiling %1[%1]")]
				= QRegExp("^g(?:cc|\\+\\+) -c .+ -o .+\\.o (.+\\.(?:c|cxx|cc|cpp))");
			
			pattern[DefaultPlugin::tr("linking %1[]")]
				= QRegExp("^g(?:cc|\\+\\+) .+ -o (\\S+)(?: .+\\.o)+");
			
			pattern[DefaultPlugin::tr("moc'ing %1[%1]")]
				= QRegExp("^(?:(?:\\w:)?/(?:[^/]+/)*)?moc (?:-\\S+ )*(\\S+)");
			
			pattern[DefaultPlugin::tr("processing form %1[%1]")]
				= QRegExp("^(?:(?:\\w:)?/(?:[^/]+/)*)?uic (\\S+)( -\\w+(?: (?:\\S+)?))*");
			
			pattern[DefaultPlugin::tr("processing resource %1[%1]")]
				= QRegExp("^(?:(?:\\w:)?/(?:[^/]+/)*)?rcc (?:-name \\S+ )(\\S+)");
			
			pattern["$enterleave"]
				= QRegExp(QString("^%1\\[\\d+\\]:").arg(GNU_MAKE_DEFAULT));
			
			pattern["%3[%1:%2]"]
				= QRegExp("^([^\n:\\[\\]]+):(\\d+):(.+)");
			
			QMap<QString, QRegExp>::iterator it = pattern.begin();
			
			QString data = outputLine.trimmed();
			
			while ( it != pattern.end() )
			{
				if ( data.contains(*it) )
				{
					line.message = it.key();
					QStringList caps = it->capturedTexts();
					
					if ( (line.message == "$enterleave") && data.contains(enterleave) )
					{
						QString oldpath = path;
						path = enterleave.cap(1).trimmed();
						
						//qDebug("path : %s", qPrintable(path));
						
						if ( QDir(path).exists() && (path != oldpath) )
						{
							line.message = DefaultPlugin::tr("Moving to %1").arg(path);
						} else {
							line.message.clear();
							path = oldpath;
						}
					} else if ( line.message.startsWith('$') ) {
						line.message.clear();
					} else {
						for ( int i = 1; i < caps.count(); ++i )
						{
							line.message.replace(QString::number(i).prepend("%"), caps.at(i));
							
							if ( caps.at(i).startsWith(" -o ") )
							{
								QString out = caps.at(i).mid(4);
								
								if ( out.count() && path.count() && !QFileInfo(out).isAbsolute() )
									out = QDir(path).absoluteFilePath(out);
								
								if ( out.count() )
									modifiedFiles << out;
							}
						}
						
						if ( line.message.contains(special) )
						{
							caps = special.capturedTexts();
							
							line.file = caps.at(1);
							
							// beware gcc line numbers are starting from 1 not 0 as QCodeEdit expects
							if ( caps.count() > 2 )
								line.line = caps.at(2).toInt() - 1;
							else
								line.line = -1;
							
							if ( caps.count() > 3 )
								line.column = caps.at(3).toInt();
							else
								line.column = -1;
							
							line.message.remove(caps.at(0));
						}
						
						if ( line.file.count() && path.count() && !QFileInfo(line.file).isAbsolute() )
							line.file = QDir(path).absoluteFilePath(line.file);
						
						modifiedFiles << line.file;
						
						//qDebug("{%s,%s}=>%s", qPrintable(line.message), qPrintable(path), qPrintable(line.file));
					}
					
					break;
				}
				
				++it;
			}
			
			return modifiedFiles;
		}
};

static GppParser _commonParser;

class CompileCommand : public QBuilder::Command
{
	public:
	virtual ~CompileCommand() {}
	
	virtual QIcon icon() const
	{
		return QIcon(":/compile.png");
	}
	
	virtual QString label() const
	{
		return DefaultPlugin::tr("&Compile");
	}
	
	virtual bool mayAffectTargetList() const
	{
		return true;
	}
	
	virtual Info info(const QString& in, const QString& mode) const
	{
		Info info;
		info.exec = DefaultPlugin::configKey<QString>("QBuilder/GnuMakeBuilder/make", GNU_MAKE_DEFAULT);
		
		if ( mode.count() && mode != "default" )
			info.arguments << mode;
		
		QList<QStringList> tl;
		tl << QStringList(QString());
		Makefile::targets(in, tl);
		
		//qDebug() << tl;
		
		foreach ( QStringList l, tl )
		{
			if ( l.count() <= 1 )
				continue;
			
			//qDebug("%s == %s?", qPrintable(mode), qPrintable(l.at(0)));
			
			if ( (l.at(0) == mode) || (mode == "default" && l.at(0).isEmpty()) )
			{
				info.output = l.at(1);
				//qDebug("target : %s", qPrintable(info.output));
				break;
			}
		}
		
		return info;
	}
	
	virtual QList<QBuilder::Command*> depends() const
	{
		return QList<QBuilder::Command*>();
	}
	
	virtual QBuilder::CommandParser* outputParser() const
	{
		return &_commonParser;
	}
};

class RunCommand : public QBuilder::Command
{
	public:
	virtual ~RunCommand() {}
	
	virtual QIcon icon() const
	{
		return QIcon(":/run.png");
	}
	
	virtual QString label() const
	{
		return DefaultPlugin::tr("&Execute");
	}
	
	virtual bool isStandalone() const
	{
		return true;
	}
	
	virtual Info info(const QString& in, const QString& mode) const
	{
		Info info;
		info.exec = in;
		
		if ( in.isEmpty() || !DefaultPlugin::configKey<bool>("QBuilder/GnuMakeBuilder/args", true) )
			return info;
		
		QString a = QInputDialog::getText(0, DefaultPlugin::tr("Enter arguments"), DefaultPlugin::tr("Arguments :"));
		
		info.arguments = Edyuk::splitArguments(a);
		
		return info;
	}
	
	virtual QList<QBuilder::Command*> depends() const
	{
		return QList<QBuilder::Command*>();
	}
	
	virtual QBuilder::CommandParser* outputParser() const
	{
		return &_commonParser;
	}
};

class CleanCommand : public QBuilder::Command
{
	public:
	virtual ~CleanCommand() {}
	
	virtual QIcon icon() const
	{
		// use invalid pixmap to hide from toolbar. Remove this if we find an icon...
		//return QIcon(":/clean.png");
		return QIcon();
	}
	
	virtual QString label() const
	{
		return DefaultPlugin::tr("Clea&n");
	}
	
	virtual bool mayAffectTargetList() const
	{
		return true;
	}
	
	virtual Info info(const QString& in, const QString& mode) const
	{
		Info info;
		info.exec = DefaultPlugin::configKey<QString>("QBuilder/GnuMakeBuilder/make", GNU_MAKE_DEFAULT);
		
		info.arguments << "distclean";
		info.output = "$source$";
		
		return info;
	}
	
	virtual QList<QBuilder::Command*> depends() const
	{
		return QList<QBuilder::Command*>();
	}
	
	virtual QBuilder::CommandParser* outputParser() const
	{
		return &_commonParser;
	}
};

class CompileRunCommand : public QBuilder::Command
{
	public:
	virtual ~CompileRunCommand() {}
	
	virtual QIcon icon() const
	{
		return QIcon(":/buildrun.png");
	}
	
	virtual QString label() const
	{
		return DefaultPlugin::tr("Compile &and run");
	}
	
	virtual Info info(const QString& in, const QString& mode) const
	{
		Info info;
		
		return info;
	}
	
	virtual QList<QBuilder::Command*> depends() const
	{
		return QList<QBuilder::Command*>()
			<< GnuMakeBuilder::m_compile
			<< GnuMakeBuilder::m_run
			;
	}
	
	virtual QBuilder::CommandParser* outputParser() const
	{
		return &_commonParser;
	}
};

class RebuildCommand : public QBuilder::Command
{
	public:
	virtual ~RebuildCommand() {}
	
	virtual QIcon icon() const
	{
		return QIcon(":/rebuild.png");
	}
	
	virtual QString label() const
	{
		return DefaultPlugin::tr("&Rebuild");
	}
	
	virtual Info info(const QString& in, const QString& mode) const
	{
		Info info;
		
		return info;
	}
	
	virtual QList<QBuilder::Command*> depends() const
	{
		return QList<QBuilder::Command*>()
			<< GnuMakeBuilder::m_clean
			<< 0
			<< GnuMakeBuilder::m_compile
			;
	}
	
	virtual QBuilder::CommandParser* outputParser() const
	{
		return &_commonParser;
	}
};

void GnuMakeBuilder::setMakeCommand(const QVariant& v)
{
	Q_UNUSED(v)
}

QBuilder::Command* GnuMakeBuilder::m_compile = 0;
QBuilder::Command* GnuMakeBuilder::m_compileAndRun = 0;
QBuilder::Command* GnuMakeBuilder::m_run = 0;
QBuilder::Command* GnuMakeBuilder::m_clean = 0;
QBuilder::Command* GnuMakeBuilder::m_rebuild = 0;

GnuMakeBuilder::GnuMakeBuilder()
{
	//qDebug("GnuMakeBuilder : %p", this);
	if ( !m_compile )
	{
		m_compile = new CompileCommand;
		m_run = new RunCommand;
		m_compileAndRun = new CompileRunCommand;
		m_clean = new CleanCommand;
		m_rebuild = new RebuildCommand;
		
		//DefaultPlugin::addWatch("QBuilder/GnuMakeBuilder/make", setMakeCommand);
	}
}

GnuMakeBuilder::~GnuMakeBuilder()
{
	//qDebug("~GnuMakeBuilder : %p", this);
}

QString GnuMakeBuilder::name() const
{
	return "GNU Makefile";
}

QString GnuMakeBuilder::label() const
{
	return DefaultPlugin::tr("GNU Makefile");
}

QString GnuMakeBuilder::inputType() const
{
	//qDebug("GnuMakeBuilder::inputType()");
	return "GNU Makefile";
}

QString GnuMakeBuilder::outputType() const
{
	//qDebug("GnuMakeBuilder::outputType()");
	return "exec";
}

QBuilder::Output GnuMakeBuilder::output(const QString& input, const QString& mode) const
{
	//qDebug("gnu make builder : %s", qPrintable(input));
	
	QBuilder::Output o;
	
	o.source = input;
	o.targets << QStringList(QString());
	Makefile::targets(input, o.targets);
	
	if ( o.targets.at(0).count() == 1 )
	{
		o.targets.removeAt(0);
	} else {
		o.targets[0][0] = "default";
	}
	
	//qDebug() << "gnu makefile =>" << endl;
	
	//foreach ( QStringList l, o.targets )
	//	qDebug() << l << endl;
	
	return o;
}

QList<QBuilder::Command*> GnuMakeBuilder::commands() const
{
	static QList<Command*> l = QList<Command*>()
		<< m_compile
		<< m_compileAndRun
		<< m_clean
		<< m_rebuild;
	
	return l;
}

void Makefile::targets(const QString& f, QList<QStringList>& dest, const QString& scope)
{
	//qDebug("querying targets in %s", qPrintable(f));
	
	QStringList l;
	QFile makefile(f);
	QList<QByteArray> subtargets;
	QByteArray destdir, target, sub;
	QString path = QFileInfo(f).path() + QDir::separator();
	
	if ( !makefile.open(QFile::ReadOnly | QFile::Text) )
	{
		//qDebug("Can't open file...");
		return;
	}
	
	while ( !makefile.atEnd() )
	{
		QByteArray line = makefile.readLine().simplified();
		
		int idx = line.indexOf('#');
		
		if ( idx != -1 )
			line = line.left(idx).trimmed();
		
		if ( line.isEmpty() )
			continue;
		
		if ( line.startsWith("SUBTARGETS = ") )
		{
			//qDebug("line : %s", line.constData());
			
			bool multi = false;
			line.remove(0, 13);
			
			if ( line.isEmpty() )
				continue;
			
			multi = line.at(line.count()-1) == '\\';
			
			if ( multi )
				line.chop(1);
			
			if ( line.count() )
				subtargets << line.split(' ');
			
			while ( multi )
			{
				line = makefile.readLine().simplified();
				idx = line.indexOf('#');
				
				if ( idx != -1 )
					line = line.left(idx).trimmed();
				
				if ( line.isEmpty() )
				{
					if ( idx != -1 )
						continue;
					else
						break;
				}
				
				multi = line.at(line.count()-1) == '\\';
				
				if ( multi )
					line.chop(1);
				
				if ( line.count() )
					subtargets << line.split(' ');
			}
			
		} else if ( line.startsWith("DESTDIR = ") ) {
			
			//qDebug("line : %s", line.constData());
			
			line.remove(0, 10);
			
			destdir = line;
			
		} else if ( line.startsWith("TARGET = ") ) {
			
			//qDebug("line : %s", line.constData());
			
			line.remove(0, 9);
			
			target = line;
		}
	}
	
	if ( subtargets.count() )
	{
		foreach ( sub, subtargets )
		{
			if ( sub.isEmpty() )
				continue;
			
			//qDebug("sub target : %s", sub.constData());
			
			if ( sub.startsWith("sub-") )
			{
				sub = sub
						.mid(4, sub.lastIndexOf('-', -5) - 4)
						.replace('-', '/');
				
				//qDebug("%s", qPrintable(path + sub + "/Makefile"));
				
				targets(path + sub + "/Makefile", dest, scope);
			} else {
				QString subscope = scope.count() ? scope + ":" + sub : sub;
				
				QString subfile = f;
				subfile += '.';
				subfile += toupper(sub.at(0));
				subfile += sub.mid(1);
				
				if ( QFile::exists(subfile) )
				{
					bool found = false;
					
					for ( int i = 0; i < dest.count(); ++i )
					{
						QStringList tl = dest.at(i);
						
						if ( tl.at(0) == sub )
						{
							found = true;
							break;
						}
					}
					
					if( !found )
						dest << QStringList(sub);
					
					targets(subfile, dest, subscope);
				} else {
					qWarning("Makefile parser : failed to follow subtarget : %s", sub.constData());
				}
			}
		}
	} else if ( target.count() ) {
		
		if ( destdir.count() &&
			!( destdir.endsWith('/') || destdir.endsWith('\\') ) )
			destdir.append(QDir::separator());
		
		if ( !target.startsWith(destdir) )
			target.prepend(destdir);
		
		l << target;
		
		//qDebug("target : %s", target.constData());
	}
	
	for ( int i = 0; i < dest.count(); ++i )
	{
		QStringList tl = dest.at(i);
		
		if ( tl.isEmpty() || tl.at(0) != scope )
			continue;
		
		foreach ( QString t, l )
		{
			t = QDir::cleanPath(path + t);
			
			if ( !QLibrary::isLibrary(t) ) //QFile::exists(t) && QFileInfo(t).isExecutable() &&  )
			{
				if ( !tl.contains(t) )
				{
					//qDebug(" exec target : %s", qPrintable(t));
					
					//if ( scope.count() )
					//	t.prepend(scope.toLocal8Bit() + ':');
				
					tl << t;
				}
			}
		}
		
		dest[i] = tl;
		
		break;
	}
	//qDebug("done querying targets in %s [found %i]", qPrintable(f), dest.count());
}

QString Makefile::target(const QString& t, const QStringList& cfg)
{
	QList<QStringList> l;
	
	targets(t, l);
	
	QString dirname = QFileInfo(t).dir().dirName();
	
	if ( l.isEmpty() )
		return QString();
	
// 	if ( l.count() == 1 )
// 	{
// 		#ifdef _DEBUG_
// 		qDebug("unique target : %s", qPrintable(l.at(0)));
// 		#endif
// 		
// 		return l.at(0);
// 	}
	
	foreach ( QStringList tl, l )
	{
		if ( !cfg.contains(tl.at(0)) )
			continue;
		
		// try to match exe basename with directory
		foreach ( QString fn, tl )
		{
			if ( QFileInfo(fn).baseName() == dirname )
			{
				#ifdef _DEBUG_
				qDebug("nearest target : %s", qPrintable(fn));
				#endif
				return fn;
			}
		}
	}
	
	#ifdef _DEBUG_
	//qDebug("fallback target : %s", qPrintable(l.at(0)));
	#endif
	
	return QString(); // l.at(0);
}
