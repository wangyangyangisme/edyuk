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

#ifndef _QBUILD_ENGINE_H_
#define _QBUILD_ENGINE_H_

#include "edyuk.h"

/*!
	\file qbuildengine.h
	\brief Definition of the QBuildEngine class.
*/

#include <QObject>
#include "qmdiclient.h"

#include "qbuilder.h"

#include <QHash>
#include <QPointer>
#include <QStringList>

class QAction;
class QActionGroup;

class QBuildTask;

class EDYUK_EXPORT QBuildEngine : public QObject, public qmdiClient
{
	friend class QBuildTask;
	
	Q_OBJECT
	
	public:
		QBuildEngine(QObject *p = 0);
		virtual ~QBuildEngine();
		
		QString activeSource() const;
		QString activeTarget() const;
		
		bool taskRunning() const;
		
		static QBuildEngine* instance();
		
	public slots:
		void run();
		void abort();
		
		void retranslate();
		
		void addBuilder(QBuilder *b);
		
		void setActiveSource(const QString& source, const QString& backend);
		
	signals:
		void targetListUpdateRequested();
		void mergingRequested(qmdiClient *c, bool on);
		
		void taskStarted();
		void taskAboutToStart();
		void taskFinished();
		
		void filesChanged(const QStringList& f);
		
		void log(const QString& line);
		void message(const QString& fn, int line, const QString& msg);
		
		void buildModeChanged(const QString& mode);
		void execTargetChanged(const QString& target);
		
	private slots:
		void modeChanged(QAction *a);
		void actionTriggered(QAction *a);
		void execTargetChanged(QAction *a);
		void emitTargetListUpdateRequested();
		
		void switchToolbar(bool enabled);
		void commandFailed(QBuilder::Command *cmd, int error);
		
	protected:
		QString selectedMode(QBuilder::Command *c) const;
		
	private:
		struct Remanence
		{
			QString target;
			QStringList modes;
		};
		
		QList<QBuilder*> m_builders;
		QHash<QString, QBuildChain> m_pipelines;
		
		QString m_activeSource,
				m_activePipeline,
				m_activeMode;
		
		QStringList m_availableTargets;
		QList<QBuilder::Output> m_possibleOutputs;
		
		QAction *m_abortTask, *m_execTarget;
		QMenu *m_targetMenu;
		QList<QMenu*> m_modesMenus;
		QActionGroup *m_actionGroup, *m_targetGroup;
		
		QHash<QString, Remanence> m_last;
		QHash<QBuilder::Command*, QBuilder*> m_origins;
		QHash<QAction*, QBuilder::Command*> m_commands;
		
		QPointer<QBuildTask> m_currentTask;
};

#endif // !_QBUILD_ENGINE_H_
