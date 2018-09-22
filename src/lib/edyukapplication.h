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

#ifndef _EDYUK_APPLICATION_H_
#define _EDYUK_APPLICATION_H_

#include "edyuk.h"

#include "qsingleapplication.h"

class QEvent;

class EdyukGUI;
class EdyukSettings;
class EdyukTranslator;
class EdyukToolsManager;
class EdyukTemplateManager;

class QSnippetManager;
class QShortcutManager;
class QCodeCompletionEngine;

class QSessionManager;

class EDYUK_EXPORT EdyukApplication : public QSingleApplication
{
	friend QString Edyuk::settingsPath();
	friend void CompletionEngineHandler(void *p);
	
	Q_OBJECT
	
	public:
		EdyukApplication(int& argc, char **argv);
		virtual ~EdyukApplication();
		
		static EdyukApplication* Instance();
		
		virtual void commitData(QSessionManager& manager);
		virtual void saveState (QSessionManager& manager);
		
		virtual int exec();
		virtual bool event(QEvent *e);
		
		//
		
		static int version();
		static QString versionString();
		
		EdyukGUI* gui() const;
		EdyukTranslator* translator() const;
		EdyukToolsManager* toolsManager() const;
		QShortcutManager* shortcutManager() const;
		EdyukTemplateManager* templateManager() const;
		QSnippetManager* snippetManager() const;
		
		//
		
		QString currentFile() const;
		QString currentProject() const;
		
		QStringList openedFiles() const;
		QStringList openedProjects() const;
		
		//
		
		bool loggerReady() const;
		void log(const QString& s);
		
	protected:
		virtual void request(const QString& l);
		virtual void request(const QStringList& l);
		
	private slots:
		void reopen();
		void registerEngineTrigger(QCodeCompletionEngine *eng);
		
	private:
		QString m_path;
		EdyukGUI *pGUI;
		EdyukSettings *pSettings;
		EdyukTranslator *pTranslator;
		EdyukToolsManager *pToolsManager;
		QShortcutManager *pShortcutManager;
		EdyukTemplateManager *pTemplateManager;
		QSnippetManager *m_snippetManager;
};

#define COMPONENT(TYPE) EdyukApplication::Instance()->TYPE()

#define DEV_SHORTCUT COMPONENT(shortcutManager)
#define EDYUK_SHORTCUT(action, context, defaut) \
		COMPONENT(shortcutManager)->registerAction(action, context, defaut)

#endif // _EDYUK_APPLICATION_H_
