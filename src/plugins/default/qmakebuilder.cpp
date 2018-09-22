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

#include "qmakebuilder.h"

#include "plugin.h"

#include <QDir>
#include <QIcon>
#include <QDebug>
#include <QProcess>
#include <QFileInfo>

class QMakeCommand : public QBuilder::Command
{
	public:
	virtual ~QMakeCommand() {}
	
	virtual QIcon icon() const
	{
		return QIcon();
	}
	
	virtual QString label() const
	{
		return DefaultPlugin::tr("&Generate");
	}
	
	virtual bool mayAffectTargetList() const
	{
		return false;
	}
	
	virtual Info info(const QString& in, const QString& mode) const
	{
		Info info;
		info.exec = DefaultPlugin::configKey<QString>("QBuilder/QMakeBuilder/qmake", "qmake");
		
		info.arguments << in;
		info.output = QDir(QFileInfo(in).absolutePath()).absoluteFilePath("Makefile");
		
		return info;
	}
	
	virtual QList<QBuilder::Command*> depends() const
	{
		return QList<QBuilder::Command*>();
	}
};

void QMakeBuilder::setQMakeCommand(const QVariant& v)
{
	Q_UNUSED(v)
	/*
	if ( !m_qmakeCommand )
		m_qmakeCommand = new QMakeCommand;
	
	m_qmakeCommand->command = v.toString();
	*/
}

QBuilder::Command* QMakeBuilder::m_qmakeCommand = 0;

QMakeBuilder::QMakeBuilder()
{
	//qDebug("QMakeBuilder : %p", this);
	
	if ( !m_qmakeCommand )
	{
		m_qmakeCommand = new QMakeCommand;
		
		//DefaultPlugin::addWatch("QBuilder/QMakeBuilder/qmake", setQMakeCommand);
	}
}

QMakeBuilder::~QMakeBuilder()
{
	//qDebug("~QMakeBuilder : %p", this);
}

QString QMakeBuilder::name() const
{
	return "QMake project";
}

QString QMakeBuilder::label() const
{
	return DefaultPlugin::tr("QMake project");
}

QString QMakeBuilder::inputType() const
{
	//qDebug("QMakeBuilder::inputType()");
	return "qmake project";
}

QString QMakeBuilder::outputType() const
{
	//qDebug("QMakeBuilder::outputType()");
	return "GNU Makefile";
}

QBuilder::Output QMakeBuilder::output(const QString& input, const QString& mode) const
{
	//qDebug("qmake builder : %s", qPrintable(input));
	
	QBuilder::Output o;
	
	QStringList l;
	l << QString() << QDir(QFileInfo(input).absolutePath()).absoluteFilePath("Makefile");
	
	o.source = input;
	o.targets << l;
	
	//qDebug() << "Makefile => " << o.targets << endl;
	
	return o;
}

QList<QBuilder::Command*> QMakeBuilder::commands() const
{
	static QList<Command*> l = QList<Command*>()
		<< m_qmakeCommand;
	
	return l;
}
