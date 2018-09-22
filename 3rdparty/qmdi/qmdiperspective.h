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

#ifndef _QMDI_PERSPECTIVE_H_
#define _QMDI_PERSPECTIVE_H_

#include "qmdi.h"

/*!
	\file qmdiperspective.h
	
	\brief Definition of the qmdiPerspective class.
*/

#include <QObject>
#include "qmdiclient.h"

#include <QPair>
#include <QHash>
#include <QStringList>

class QIcon;
class QDockWidget;
class QToolButton;

class qmdiclient;
class qmdiWorkspace;
class qmdiMainWindow;
class qmdiClientFactory;

class QMDI_API qmdiPerspective : public QObject, public qmdiClient
{
	Q_OBJECT
	
	friend class qmdiMainWindow;
	
	public:
		enum Affinity
		{
			None,
			Low,
			Medium,
			High,
			Exclusive
		};
		
		qmdiPerspective(qmdiMainWindow *p);
		virtual ~qmdiPerspective();
		
		virtual void retranslate();
		
		virtual QIcon icon() const = 0;
		virtual QString name() const = 0;
		
		qmdiMainWindow* mainWindow() const;
		
		virtual Affinity affinity(qmdiClient *c) const;
		
		virtual QStringList filters() const;
		virtual qmdiClient* open(const QString& filename);
		virtual bool canOpen(const QString& filename) const;
		
		virtual qmdiClient* createEmptyClient(qmdiClientFactory* f);
		
	public slots:
		void show();
		void hide();
		
		void setVisible(bool y);
		
	protected:
		virtual void setMainWindow(qmdiMainWindow *w);
		
		virtual void showEvent();
		virtual void hideEvent();
		
		void addDockWidget(QDockWidget *dw, const char *n, Qt::DockWidgetArea a);
		void removeDockWidget(QDockWidget *dw);
		
	private:
		qmdiMainWindow *pParent;
		
		QList<QDockWidget*> m_docks;
		QList<QToolButton*> m_tools;
		QList<Qt::DockWidgetArea> m_areas;
};

#endif // _QMDI_PERSPECTIVE_H_
