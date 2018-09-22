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

#include "qmdiperspective.h"

/*!
	\file qmdiperspective.cpp
	\brief Implementation of the qmdiPerspective class.
*/

#include "qmdiworkspace.h"
#include "qmdistatusbar.h"
#include "qmdimainwindow.h"
#include "qmdiclientfactory.h"

#include <QDockWidget>
#include <QToolButton>

/*!
	\ingroup mdi
	
	@{
	
	\class qmdiPerspective
	\brief Abstraction class representing a perspective.
	
	The term of "perspective" has been introduced by Eclipse and I'm not sure
	its "standard" meaning. Anyway, in Edyuk, a perspective is a set of dock
	widgets, menus and toolbars and possibily a "private" workspace.
	
	Perspectives are meant to be provided by plugins and managed by a special
	kind of \ref qmdiMainWindow "main window".
	
	\see qmdiClient
	\see qmdiStatusBar
	\see qmdiMainWindow
*/

/*!
	\enum Affinity
	
	Determines the affinity of a perspective to a given qmdiClient, i.e. how
	strong is its (the perspective's) wish of getting acitve when a given client
	gains focus.
*/

/*!
	\brief Constructor
	\param p Parent main window
*/
qmdiPerspective::qmdiPerspective(qmdiMainWindow *p)
 : QObject(p), pParent(p)
{
	
}

/*!
	\brief Destructor
*/
qmdiPerspective::~qmdiPerspective()
{
	//qDeleteAll(m_tools);
	
	m_docks.clear();
	m_tools.clear();
	m_areas.clear();
}

/*!
	\brief dynamic translation handler
*/
void qmdiPerspective::retranslate()
{
	qmdiClient::retranslate();
}

/*!
	\return the main window managing this perspective
*/
qmdiMainWindow* qmdiPerspective::mainWindow() const
{
	return pParent;
}

/*!
	\brief Show the perspective
	
	\note If the perspective is unmanaged nothing happens; this function is
	equivalent to :
	\code
	if ( perspective->mainWindow() )
		perspective->mainWindow()->setPerspective(perspective);
	\endcode
	
	\see hide()
	\see qmdiMainwindow::setPerspective(qmdiPerspective*)
*/
void qmdiPerspective::show()
{
	if ( !pParent )
		return;
	
	pParent->setPerspective(this);
}

/*!
	\brief Hide the perspective
	
	\note If the perspective is unmanaged or not currently shown, nothing happens.
	
	\see show()
	\see qmdiMainwindow::setPerspective(qmdiPerspective*)
*/
void qmdiPerspective::hide()
{
	if ( !pParent )
		return;
	
	if ( pParent->perspective() == this )
		pParent->setPerspective(0);
	
}

/*!
	\brief Show or hide the perspective according to the boolean parameter.
*/
void qmdiPerspective::setVisible(bool y)
{
	if ( y )
		show();
	else
		hide();
}

/*!
	\brief Add a dock widget to the perspective
	
	\param dw Dock widget to add
	\param n Dock widget name
	\param area Area to place the dock widget in
	
	This function alos adds a tool button in the status bar to show/hide the dock
	widget.
	
	\note The ownership of the dockwidget is given to the parent main window
	
	\see removeDockWidget(QDockWidget*)
*/
void qmdiPerspective::addDockWidget(QDockWidget *dw, const char *n,
									Qt::DockWidgetArea area)
{
	dw->hide();
	dw->setParent(pParent);
	dw->setObjectName(name() + "/" + n);
	
	QToolButton *tool = new QToolButton;
	tool->hide();
	tool->setAutoRaise(true);
	tool->setDefaultAction(dw->toggleViewAction());
	
	m_docks << dw;
	m_areas << area;
	m_tools << tool;
}

/*!
	\brief Remove a dock widget from the perspective
	
	\param dw Dock widget to remove
	
	This function also removes the tool button added by addDockWidget()
	
	\see addDockWidget(QDockWidget*, const char*, Qt::DockWidgetArea)
*/
void qmdiPerspective::removeDockWidget(QDockWidget *dw)
{
	int i = m_docks.indexOf(dw);
	
	if ( i == -1 )
		return;
	
	
	QToolButton *tool = m_tools[i];
	
	if ( tool && tool->isVisible() )
	{	
		qmdiStatusBar *status = 0;
		
		if ( pParent )
			status = pParent->status();
		
		if ( status )
			status->removeButton(tool);
		
		delete tool;
	}
	
	m_docks.removeAt(i);
	m_tools.removeAt(i);
	m_areas.removeAt(i);
	
	pParent->removeDockWidget(dw);
}

/*!
	\brief Create a default empty mdi client.
	
	\see qmdiMainWindow::createEmptyClient()
*/
qmdiClient* qmdiPerspective::createEmptyClient(qmdiClientFactory *f)
{
	return f ? f->createClient(QString()) : 0;
}

/*!
	\brief Set a new main window
	\note this function is virtual so that any perspective can be informed
	of parenting change and act accordingly...
*/
void qmdiPerspective::setMainWindow(qmdiMainWindow *p)
{
	pParent = p;
}

/*!
	\return A list of filter entries suitable for use in a QFileDialog
	
	\see qmdiMainWindow::filter()
*/
QStringList qmdiPerspective::filters() const
{
	return QStringList();
}

/*!
	\brief Check whether a given file can be opened
	\return whether a given file can be opened
*/
bool qmdiPerspective::canOpen(const QString&) const
{
	return false;
}

/*!
	\brief Open a file
	\returns a valid mdi client if opening succeeded, a null pointer otherwise
*/
qmdiClient* qmdiPerspective::open(const QString&)
{
	return 0;
}

/*!
	\return The affinity of the perspective to a given client
	\param c client to test for affinity
	
	This function allows ad hoc perspective switch to take place
	when current client changes in a workspace for instance.
	
	\note The default implementation returns Affinity::None
	\see Affinity
*/
qmdiPerspective::Affinity qmdiPerspective::affinity(qmdiClient *c) const
{
	Q_UNUSED(c)
	
	return None;
}

/*!
	\brief Handler used to fine tune behavior upon perspective show
*/
void qmdiPerspective::showEvent()
{

}

/*!
	\internal
*/
void qmdiPerspective::hideEvent()
{

}

/*! @} */
