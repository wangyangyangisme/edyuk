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

#ifndef _QPLUGIN_MANAGER_CONFIG_H_
#define _QPLUGIN_MANAGER_CONFIG_H_

#include "qpluginsystem.h"

/*!
	\file qpluginmanagerconfig.h
	\brief Definition of the QPluginManagerConfig class.
*/

#include <QStackedWidget>

class QPluginConfig;
class QPluginManager;
class QPluginConfigEntry;

class QComboBox;
class QListWidget;
class QVBoxLayout;

class QPluginManagerConfig : public QStackedWidget
{
	Q_OBJECT
	
	public:
		QPluginManagerConfig(QPluginManager *m, QWidget *p = 0);
		virtual ~QPluginManagerConfig();
		
		bool isContentModified() const;
		
	public slots:
		void commit();
		void discard();
		
		void tryCommit();
		
		void retranslate();
		
		void showSettings(QPluginConfigEntry *e);
		
	protected:
		virtual void showEvent(QShowEvent *e);
		virtual void hideEvent(QHideEvent *e);
		
	private:
		int m_cfgidx;
		QWidget *m_cfgw;
		QComboBox *m_filter;
		QVBoxLayout *m_cfgl;
		QListWidget *m_plugins;
		
		QPluginManager *m_manager;
};

#endif // !_QPLUGIN_MANAGER_CONFIG_H_
