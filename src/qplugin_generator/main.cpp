/****************************************************************************
**
** Copyright (C) 2006 FullMetalCoder
**
** This file is part of the Edyuk project <http://edyuk.org> (beta version)
** 
** This file may be used under the terms of the GNU General Public License
** version 2 as published by the Free Software Foundation and appearing in the
** file GPL.txt included in the packaging of this file.
**
** Notes :	Parts of the project are derivative work of Trolltech's QSA library
** or Trolltech's Qt4 framework but, unless notified, every single line of code
** is the work of the Edyuk team or a contributor. 
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <QDir>
#include <QFile>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QXmlStreamReader>

#define VERSION "1.0.0"

int usage(char *argv0)
{
	QTextStream out(stdout);
	
	out << endl
		<< "QPlugin Generator : generates plugin code from XML config scheme"
		<< endl
		<< "Usage : " << endl
		<< argv0 << " [options]" << endl
		<< endl << endl
			<< "\t-i <filename>   : specify input file" << endl
			<< "\t-o <basename>   : specify basename of the output files" << endl
			<< "\t-r <filename>   : specify filename of the resource file storing the scheme" << endl
			<< "\t-h              : display this help" << endl
		<< endl << endl;
	
	return 1;
}

bool parse(int argc, char **argv, QHash<QString, QString>& map)
{
	if ( argc <= 1 )
		return true;
	
	for ( int i = 1; i < argc; ++i )
	{
		if ( QLatin1String("-i") == argv[i] )
		{
			map["input"] = QString::fromLocal8Bit(argv[++i]);
		} else if ( QLatin1String("-o") == argv[i] ) {
			map["output"] = QString::fromLocal8Bit(argv[++i]);
		} else if ( QLatin1String("-r") == argv[i] ) {
			map["resource"] = QString::fromLocal8Bit(argv[++i]);
		} else if ( QLatin1String("-h") == argv[i] ) {
			return true;
		}
	}
	
	return false;
}

void concrete(const QString& in, const QString& out, const QHash<QString, QString>& vars)
{
	QFile fin(in), fout(out);
	QTextStream sin(&fin), sout(&fout);
	
	if ( !fin.open(QFile::ReadOnly | QFile::Text) )
	{
		qWarning("Unable to open template : %s", qPrintable(in));
		return;
	}
	
	QString buffer = fin.readAll();
	fin.close();
	
	QHash<QString, QString>::const_iterator it = vars.constBegin();
	
	while ( it != vars.constEnd() )
	{
		buffer.replace("$" + it.key() + "$", *it);
		
		++it;
	}
	
	buffer.replace("$$", "$");
	
	if ( !fout.open(QFile::WriteOnly | QFile::Text) )
	{
		qWarning("Unable to write template output : %s", qPrintable(out));
		return;
	}
	
	sout << buffer;
}

int main(int argc, char **argv)
{
	QHash<QString, QString> options;
	
	options["input"] = "plugin.xml";
	options["output"] = "plugin";
	options["resource"] = "plugin.qrc";
	
	if ( parse(argc, argv, options) )
		return usage(*argv);
	
	// parse resource file to get path of the scheme
	QString schemePath, subrsrc,
			resPath = options["resource"],
			resInRef = QFileInfo(options["input"]).fileName();
	
	QFile rsrc(resPath);
	
	if ( !rsrc.open(QFile::ReadOnly | QFile::Text) )
	{
		qWarning("Unable to open resource file : %s", qPrintable(resPath));
		return 1;
	}
	
	QXmlStreamReader resource(&rsrc);
	
	while ( !resource.atEnd() )
	{
		resource.readNext();
		
		if ( resource.isStartElement() )
		{
			if ( resource.name() == "qresource" )
			{
				subrsrc = resource.attributes().value("prefix").toString();
			} else if ( resource.name() == "file" ) {
				QString alias = resource.attributes().value("alias").toString();
				
				QString fn = resource.readElementText();
				
				if ( subrsrc.count() && (fn == resInRef) )
				{
					schemePath = subrsrc;
					
					if ( schemePath.count() && !schemePath.endsWith("/") )
						schemePath += "/";
					
					schemePath += alias.count() ? alias : fn;
					break;
				}
			}
		}
	}
	
	rsrc.close();
	
	if ( schemePath.isEmpty() )
	{
		qWarning("Resource file %s does not references scheme file %s",
				qPrintable(resPath),
				qPrintable(options["input"])
				);
		
		return 1;
	}
	
	// parse config scheme
	QFile in(options["input"]);
	
	if ( !in.open(QFile::ReadOnly | QFile::Text) )
	{
		qWarning("Unable to open input file : %s", qPrintable(options["input"]));
		return 1;
	}
	
	QXmlStreamReader scheme(&in);
	
	QString className;
	QStringList strings;
	QHash<QString, QStringList> components;
	QHash<QString, QStringList>::iterator current = components.end();
	
	while ( !scheme.atEnd() )
	{
		scheme.readNext();
		
		if ( scheme.isStartElement() )
		{
			if ( scheme.name() == "EdyukPlugin" )
			{
				//qDebug("found start elem EdyukPlugin");
				className = scheme.attributes().value("class").toString();
			} else if ( scheme.name() == "Description" ) {
				strings << scheme.readElementText().trimmed();
			} else if ( scheme.name() == "Component" ) {
				QString type = scheme.attributes().value("type").toString();
				
				current = components.find(type);
				
				if ( current == components.end() )
				{
					current = components.insert(type, QStringList());
				}
			} else if ( scheme.name() == "Class" ) {
				QString id = scheme.attributes().value("name").toString();
				
				if ( current != components.end() )
				{
					if ( !current->contains(id) && id.count() )
						current->append(id);
				}
			}
			
			QStringRef lbl = scheme.attributes().value("label");
			
			if ( lbl.count() )
				strings << lbl.toString();
			
		} else if ( scheme.isEndElement() ) {
			if ( scheme.name() == "Component" )
			{
				current = components.end();
			}
		}
	}
	
	if ( scheme.hasError() )
	{
		qWarning("Error encountered while processing config scheme : \n%s",
				qPrintable(scheme.errorString())
				);
		
		return 1;
	}
	
	//qDebug("Done loading XML config scheme.");
	
	QHash<QString, QString> vars;
	
	vars["version"] = VERSION;
	vars["class"] = className;
	vars["input"] = QFileInfo(options["input"]).fileName();
	vars["output"] = QFileInfo(options["output"]).fileName();
	vars["scheme_resource"] = QFileInfo(options["resource"]).baseName();
	vars["scheme_path"] = schemePath;
	
	QString tmp("_");
	
	// compute header guard
	for ( int i = 0; i < className.count(); ++i )
	{
		tmp += className.at(i).toUpper();
		
		if (
				((i + 1) < className.count())
			&&
				className.at(i).isLower()
			&&
				className.at(i + 1).isUpper()
			)
			tmp += '_';
		
	}
	
	tmp += "_H_";
	
	vars["header_guard"] = tmp;
	tmp.clear();
	// ---
	
	//qDebug("Done computing header guard.");
	
	// add stub strings to ensure translation will happen
	foreach ( QString str, strings )
	{
		tmp += "\ttr(\"";
		tmp += str.simplified();
		tmp += "\");\n";
	}
	
	tmp.chop(1);
	
	vars["scheme_strings"] = tmp;
	tmp.clear();
	// ---
	
	//qDebug("Done computing stub translations.");
	
	// Compute all components lists at once
	current = components.begin();
	
	QString ctypes, ckeys, objforkey, common;
	QString forward, members, include, init, del;
	
	objforkey = ckeys = ctypes = "\t";
	ctypes = "\treturn QStringList()\n";
	
	while ( current != components.end() )
	{
		if ( current->isEmpty() )
		{
			++current;
			continue;
		}
		
		QString t = current.key();
		ctypes += "\t\t<< QPLUGIN_KEY(" + t + ")\n";
		
		common = "if ( t == QPLUGIN_KEY(" + t + ") ) {\n";
		ckeys += common + "\t\treturn QStringList()\n";
		objforkey += common;
		
		foreach ( const QString& c, *current )
		{
			ckeys += "\t\t\t<< QPLUGIN_KEY(" + c + ")\n";
			objforkey += "\t\tQPLUGIN_INST(id, " + c + ", " + t + ")\n";
			
			QString inst = "QPLUGIN_COMPONENT(" + c + ")";
			
			forward	+= "class " + c + ";\n";
			//members	+= "\t\t" + c + " *" + inst + ";\n";
			members	+= "\t\tQPLUGIN_ADD_COMPONENT(" + c + ");\n";
			include	+= "#include \"" + c.toLower() + ".h\"\n";
			init	+= "\t" + inst + " = 0;\n";
			del		+= "\tdelete " + inst + ";\n";
		}
		
		common = "\t} else ";
		
		ckeys += "\t\t\t;\n" + common;
		objforkey += common;
		
		++current;
	}
	
	common = "{\n\t\tqDebug(\"Unknow object type : %s\", qPrintable(t));\n\t}";
	
	forward.chop(1);
	vars["forward_component_list"] = forward;
	
	members.chop(1);
	vars["members_component_list"] = members;
	
	include.chop(1);
	vars["include_component_list"] = include;
	
	init.chop(1);
	vars["init_component_list"]    = init;
	
	del.chop(1);
	vars["delete_component_list"]  = del;
	
	ctypes += "\t\t;";
	vars["component_types"] = ctypes;
	
	ckeys += common;
	vars["component_keys"] = ckeys;
	
	objforkey += common;
	vars["object_for_component"] = objforkey;
	// ---
	
	//qDebug("Done computing component lists.");
	
	// can't do it earlier... QStringRef's would get invalid
	in.close();
	
	concrete(":/plugin.h", options["output"] + ".h", vars);
	concrete(":/plugin.cpp", options["output"] + ".cpp", vars);
	
	return 0;
}
