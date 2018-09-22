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

#ifndef _QMDI_H_
#define _QMDI_H_

/*!
	\macro QMDI_API
	Allow a cross-platform and sure possibility to link in static or dynamic
	way, depending to what's needed...
*/
#ifdef _QMDI_BUILD_
	#if defined(QT_SHARED) || defined(QT_DLL)
		#define QMDI_API Q_DECL_EXPORT
	#else
		#define QMDI_API
	#endif
#else
	#define QMDI_API Q_DECL_IMPORT
#endif

/*!
	\defgroup mdi MDI layer
*/

class QObject;
class QString;

class QMenu;
class QAction;
class QWidget;
class QTabBar;
class QToolBar;
class QMenuBar;
class QWorkspace;
class QToolButton;
class QMainWindow;

#include <QList>
#include <QHash>
#include <QVector>
#include <QPointer>

#endif // _QMDI_H_
