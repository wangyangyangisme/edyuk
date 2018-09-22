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

#ifndef _QRC_EDIT_H_
#define _QRC_EDIT_H_

#include "plugin.h"

#include "qmdiwidget.h"

class QString;

class QMenu;
class QLabel;
class QAction;
class QLayout;
class QLineEdit;
class QTreeWidget;
class QScrollArea;
class QDomDocument;
class QTreeWidgetItem;

class QRCEdit : public qmdiWidget
{
	Q_OBJECT
	
	public:
		QRCEdit(QWidget *w = 0);
		QRCEdit(const QString& f, QWidget *w = 0);
		virtual ~QRCEdit();
		
		virtual void save();
		virtual void retranslate();
		
		void read(const QString& f);
		void write(const QString& f);
		
		
	private slots:
		void contextMenuRequested(const QPoint& pos);
		void fileChanged(QTreeWidgetItem *p, QTreeWidgetItem *c);
		
		void addResource();
		void remResource();
		void resourcePrefix();
		
		void addFile();
		void remFile();
		
	private:
		enum NodeType
		{
			Null,
			QResource,
			File
		};
		
		enum FileType
		{
			None,
			Text,
			Html,
			Image,
			Ui
		};
		
		void setup();
		
		FileType type(const QString& f);
		
		QDomDocument *pDoc;
		
		QMenu *pMenu;
		
		QAction *aAddFile, *aRemFile,
				*aAddResource, *aRemResource, *aPrefix;
		
		QLayout *pLayout;
		QLabel *pRelative;
		QTreeWidget *pTree;
		QLineEdit *pAlias, *pAbsolute;
		
		QScrollArea *pPreview;
		
		QLabel *labelAbsolute, *labelAlias, *labelPreview;
};

#endif // _QRC_EDIT_H_
