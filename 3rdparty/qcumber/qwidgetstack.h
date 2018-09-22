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

#ifndef _QWIDGET_STACK_H_
#define _QWIDGET_STACK_H_

#include "qcumber.h"

/*!
	\file qwidgetstack.h
	\brief Definition of the QWidgetStack class
	
	\see QWidgetStack
*/

#include <QWidget>
#include <QPointer>

class QTabBar;
class QToolBox;
class QComboBox;
class QVBoxLayout;
class QStackedWidget;

class QCUMBER_EXPORT QWidgetStack : public QWidget
{
	Q_OBJECT
	
	public:
		enum Mode
		{
			Tabs,
			ToolBox,
			DropDown
		};
		
		QWidgetStack(int m, QWidget *p = 0);
		virtual ~QWidgetStack();
		
		int mode() const;
		
		int count() const;
		int currentIndex() const;
		
		QStringList labels() const;
		QWidgetList widgets() const;
		
		int indexOf(QWidget *w) const;
		int indexOf(const QString& s) const;
		
		QWidget* takeWidget(int idx);
		QWidget* takeWidget(const QString& s);
		
		QWidget* widget(int idx) const;
		QWidget* widget(const QString& s) const;
		
	public slots:
		void setMode(int m);
		
		void setCurrentIndex(int idx);
		
		void setLabel(QWidget *w, const QString& s);
		void setWidget(const QString& s, QWidget *w);
		
		void addWidget(const QString& s, QWidget *w);
		void insertWidget(int idx, const QString& s, QWidget *w);
		
		void removeWidget(int idx);
		void removeWidget(const QString& s);
		
		void clear();
		
		void showContent();
		void hideContent();
		
	signals:
		void currentIndexChanged(int idx);
		
	protected slots:
		void safeModeChange();
		
	protected:
		virtual void contextMenuEvent(QContextMenuEvent *e);
		
	private:
		void setup();
		
		int m_mode, m_tmp;
		QVBoxLayout *pLayout;
		
		QPointer<QTabBar> pTabs;
		QPointer<QComboBox> pEntries;
		
		QPointer<QToolBox> pToolBox;
		QPointer<QStackedWidget> pStack;
};

#endif // _QWIDGET_STACK_H_
