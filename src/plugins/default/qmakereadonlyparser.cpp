/****************************************************************************
**
** Copyright (C) 2006-2008 fullmetalcoder <fullmetalcoder@hotmail.fr>
**
** This file is part of the Edyuk project <http://edyuk.org>
** 
** This file may be used under the terms of the GNU General Public License
** version 2 as published by the Free Software Foundation and appearing in the
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/


#include "qmake/project.h"
#include "qmake/property.h"
#include "qmake/option.h"
#include "qmake/cachekeys.h"

#include <QDir>

#include "qmakeparser.h"
#include "qmakebackend.h"

using namespace QMakeModel;

// <BorrowedCode origin="qmake sources">

// for Borland, main is defined to qMain which breaks qmake
#undef main
#ifdef Q_OS_MAC
#endif

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

// </BorrowedCode>

static QMap<QString, QStringList> m_conf;

int fillReadOnlyProject(QProject *p, INode *n, const QString& projectFile)
{
	static QStringList variableFilter = QStringList()
		<< "SOURCES"
		<< "HEADERS"
		<< "RESOURCES"
		<< "FORMS"
		<< "TEXTS"
		<< "TRANSLATIONS"
		<< "SUBDIRS"
		<< "UI_DIR"
		<< "DEPENDPATH"
		<< "INCLUDEPATH"
		<< "LIBS"
		<< "CONFIG"
		<< "DEFINES"
		<< "TEMPLATE"
		<< "VERSION"
		<< "LANGUAGE"
		<< "APP_NAME"
		<< "APP_AUTHOR"
		;
	
	// <InspiredCode origin="qmake sources">
	static bool initialized = false;
	
	if ( !initialized )
		Option::init(0, 0);
	
	/*
	if(ret != Option::QMAKE_CMDLINE_SUCCESS) {
		if ((ret & Option::QMAKE_CMDLINE_ERROR) != 0) {
			return 1;
		}
		return 0;
	}
	*/
	
	QString oldpwd = QFileInfo(projectFile).absoluteFilePath(); //qmake_getpwd();
	#ifdef Q_WS_WIN
	if(!(oldpwd.length() == 3 && oldpwd[0].isLetter() && oldpwd.endsWith(":/")))
	#endif
	{
		if(oldpwd.right(1) != QString(QChar(QDir::separator())))
			oldpwd += QDir::separator();
	}
	
	Option::output_dir = oldpwd; //for now this is the output dir
	
	int exit_val = 0;
	QMakeProject project;
	QString fn = projectFile; //Option::fixPathToLocalOS(projectFile);
	
	if ( !QFile::exists(fn) )
	{
		//fprintf(stderr, "Cannot find file: %s.\n", fn.toLatin1().constData());
		qDebug("cannot load %s", qPrintable(fn));
		return 2;
	}
	
	//setup pwd properly
	qmake_setpwd(oldpwd); //reset the old pwd
	
	/*
	int di = fn.lastIndexOf(Option::dir_sep);
	if ( di != -1 )
	{
		//debug_msg(1, "Changing dir to: %s", fn.left(di).toLatin1().constData());
		
		//if( !qmake_setpwd(fn.left(di)) )
		//	fprintf(stderr, "Cannot find directory: %s\n", fn.left(di).toLatin1().constData());
		
		fn = fn.right(fn.length() - di - 1);
	}
	*/
	
	// read project..
	if ( !project.read(fn) )
	{
		//fprintf(stderr, "Error processing project file: %s\n",
		//		fn == "-" ? "(stdin)" : (*pfile).toLatin1().constData());
		qDebug("error processing %s", qPrintable(fn));
		
		return 3;
	}
	
	qmakeClearCaches();
	
	// </InspiredCode>
	
	int kept = 0;
	QMap<QString, QStringList> variables = project.variables();
	QMap<QString, QStringList>::const_iterator it = variables.constBegin();
	
	while ( it != variables.constEnd() )
	{
		if ( !variableFilter.contains(it.key()) )
		{
			++it;
			continue;
		}
		
		++kept;
		
		bool subdirs = it.key() == QLatin1String("SUBDIRS");
		
		setNextNodeType(QProjectNode::Folder);
		
		INode *v = new INode(INode::Variable, it.key() + " =");
		INodeBackingStore::instance()->appendChild(n, v, NodeProvider);
		
		QProjectNode::NodeType nt = QProjectNode::File;
		
		if ( subdirs )
			nt = QProjectNode::Project;
		
		setNextNodeType(nt);
		
		foreach ( const QString& val, *it )
		{
			bool done = false;
			
			if ( subdirs )
			{
				// TODO : support objects...
				QString fn = p->absoluteFilePath(
								QMakeModel::Search::substitute(
									val,
									p,
									Search::Project
								).at(0)
							);
				
				QFileInfo info(fn);
				
				if ( info.isDir() )
				{
					QDir d(fn);
					QStringList availables;
					QString pfn = d.absoluteFilePath(info.baseName() + ".pro");
					
					foreach (
								const QFileInfo& i,
								d.entryInfoList(QDir::Files | QDir::Readable)
							)
					{
							availables << i.absoluteFilePath();
					}
					
					if ( availables.isEmpty() )
						fn.clear();
					else if ( availables.count() == 1 )
						fn = availables.at(0);
					else if ( availables.contains(pfn) )
						fn = pfn;
					else {
						qWarning("%s : Ambiguous subdir resolution.", __FUNCTION__);
						fn = availables.at(0);
					}
				}
				
				if ( QFile::exists(fn) )
				{
					//qDebug("subdir : %s", qPrintable(fn));
					
					bool shared = false;
				
					INode *sub = INodeBackingStore::instance()
									->getFileNode(INode::Project, fn, &shared);
					
					QList<QProject*> before = p->subProjects();
					
					INodeBackingStore::instance()->appendChild(	v,
																sub,
																ProjectProvider,
																shared);
					
					QList<QProject*> after = p->subProjects();
					
					foreach ( QProject *sp, before )
						after.removeAll(sp);
					
					if ( !shared || (after.count() == 1) )
					{
						if ( after.count() != 1 )
							qWarning("quirk...");
						
						fillReadOnlyProject(after.at(0), sub, fn);
						//qDebug("%s filled...", qPrintable(fn));
					} else {
						//qDebug("%s shared...", qPrintable(fn));
					}
					
					done = true;
				} else {
					qDebug("unable to locate subdir : %s", qPrintable(fn));
					setNextNodeType(QProjectNode::File);
				}
			}
			if ( !done )
			{
				INode *n = new INode(INode::Value, val);
				
				INodeBackingStore::instance()->appendChild(v, n, NodeProvider);
			}
		}
		
		++it;
	}
	
	qDebug("%s filled (%i variables kept for %i availables)...", qPrintable(fn), kept, variables.count());
	
	return exit_val;
}
