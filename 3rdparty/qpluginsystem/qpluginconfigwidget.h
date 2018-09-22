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

#ifndef _QPLUGIN_CONFIG_WIDGET_H_
#define _QPLUGIN_CONFIG_WIDGET_H_

#include "qpluginsystem.h"

/*!
	\file qpluginconfigwidget.h
	\brief Definition of the QPluginConfigWidget class.
*/

#include <QWidget>

#include "qpluginconfig.h"

class QPluginConfigWidget : public QWidget
{
	Q_OBJECT
	
	public:
		static QPluginConfigWidget* create(const QPluginConfig::Entry& entry);
		
		bool isContentModified() const;
		
	public slots:
		void commit();
		void discard();
		
		void tryCommit();
		
	protected:
		QPluginConfigWidget(const QPluginConfig::Entry& e, QWidget *p = 0);
		virtual ~QPluginConfigWidget();
		
	private slots:
		void setContentModified();
		void setProperty(const QString& k, const QString& v);
		
	private:
		bool m_modified;
		QPluginConfig::Entry m_entry;
};

#endif // !_QPLUGIN_CONFIG_WIDGET_H_
