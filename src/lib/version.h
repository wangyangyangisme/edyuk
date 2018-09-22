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

#ifndef _VERSION_H_
#define _VERSION_H_

/*!
	\file version.h
	\brief File holding various macros related to versionning
*/

/*!
	Oldest supported version, used by plugin framework to check compatibility of
	a plugin. Typically support is kept between minor versions, unless a major
	change occurred in the plugin sytsem itself.
*/
#define EDYUK_VERSION_MIN 0x010100 //version 1.1.0

/*!
	Edyuk version number, used by plugin framework to check compatibility of
	a plugin.
*/
#define EDYUK_VERSION 0x010100 //version 1.1.0

/*!
	Edyuk version string, put in main window title
*/
#define EDYUK_VERSION_STR "1.1.0"

#endif // _VERSION_H_
