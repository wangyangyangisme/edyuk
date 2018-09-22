
#include "qdesignerinternals.h"

#include <QPluginLoader>
#include <QDesignerFormEditorInterface>

#include "pluginmanager_p.h"
#include "qdesigner_integration_p.h"

namespace wrap435
{
	QObjectList pluginInstanciator(QDesignerFormEditorInterface *iface)
	{
		return QPluginLoader::staticInstances() + iface->pluginManager()->instances();
	}
	
	void integrator(QDesignerFormEditorInterface *iface, QObject *p)
	{
		(void) new qdesigner_internal::QDesignerIntegration(iface, p);
	}
	
	static SafetyNet::QDesignerInternals::VersionRegistar registar("4.3.5", pluginInstanciator, integrator);
}
