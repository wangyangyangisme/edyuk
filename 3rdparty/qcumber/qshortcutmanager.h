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

#ifndef _QSHORTCUT_MANAGER_H_
#define _QSHORTCUT_MANAGER_H_

#include "qcumber.h"

/*!
	\file qshortcutmanager.h
	\brief Definition of the QShortcutManager class
	
	\see QShortcutManager
*/

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QDomElement>

class QAction;
class QDomDocument;
class QShortcutDialog;

class QCUMBER_EXPORT QShortcutManager : public QObject
{
	Q_OBJECT
	
	friend class QShortcutDialog;
	
	public:
		struct Action
		{
			QAction *action;
			QString context;
		};
		
		QShortcutManager();
		virtual ~QShortcutManager();
		
		static QString settingsPath();
		static void setSettingsPath(const QString& path);
		
		void applyAll();
		void apply(const QString& s, const QString& t);
		
		bool contains(QAction *a) const;
		
		void registerAction(QAction *a, const QString& cxt, const QString& def);
		void unregisterAction(QAction *a);
		
		void translateContext(const QString& cxt, const QString& tcxt);
		
	public slots:
		void readXml();
		void writeXml();
		
		void configure();
		void languageChanged(const QString& lang);
		
	private slots:
		void destroyed(QObject *o);
		
	private:
		QString file(const QString& lang);
		QDomElement node(QAction *a, const QString& cxt);
		QDomElement node(const QString& n, const QString& cxt, bool create = false);
		
		QString sLang;
		static QString sPath;
		
		QDomDocument *pDoc;
		
		QHash<QString, QString> m_shortcuts;
		QHash<QString, QString> m_translations;
		QHash<QString, QList<QAction*> > m_actions;
		
		QShortcutDialog *pDialog;
};

#endif // _QSHORTCUT_MANAGER_H_
