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

#include "edyuktemplatemanager.h"

#include "version.h"

/*!
	\file edyuktemplatemanager.cpp
	
	\brief Implementation of the EdyukTemplateManager class.
*/

#include <QDir>
#include <QFile>
#include <QSettings>
#include <QFileInfo>
#include <QDateTime>
#include <QTextStream>
#include <QApplication>

/*!
	\class EdyukTemplateManager
	
	\brief This class provides a generic template engine for Edyuk.
	
	Template files are stored as .template files written as INI through
	QSettings which contain several settings and a list of related files.
	
	Templates are used by the File creation dialog but can also be accessed
	by plugins. For instance the default plugin uses them when creating new
	C++ source/header files.
	
	A template file basically consists of a main section and several file sections :
	
	\code
	[main]
	name=...
	category=...
	icon=...
	description=...
	
	[file0]
	input=...
	output=...
	extension=...
	open=...
	
	[file1]
	input=...
	output=...
	extension=...
	open=...
	
	[file2]
	input=...
	output=...
	extension=...
	open=...
	
	...
	\endcode
	
	There are three (regular) categories of templates :
	<li>
		<ul><b>projects</b> : A template used to desribe projects (ex : Qt GUI application)</ul>
		<ul><b>files</b> : A template used to desribe single files (ex : C++ source file)</ul>
		<ul><b>extras</b> : A template used to desribe special files/sets of files (ex : C++ class)</ul>
	</li>
	
	The name of file section is absolutely free. Yet keep in mind that each section must appear only
	once... i.e, having n file section with the same name will result in only the last one being considered.
	<li>
		<ul><b>input</b> : a filename (relative to the template file) used to "instanciate" templates i.e. create
		new files or projects.</ul>
		<ul><b>ouptut</b> : the name of the output file. It can make use of substitutions or be
		fied. If no output is specified, it is defaulted to input.</ul>
		<ul><b>extension</b> : this field specify a file extension to use in case the user didn't choose any...</ul>
		<ul><b>open</b> : the open mode, relevant only when "Open" is checked on Create new... dialog. It can take
		three walues :
		<li>
			<ul>no : prevent file from being opened</ul>
			<ul>file : open as file \see EdyukGUI::fileOpen() [default]</ul>
			<ul>project : open as project \see EdyukGUI::projectOpen()</ul>
		</li>
		</lu>
	</li>
	
	Template auxiliary files (referenced within the .template files and distibuted along)
	are also tunnable through special sequences.
	
	Frequently used sequences :
	$$			literal '$' character
	$name$		name passed to template generator (can be project name of output file name)
	$date$		Current date and time (QDateTime::currentDateTime())
	
	$ide_version$			Edyuk version (x.y.z or x.y.z-rcW) (renamed for compat with Monkey Studio)
	$ide_version_string$	Same as version with "Edyuk " prepended
	
	[Specific to template files (input=>output process)]
	$filename$	current output filename
	$$
	
	Besides, the macro'ing engine also support a variety of functions which allow fine-tunning
	of variables value. These functions are accessible by appending a dot and the function name
	after the variable name (still enclosed in the two '$' signs). Another important thing to
	say is that these functions can be mixed in every way you want, just like in C++ and most OO
	languages...
	
	Valid function names are :
	upper		: Name is self-explanatory (see QString documentation)
	lower		: idem
	trimmed		: idem
	simplified	: idem
	
	The following sequences are equivalents :
	\example
	$some_variable.simplified.lower$
	$some_variable.lower.simplified$
	\endexample
	
	And correspond to :
	\code
	QString some_variable; // initialized with proper value...
	some_variable = some_variable.toLower();
	some_variable = some_variable.simplified();
	\endcode
	
	\note All filenames should be relative to the template directory.
*/

EdyukTemplateManager::EdyukTemplateManager(QObject *p)
 : QObject(p)
{
	scan();
	
	setVariableValue("ide_name", "Edyuk");
	setVariableValue("ide_version", EDYUK_VERSION_STR);
	setVariableValue("ide_version_string", QString("Edyuk ") + EDYUK_VERSION_STR);
}

void EdyukTemplateManager::scan()
{
	m_templates.clear();
	
	QStringList l = Edyuk::dataPathes();
	
	foreach ( QString dir, l )
	{
		QDir d(dir);
		
		if ( !d.cd("templates") )
		{
			if ( !QFileInfo(d.path()).isWritable() )
				continue;
			
			d.mkdir("templates");
			d.cd("templates");
		}
		
		foreach ( QString entry, d.entryList(QDir::Files | QDir::Readable) )
		{
			//qDebug("entry %s", qPrintable(entry));
			
			if ( !entry.endsWith(".template") )
				continue;
			
			//qDebug("\t=>template found");
			
			QSettings template_file(d.filePath(entry), QSettings::IniFormat);
			
			EdyukTemplate tpl;
			tpl.filename = d.filePath(entry);
			tpl.id = QFileInfo(entry).baseName();
			
			QStringList groups = template_file.childGroups();
			
			if ( !groups.contains("main") )
				continue;
			else
				groups.removeAll("main");
			
			template_file.beginGroup("main");
			
			tpl.name = template_file.value("name").toString();
			tpl.icon = template_file.value("icon").toString();
			tpl.category = template_file.value("category", "extras").toString();
			tpl.description = template_file.value("description", "missing description").toString();
			template_file.endGroup();
			
			foreach ( QString entity, groups )
			{
				template_file.beginGroup(entity);
				
				EdyukTemplateFile tf;
				
				tf.input = template_file.value("input").toString();
				tf.output = template_file.value("output", QFileInfo(tf.input).fileName()).toString();
				tf.extension = template_file.value("extension").toString();
				
				QString m = template_file.value("open", "file").toString();
				tf.mode = (m == "no") ? Edyuk::No : ((m == "project") ? Edyuk::Project : Edyuk::File);
				
				tpl.files << tf;
				
				template_file.endGroup();
			}
			
			m_templates << tpl;
		}
	}
}

QStringList EdyukTemplateManager::categories() const
{
	QStringList l;
	
	foreach ( const EdyukTemplate& tpl, m_templates )
		if ( !l.contains(tpl.category) )
			l << tpl.category;
	
	return l;
}

EdyukTemplate EdyukTemplateManager::templateForId(const QString& id) const
{
	foreach ( const EdyukTemplate& tpl, m_templates )
		if ( tpl.id == id )
			return tpl;
	
	return EdyukTemplate();
}

EdyukTemplate EdyukTemplateManager::templateForName(const QString& name) const
{
	foreach ( const EdyukTemplate& tpl, m_templates )
		if ( tpl.name == name )
			return tpl;
	
	return EdyukTemplate();
}

EdyukTemplateList EdyukTemplateManager::templates(const QString& category) const
{
	if ( category.isEmpty() )
		return m_templates;
	
	EdyukTemplateList l;
	
	foreach ( const EdyukTemplate& tpl, l )
		if ( tpl.category == category )
			l << tpl;
	
	return l;
}

/*!
	\brief Substitute generic sequences $${...}
*/
QString EdyukTemplateManager::substitute(const QString& v, const QHash<QString, QString>& macros)
{
	QString s;
	//static QScriptEngine engine;
	
	
	for ( int i = 0; i < v.count(); ++i )
	{
		if ( v.mid(i, 3) == "$${" )
		{
			i += 3;
			int e = v.indexOf('}', i);
			
			if ( e == -1 )
				break;
			
			QString sub, id = v.mid(i, e - i);
			i = e;
			
			if ( id.contains(',') )
			{
				QStringList alter = id.split(',');
				QStringList cond, val;
				
				foreach ( const QString& a, alter )
				{
					if ( a.count(':') == 1 )
					{
						QStringList tmp = a.split(':');
						cond << tmp.at(0);
						val  << tmp.at(1);
					} else {
						cond << QString();
						val  << a;
					}
				}
				
				for ( int j = 0; j < cond.count(); ++j )
				{
					if ( false ) //isTrue(cond.at(j)) )
					{
						sub = val.at(j);
						break;
					}
				}
				
			} else {
				sub = macros[id];
			}
			
			if ( sub.count() )
			{
				while ( ((e + 1) < v.count()) && (s.at(e + 1) == '.') )
				{
					// functions :D
					i = e + 2;
					e = v.indexOf('(', i);
					
					if ( e != -1 )
					{
						QString func = v.mid(i, e - i);
						
						
					} else {
						e = v.count();
					}
				}
				
				s += sub;
			}
		} else if ( ((i + 1) < v.count()) && (v.at(i) == '\\') ) {
			s += v.at(++i);
		} else {
			s += v.at(i);
		}
	}
	
	return s;
}

void EdyukTemplateManager::macro_substitution(QString& s, const QHash<QString, QString>& macros)
{
	QStringList functions;
	int index = 0, next = 0;
	
	while ( index < s.count() )
	{
		index = s.indexOf('$', index);
		
		if ( index == -1 )
			break;
		
		if ( s.at(index + 1) == '$' )
		{
			// litteral '$'
			s.remove(index, 1);
		} else {
			next = s.indexOf('$', index + 1);
			
			if ( next == -1 )
				break;
			
			QString var = s.mid(index + 1, next - index - 1);
			
			//qDebug("variable : %s", qPrintable(var));
			
			functions = var.split('.', QString::SkipEmptyParts);
			
			var = functions.takeAt(0);
			
			//
			//if ( (function = var.indexOf('.')) != -1 )
			//{
			//	fct = var.mid(function + 1);
			//	var = var.left(function - 1);
			//}
			///
			
			QString rep = macros[var];
			
			while ( functions.count() )
			{
				QString fct = functions.takeFirst();
				
				//qDebug("shall do further processing : %s", qPrintable(fct));
				if ( fct == "upper" )
					rep = rep.toUpper();
				else if ( fct == "lower" )
					rep = rep.toLower();
				else if ( fct == "trimmed" )
					rep = rep.trimmed();
				else if ( fct == "simplified" )
					rep = rep.simplified();
				else
					qWarning("Unhandled function : %s", qPrintable(fct));
			}
			
			s.replace(index, next - index + 1, rep);
			
			next = index;
		}
	}
}

void EdyukTemplateManager::create(	const QString& tpl, const QString& name,
									QStringList *output_files,
									const QHash<QString, QString> *custom) const
{
	EdyukTemplate t = templateForName(tpl);
	
	if ( t.name.isEmpty() )
		t = templateForId(tpl);
	
	create(t, name, output_files, custom);
}

void EdyukTemplateManager::create(	const EdyukTemplate& tpl, const QString& name,
									QStringList *output_files,
									const QHash<QString, QString> *custom) const
{
	QFileInfo info(name);
	
	setVariableValue("name", info.baseName());
	
	setVariableValue("date", QDateTime::currentDateTime().toString());
	
	foreach ( EdyukTemplateFile tf, tpl.files )
	{
		QString abs_in = Edyuk::makeAbsolute(tf.input, tpl.filename),
				abs_out = Edyuk::makeAbsolute(tf.output, name);
		
		if ( custom )
			macro_substitution(abs_out, *custom);
		
		macro_substitution(abs_out, m_variables);
		
		if ( QFileInfo(abs_out).completeSuffix().isEmpty() )
			abs_out += "." + tf.extension;
		
		//qDebug("Concretizing template : %s => %s", qPrintable(abs_in), qPrintable(abs_out));
		
		QFileInfo info_out(abs_out);
		
		setVariableValue("filename", info_out.fileName());
		setVariableValue("basename", info_out.baseName());
		
		QFile fin(abs_in);
		
		if ( !fin.open(QFile::ReadOnly | QFile::Text) )
		{
			qWarning("Unable to open template input : %s", qPrintable(abs_in));
			continue;
		}
		
		QFile fout(abs_out);
		
		if ( !fout.open(QFile::WriteOnly | QFile::Text) )
		{
			qWarning("Unable to open template output : %s", qPrintable(abs_out));
			continue;
		}
		
		QTextStream input(&fin);
		
		QString data = input.readAll();
		
		if ( custom )
			macro_substitution(data, *custom);
		
		macro_substitution(data, m_variables);
		
		QTextStream output(&fout);
		
		output << data;
		
		if ( output_files )
		{
			abs_out.prepend(":");
			abs_out.prepend(QString::number(tf.mode));
			
			output_files->append(abs_out);
		}
	}
}

void EdyukTemplateManager::setVariableValue(const QString& variable, const QString& value) const
{
	m_variables[variable] = value;
	
	emit variableValueChanged(variable, value);
}
