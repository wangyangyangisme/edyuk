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

#ifndef _EDYUK_H_
#define _EDYUK_H_

/*!
	\file edyuk.h
*/

/*!
	\macro EDYUK_EXPORT
	
	\brief Required for each class that have to be accessible from plugins.
*/
#ifdef _EDYUK_CORELIB_BUILD_
	#define EDYUK_EXPORT Q_DECL_EXPORT
#else
	#define EDYUK_EXPORT Q_DECL_IMPORT
#endif

#include <QEvent>
#include <QStringList>

#ifdef Q_WS_MAC
#define EDYUK_TRAY_DEFAULT false
#else
#define EDYUK_TRAY_DEFAULT true
#endif

namespace Edyuk
{
	enum Events
	{
		RunTimeTranslation = QEvent::User
	};
	
	enum Reopen
	{
		//remember opened files and projects
		ReopenNone		= -1,
		ReopenAll		= 0,		// all (default)
		ReopenCurrent	= 1		// only reopen currently open files
	};

	enum ReloadPerspective
	{
		// Which perspective to show
		ReloadPerspectiveNone		= 0,
		ReloadPerspectiveLastUsed	= 1
	};

	enum Instance
	{
		// Single or multiple instances
		InstanceSingle		= 0,
		InstanceMultiple	= 1
	};

	enum Language
	{
		// Language of Edyuk
		Untranslated		= 0,
		SystemLocale		= 1,
		LastUsed			= 2
	};
	
	enum AutoSave
	{
		AlwaysAsk			= 0,
		SaveWithoutAsking	= 1,
		DiscardChanges		= 2
	};
	
	EDYUK_EXPORT QString settingsPath();
	
	EDYUK_EXPORT QStringList dataPathes();
	EDYUK_EXPORT QString dataFile(const QString& file);
	
	EDYUK_EXPORT void addDataPath(const QString& p);
	
	EDYUK_EXPORT QString makeAbsolute(const QString& rel, const QString& abs);
	EDYUK_EXPORT QString makeRelative(const QString& src, const QString& ref);
	
	EDYUK_EXPORT QStringList splitArguments(const QString& s);
}

#endif
