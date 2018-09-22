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

#ifndef _DESIGNER_INTERNALS_H_
#define _DESIGNER_INTERNALS_H_

#include <QList>
#include <QObject>

class QDesignerFormEditorInterface;

namespace SafetyNet
{
	class QDesignerInternals
	{
		public:
			typedef QObjectList (*PluginInstanciator)(QDesignerFormEditorInterface *);
			typedef void (*Integrator)(QDesignerFormEditorInterface *, QObject *);
			
			class VersionRegistar
			{
				public:
					VersionRegistar(const QByteArray& version, PluginInstanciator pi, Integrator i);
			};
			
			static QObjectList pluginInstances(QDesignerFormEditorInterface *iface);
			
			static void createIntegration(QDesignerFormEditorInterface *iface, QObject *p = 0);
			
			static QList<QByteArray>& versions(); 
			static QList<Integrator>& integrators();
			static QList<PluginInstanciator>& instanciators();
	};
}

#endif //
