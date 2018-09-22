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

#ifndef _QMAKE_SETTINGS_H_
#define _QMAKE_SETTINGS_H_

/*!
	\file qmakesettings.h
	
	\brief Definition of QMakeSettings
*/

#include <QDialog>

#include "ui_projectsettings.h"

class QProject;
class QProjectProxyModel;

class QMakeSettings : public QDialog, private Ui::ProjectSettings
{
	Q_OBJECT
	
	public:
		QMakeSettings(QWidget *p = 0);
		virtual ~QMakeSettings();
		
	public slots:
		void setProject(QProject *p);
		
	private slots:
		void on_leName_editingFinished();
		void on_leAuthor_editingFinished();
		
		void on_leOutputName_editingFinished();
		void on_leOutputPath_editingFinished();
		void on_tbOutputPath_released();
		void on_leBuildPath_editingFinished();
		
		void on_rbDebug_toggled(bool on);
		void on_rbRelease_toggled(bool on);
		void on_rbDebugRelease_toggled(bool on);
		
		void on_cbWarnings_toggled(bool on);
		void on_cbExceptions_toggled(bool on);
		void on_cbRTTI_toggled(bool on);
		void on_cbSTL_toggled(bool on);
		
		void on_cbBuildAll_toggled(bool on);
		void on_cbOrdered_toggled(bool on);
		void on_cbStaticLib_toggled(bool on);
		void on_cbDLL_toggled(bool on);
		void on_cbPlugin_toggled(bool on);
		void on_cbDesigner_toggled(bool on);
		
		void on_gbVersion_toggled(bool on);
		
		void on_sbMajor_valueChanged(int val);
		void on_sbMinor_valueChanged(int val);
		void on_sbRelease_valueChanged(int val);
		void on_sbBuild_valueChanged(int val);
		
		void on_cbTemplate_currentIndexChanged(const QString& tpl);
		void on_cbLanguage_currentIndexChanged(const QString& lng);
		
		void on_lwQtModules_itemChanged(QListWidgetItem *i);
		
		void on_lwDefines_customContextMenuRequested(const QPoint& p);
		void on_lwIncludes_customContextMenuRequested(const QPoint& p);
		void on_lwLibraries_customContextMenuRequested(const QPoint& p);
		
	private:
		QProject *m_project;
		QProjectProxyModel *m_proxy;
};

#endif // !_QMAKE_SETTINGS_H_
