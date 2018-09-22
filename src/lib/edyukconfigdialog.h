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

#ifndef _EDYUK_CONFIG_DIALOG_H_
#define _EDYUK_CONFIG_DIALOG_H_

#include "edyuk.h"

#include "ui_configdialog.h"

/*!
	\file edyukconfigdialog.h
	
	\brief Definition of the EdyukConfigDialog class.
*/

#include <QList>
#include <QPointer>

class QListWidgetItem;

class QEditConfig;
class QFormatConfig;
class QLanguageFactory;

class QSettingsServer;

class QSnippetEdit;
class QSnippetManager;

class QPluginManagerConfig;

class EdyukConfigDialog : public QDialog, private Ui::ConfigDialog
{
	Q_OBJECT
	
	public:
		EdyukConfigDialog(QSettingsServer *s, QWidget *p = 0);
		virtual ~EdyukConfigDialog();
		
	public slots:
		void apply();
		void reload();
		void retranslate();
		
		void tryAccept();
		void tryReject();
		
		void setSnippetManager(QSnippetManager *m);
		
		void loadFormatSchemes(QLanguageFactory *f);
		
	protected:
		virtual void closeEvent(QCloseEvent *e);
		
	protected slots:
		void slotButtonBarClicked(QAbstractButton *button);
		
		void editorKeyChanged(const QString& key, const QVariant& value);
		
	private:
		QSettingsServer *pServer;
		
		QEditConfig *m_editConfig;
		QFormatConfig *m_formatConfig;
		QSnippetEdit *m_snippetConfig;
		QMap<QString, QVariant> m_editKeys;
		
		QPluginManagerConfig *m_pluginConfig;
};

#endif //!_EDYUK_CONFIG_DIALOG_H_
