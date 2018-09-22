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

#include "qdesignerinternals.h"

/*
	here we gotta do method multiplexing based on run-time version of Qt,
	which is way more complicated than a bunch of #ifdefs...
*/

#include <QByteArray>

/*
	utilities to easily register versions
*/
QList<QByteArray>& SafetyNet::QDesignerInternals::versions()
{
	static QList<QByteArray> _version_data;
	return _version_data;
}

QList<SafetyNet::QDesignerInternals::PluginInstanciator>& SafetyNet::QDesignerInternals::instanciators()
{
	static QList<PluginInstanciator> _instanciator_data;
	return _instanciator_data;
}

QList<SafetyNet::QDesignerInternals::Integrator>& SafetyNet::QDesignerInternals::integrators()
{
	static QList<Integrator> _integrator_data;
	return _integrator_data;
}

SafetyNet::QDesignerInternals::VersionRegistar::VersionRegistar(const QByteArray& version, PluginInstanciator pi, Integrator i)
{
	QList<QByteArray>::iterator it = qLowerBound(versions().begin(), versions().end(), version);
	
	versions().insert(it, version);
	
	int idx = it - versions().begin();
	
	integrators().insert(idx, i);
	instanciators().insert(idx, pi);
	
	qDebug("Designer multiplexer : registered handler for %s", version.constData());
}

/*
	the list of availables version is assumed to contain only valid version
	strings and to be properly sorted (by version, version qualifiers do not
	matter).
	
	the target version (v) is assumed to be a valid version string
	
	valid version string in this context boils down to Qt version scheme :
	
	\d\.\d\.\d(-\w+)? : major version '.' minor version '.' patch version and optional version qualifier (tp, beta, rc...)
*/
int nearestVersion(const QByteArray& v, const QList<QByteArray>& availables)
{
	bool inf = false;
	QByteArray trimmed;
	
	if ( v.count() > 5 )
		trimmed = v.left(5);
	
	for ( int i = 0; i < availables.count(); ++i )
	{
		QByteArray lowestHigher = availables.at(i);
		
		if ( v == lowestHigher )
		{
			qDebug("Designer multiplexer : found handler for %s", v.constData());
			return i;
		}
		
		// trick for tech previews, betas and rcs
		if ( trimmed.count() && lowestHigher.count() == 5 && trimmed == lowestHigher )
		{
			qDebug("Designer multiplexer : found near handler for %s", v.constData());
			return i;
		}
		
		if ( lowestHigher.count() > 5 )
			lowestHigher = lowestHigher.left(5);
		
		if ( trimmed.count() )
			inf = lowestHigher > trimmed;
		else
			inf = lowestHigher > v;
		
		if ( inf )
		{
			// if target version unsupported : use highest previous supported one or lowest
			// next supported one if it is the nearest (in terms of minor version)
			QByteArray highestLower;
			
			if ( i )
				highestLower = availables.at(i - 1);
			
			// compare minor version
			if ( highestLower.count() )
			{
				if ( highestLower.at(2) == v.at(2) )
				{
					qDebug("Designer multiplexer : selected %s as nearest handler for %s", highestLower.constData(), v.constData());
					return i - 1;
				}
				
				if ( lowestHigher.at(2) == v.at(2) )
				{
					qDebug("Designer multiplexer : selected %s as nearest handler for %s", lowestHigher.constData(), v.constData());
					return i;
				}
			}
			
			qDebug("Designer multiplexer : selected %s as nearest handler for %s", highestLower.constData(), v.constData());
			return i - 1;
		}
	}
	
	// return latest supported version if target version above...
	return availables.count() - 1;
}

QObjectList SafetyNet::QDesignerInternals::pluginInstances(QDesignerFormEditorInterface *iface)
{
	// version dep : move away
	int idx = nearestVersion(qVersion(), versions());
	
	return instanciators().at(idx)(iface);
}

void SafetyNet::QDesignerInternals::createIntegration(QDesignerFormEditorInterface *iface, QObject *p)
{
	int idx = nearestVersion(qVersion(), versions());
	
	integrators().at(idx)(iface, p);
}
