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

#ifndef _QPM_CONFIG_H_
#define _QPM_CONFIG_H_

#include <qglobal.h>

/*!
	\file qpm-config.h
	\brief Globals definitions used by (almost) all other files.
*/

/*!
	\macro QPM_EXPORT
	Allow a cross-platform and sure possibility to link in static or dynamic
	way, depending to what's needed...
*/
#ifdef _QPROJECT_MODEL_BUILD_
	#if (defined(QT_SHARED) || defined(QT_DLL)) && !defined(QT_PLUGIN)
		#define QPM_EXPORT Q_DECL_EXPORT
	#endif
#else
	#define QPM_EXPORT Q_DECL_IMPORT
#endif

#ifndef QPM_EXPORT
	#define QPM_EXPORT
#endif

#endif // _QPM_CONFIG_H_
