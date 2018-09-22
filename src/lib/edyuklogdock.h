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

#ifndef _EDYUK_LOG_DOCK_H_
#define _EDYUK_LOG_DOCK_H_

#include "edyuk.h"

/*!
	\file edyuklogdock.h
	\brief Definition of the EdyuLogDock class
	
	\see EdyuLogDock
*/

#include <QDockWidget>

class QTabBar;
class QTextEdit;
class QListWidget;
class EdyukLogFrame;
class qmdiMainWindow;
class QListWidgetItem;

class EDYUK_EXPORT EdyukLogDock : public QDockWidget
{
	friend class EdyukApplication;
	
	Q_OBJECT
	
	public:
		enum DataFields
		{
			File	= Qt::UserRole + 1,
			Line	= File + 1
		};
		
		EdyukLogDock(qmdiMainWindow *p);
		virtual ~EdyukLogDock();
		
		virtual void retranslate();
		
		virtual QSize sizeHint() const;
		virtual QSize minimumSizeHint() const;
		
	public slots:
		void clear();
		void log(const QString& line);
		void message(const QString& file, int line, const QString& msg);
		
		void edyukLog(const QString& line);
		
		void addExtraWidget(QWidget *w);
		void removeExtraWidget(QWidget *w);
		
	protected:
		virtual bool eventFilter(QObject *o, QEvent *e);
		
		virtual void paintEvent(QPaintEvent *e);
		virtual void resizeEvent(QResizeEvent *e);
		
	protected slots:
		void messageActivated(QListWidgetItem *i);
		
	private:
		qmdiMainWindow *m_parent;
		
		QTabBar *m_tabs;
		EdyukLogFrame *m_frame;
		
		QListWidget *m_buildMessages;
		QTextEdit *m_buildLog, *m_edyukLog;
};

#endif // ! _EDYUK_LOG_DOCK_H_
