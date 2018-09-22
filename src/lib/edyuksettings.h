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

#ifndef _EDYUK_SETTINGS_H_
#define _EDYUK_SETTINGS_H_

#include "edyuk.h"

/*!
	\file edyuksettings.h
	\brief Definition of the EdyukSettings class
	
	\see EdyukSettings
*/

#include <QHash>
#include <QStringList>
#include "qsettingsserver.h"

class QMenu;
class QAction;
class QSnippetManager;
class QLanguageFactory;
class EdyukConfigDialog;

class EDYUK_EXPORT EdyukSettings : public QSettingsServer
{
	Q_OBJECT
	
	public:
		EdyukSettings(QObject *p = 0);
		EdyukSettings(const QString& loc, QObject *p = 0);
		virtual ~EdyukSettings();
		
		QAction* recent() const;
		
		QString environment(const QString& var);
		QStringList environment(const QStringList& dirs = QStringList());
		
	public slots:
		void configure();
		virtual void setDefault();
		virtual void retranslate();
		
		void addRecentFile(const QString& filename);
		void addRecentProject(const QString& filename);
		
		void addRecent(const QString& filename, bool project);
		
		void setSnippetManager(QSnippetManager *m);
		void loadFormatSchemes(QLanguageFactory *f);
		
	protected slots:
		void buildRecents();
		void clearRecents();
		void recent(QAction *a);
		
	signals:
		void recentFile(const QString& f);
		void recentProject(const QString& f);
		
	private:
		QHash<QAction*, QString> recentFiles;
		QHash<QAction*, QString> recentProjects;
		
		EdyukConfigDialog *pCfgDlg;
		
		QMenu *m;
		QAction *aClear;
		
		QString sRecent;
		
		static const QString PATH_VAR;
};

#endif // !_EDYUK_SETTINGS_H_
