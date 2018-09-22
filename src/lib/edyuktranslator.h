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

#ifndef _EDYUK_TRANSLATOR_H_
#define _EDYUK_TRANSLATOR_H_

#include "edyuk.h"

/*!
	\file edyuktranslator.h
	\brief Definition of the EdyukTranslator class.
	
	\see EdyukTranslator
*/

#include <QHash>
#include <QObject>
#include <QString>
#include <QVector>

#include "qsettingsclient.h"

class QMenu;
class QAction;
class QTranslator;
class QActionGroup;

class EDYUK_EXPORT EdyukTranslator : public QObject, public QSettingsClient
{
	Q_OBJECT
	
	public:
		EdyukTranslator(QSettingsServer *s);
		virtual ~EdyukTranslator();
		
		QAction* action() const;
		
		QString currentLanguage() const;
		
	public slots:
		void setDefaultLanguage();
		void setLanguage(const QString& lang);
		
	signals:
		void languageChanged(const QString& lang);
		
	private slots:
		void scanLangs();
		void setLanguage(QAction *a);
		
	private:
		QMenu *pMenu;
		QActionGroup *g;
		
		QString sLang;
		
		QHash<QAction*, QString> cross;
		QVector<QTranslator*> translators;
};

#endif
