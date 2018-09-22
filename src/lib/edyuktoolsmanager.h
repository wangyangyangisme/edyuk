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

#ifndef _EDYUK_TOOLS_MANAGER_H_
#define _EDYUK_TOOLS_MANAGER_H_

#include "edyuk.h"

/*!
	\file edyuktoolsmanager.h
	\brief Definition of the EdyukToolsManager class
	
	\see EdyukToolsManager
*/

#include <QObject>
#include <QProcess>
#include <QStringList>
#include <QDomElement>

class QAction;
class QActionGroup;
class QDomDocument;
class EdyukToolsDialog;

class EDYUK_EXPORT EdyukToolsManager : public QObject
{
	Q_OBJECT
	
	friend class EdyukToolsDialog;
	
	public:
		class Tool : public QDomElement
		{
			public:
				Tool() : QDomElement() {}
				Tool(const QDomElement& e) : QDomElement(e) {}
				
				Tool& operator = (const QDomElement& e)
				{ QDomElement::operator = (e); return *this; }
				
				inline int id() { return attribute("id").toInt(); }
				inline QString caption() { return attribute("caption"); }
				inline QString program() { return attribute("program"); }
				inline QString working() { return attribute("pwd"); }
				
				inline QStringList arguments()
				{ return attribute("arguments").split("#@#"); }
				inline QStringList environment()
				{ return attribute("environment").split("#@#"); }
				
				inline void setId(int i) { setAttribute("id", i); }
				
				inline void setCaption(const QString& s)
				{ setAttribute("caption", s); }
				inline void setProgram(const QString& s)
				{ setAttribute("program", s); }
				inline void setWorking(const QString& s)
				{ setAttribute("pwd", s); }
				
				inline void setArguments(const QStringList& l)
				{ setAttribute("arguments", l.join("#@#")); }
				inline void setEnvironment(const QStringList& l)
				{ setAttribute("environment", l.join("#@#")); }
				
		};
		
		EdyukToolsManager();
		~EdyukToolsManager();
		
		int addTool(const QString& caption, const QString& exe,
					const QString& pwd,
					const QStringList& args = QStringList(),
					const QStringList& env = QStringList());
		
		void remTool(int id);
		void swapToolIds(int id1, int id2);
		
		void retranslate();
		
		void updateActions();
		
	public slots:
		void configure();
		
	private slots:
		void toolTriggered(QAction *a);
		void toolError(QProcess::ProcessError e);
		
	signals:
		void toolsChanged(QActionGroup *g);
		
	private:
		void readXml();
		void writeXml();
		
		Tool tool(int id);
		Tool findXml(const QString& id);
		
		QDomNodeList tools() const;
		
		QDomDocument *pDoc;
		
		QActionGroup *pActions;
		EdyukToolsDialog *pDialog;
		
};

#endif // _EDYUK_TOOLS_MANAGER_H_
