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

#ifndef _EDYUK_TEMPLATE_MANAGER_H_
#define _EDYUK_TEMPLATE_MANAGER_H_

#include "edyuk.h"

/*!
	\file edyuktemplatemanager.h
	
	\brief Definition of the EdyukTemplateManager class.
*/

#include <QHash>
#include <QString>
#include <QObject>

namespace Edyuk
{
	enum OpenMode
	{
		No,
		File,
		Project
	};
}

struct EdyukTemplateFile
{
	QString input;
	QString output;
	QString extension;
	
	Edyuk::OpenMode mode;
};

typedef QList<EdyukTemplateFile> EdyukTemplateFileList;

struct EdyukTemplate
{
	QString id;
	QString filename;
	
	QString name;
	QString icon;
	QString category;
	QString description;
	
	EdyukTemplateFileList files;
};

typedef QList<EdyukTemplate> EdyukTemplateList;

class EDYUK_EXPORT EdyukTemplateManager : public QObject
{
	Q_OBJECT
	
	public:
		EdyukTemplateManager(QObject *p = 0);
		
		void scan();
		
		QStringList categories() const;
		
		EdyukTemplate templateForId(const QString& id) const;
		EdyukTemplate templateForName(const QString& name) const;
		EdyukTemplateList templates(const QString& type = QString()) const;
		
		void create(const QString& tpl, const QString& name,
					QStringList *output_files = 0,
					const QHash<QString, QString> *custom = 0) const;
		
		void create(const EdyukTemplate& tpl, const QString& name,
					QStringList *output_files = 0,
					const QHash<QString, QString> *custom = 0) const;
		
		static void macro_substitution(QString& s, const QHash<QString, QString>& macros);
		static QString substitute(const QString& v, const QHash<QString, QString>& macros);
		
	public slots:
		void setVariableValue(const QString& variable, const QString& value) const;
		
	signals:
		void variableValueChanged(const QString& variable, const QString& value) const;
		
	private:
		EdyukTemplateList m_templates;
		mutable QHash<QString, QString> m_variables;
};

#endif // !_EDYUK_TEMPLATE_MANAGER_H_
