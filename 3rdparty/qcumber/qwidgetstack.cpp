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

#include "qwidgetstack.h"

/*!
	\file qwidgetstack.cpp
	\brief Implementation of the QWidgetStack class
	
	\see QWidgetStack
*/

#include <QMenu>
#include <QTimer>
#include <QAction>
#include <QTabBar>
#include <QToolBox>
#include <QComboBox>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QContextMenuEvent>

/*!
	\ingroup gui
	@{
	
	\class QWidgetStack
	\brief A simple wrapper around QStackWidget and QComboBox
	
	\see QWidgetStack
*/

QWidgetStack::QWidgetStack(int m, QWidget *p)
 : QWidget(p), m_mode(m)
{
	pLayout = 0;
	
	setup();
}

QWidgetStack::~QWidgetStack()
{
	
}

int QWidgetStack::mode() const
{
	return m_mode;
}

void QWidgetStack::setMode(int m)
{
	if ( m_mode == m )
		return;
	
	// remove widgets from container
	QList<QWidget*> lw;
	int idx = currentIndex();
	QStringList lbl = labels();
	
	while ( count() )
	{
		lw << takeWidget(0);
	}
	
	// change display mode
	m_mode = m;
	
	// update container
	setup();
	
	// reinsert widgets
	for ( int i = 0; i < lbl.count(); ++i )
	{
		addWidget(lbl.at(i), lw.at(i));
	}
	
	setCurrentIndex(idx);
}

void QWidgetStack::setup()
{
	delete pTabs;
	delete pStack;
	delete pEntries;
	delete pToolBox;
	
	delete pLayout;
	
	pLayout = new QVBoxLayout(this);
	pLayout->setMargin(0);
	pLayout->setSpacing(0);
	
	if ( m_mode == ToolBox )
	{
		pToolBox = new QToolBox(this);
		
		connect(pToolBox, SIGNAL( currentChanged(int) ),
				this	, SIGNAL( currentIndexChanged(int) ) );
		
		pLayout->addWidget(pToolBox);
	} else {
		pStack = new QStackedWidget(this);
		
		if ( m_mode == DropDown )
		{
			pEntries = new QComboBox(this);
			
			pLayout->addWidget(pEntries);
			
			connect(pEntries, SIGNAL( currentIndexChanged(int) ),
					this	, SIGNAL( currentIndexChanged(int) ) );
			
			connect(pEntries, SIGNAL( currentIndexChanged(int) ),
					pStack	, SLOT  ( setCurrentIndex(int) ) );
			
			pEntries->hide();
		} else if ( m_mode == Tabs ) {
			pTabs = new QTabBar(this);
			pLayout->addWidget(pTabs);
			
			connect(pTabs	, SIGNAL( currentChanged(int) ),
					this	, SIGNAL( currentIndexChanged(int) ) );
			
			connect(pTabs	, SIGNAL( currentChanged(int) ),
					pStack	, SLOT  ( setCurrentIndex(int) ) );
			
			pTabs->hide();
		} else {
			qWarning("QWidgetStack : Unsupported display mode");
		}
		
		pLayout->addWidget(pStack);
	}
}

/*!
	\brief Show the active content widget
	\note this does NOT work in ToolBox mode...
*/
void QWidgetStack::showContent()
{
	if ( pStack )
	{
		bool ok = pStack->count() <= 1;
		
		if ( pEntries && ok )
			pEntries->hide();
		else if ( pTabs && ok )
			pTabs->hide();
		
		pStack->show();
	}
}


/*!
	\brief Hide the active content widget
	\note this does NOT work in ToolBox mode...
*/
void QWidgetStack::hideContent()
{
	if ( pStack )
	{
		if ( pEntries )
			pEntries->show();
		else if ( pTabs )
			pTabs->show();
		
		pStack->hide();
	}
}

/*!

*/
int QWidgetStack::count() const
{
	if ( pEntries )
	{
		return qMin(pEntries->count(), pStack->count());
	} else if ( pTabs ) {
		return qMin(pTabs->count(), pStack->count());
	} else if ( pToolBox ) {
		return pToolBox->count();
	}
	
	return 0;
}

int QWidgetStack::currentIndex() const
{
	if ( pEntries )
	{
		return pEntries->currentIndex();
	} else if ( pTabs ) {
		return pTabs->currentIndex();
	} else if ( pToolBox ) {
		return pToolBox->currentIndex();
	}
	
	return -1;
}

void QWidgetStack::setCurrentIndex(int idx)
{
	if ( pEntries )
	{
		pEntries->setCurrentIndex(idx);
	} else if ( pTabs ) {
		pTabs->setCurrentIndex(idx);
	} else if ( pToolBox ) {
		pToolBox->setCurrentIndex(idx);
	}
}

QStringList QWidgetStack::labels() const
{
	QStringList l;
	
	if ( pEntries )
	{
		for ( int i = 0; i < pEntries->count(); i++ )
			l << pEntries->itemText(i);
	} else if ( pTabs ) {
		for ( int i = 0; i < pTabs->count(); i++ )
			l << pTabs->tabText(i);
	} else if ( pToolBox ) {
		for ( int i = 0; i < pToolBox->count(); i++ )
			l << pToolBox->itemText(i);
	}
	
	return l;
}

QWidgetList QWidgetStack::widgets() const
{
	QWidgetList l;
	
	if ( pStack )
	{
		for ( int i = 0; i < pStack->count(); i++ )
			l << pStack->widget(i);
	} else if ( pToolBox ) {
		for ( int i = 0; i < pToolBox->count(); i++ )
			l << pToolBox->widget(i);
	}
	
	return l;
}

int QWidgetStack::indexOf(const QString& s) const
{
	if ( pStack )
	{
		if ( pEntries )
		{
			return pEntries->findText(s);
		} else if ( pTabs ) {
			for ( int i = 0; i < pTabs->count(); ++i )
				if ( pTabs->tabText(i) == s )
					return i;
		}
	} else if ( pToolBox ) {
		for ( int i = 0; i < pToolBox->count(); ++i )
			if ( pToolBox->itemText(i) == s )
				return i;
	}
	
	return -1;
}

int QWidgetStack::indexOf(QWidget *w) const
{
	if ( pStack )
	{
		return pStack->indexOf(w);
	} else if ( pToolBox ) {
		for ( int i = 0; i < pToolBox->count(); ++i )
			if ( pToolBox->widget(i) == w )
				return i;
	}
	
	return -1;
}

QWidget* QWidgetStack::widget(int idx) const
{
	if ( pStack )
	{
		return (idx >= 0 && idx < pStack->count()) ? pStack->widget(idx) : 0;
	} else if ( pToolBox ) {
		return (idx >= 0 && idx < pToolBox->count()) ? pToolBox->widget(idx) : 0;
	}
	
	return 0;
}

QWidget* QWidgetStack::widget(const QString& s) const
{
	return widget(indexOf(s));
}

QWidget* QWidgetStack::takeWidget(int idx)
{
	if ( idx == -1 )
		return 0;
	
	QWidget *widget = 0;
	
	if ( pStack )
	{
		widget = pStack->widget(idx);
		pStack->removeWidget(widget);
		
		if ( pEntries )
		{
			pEntries->removeItem(idx);
			
			if ( count() <= 1 )
				pEntries->hide();
		} else if ( pTabs ) {
			pTabs->removeTab(idx);
			
			if ( count() <= 1 )
				pTabs->hide();
		}
		
	} else if ( pToolBox ) {
		widget = pToolBox->widget(idx);
		pToolBox->removeItem(idx);
	}
	
	if ( widget )
	{
		widget->setParent(0);
		widget->hide();
	}
	
	return widget;
}

QWidget* QWidgetStack::takeWidget(const QString& s)
{
	return takeWidget(indexOf(s));
}

void QWidgetStack::setLabel(QWidget *w, const QString& s)
{
	int index = indexOf(w);
	
	if ( index == -1 )
		return;
	
	if ( pEntries )
		pEntries->setItemText(index, s);
	else if ( pTabs )
		pTabs->setTabText(index, s);
	else if ( pToolBox )
		pToolBox->setItemText(index, s);
	
}

void QWidgetStack::setWidget(const QString& s, QWidget *w)
{
	int index = indexOf(s);
	
	if ( index == -1 )
		return;
	
	removeWidget(index);
	insertWidget(index, s, w);
}

void QWidgetStack::insertWidget(int idx, const QString& s, QWidget *w)
{
	//if ( labels().contains(s) )
	//	return;
	
	if ( pStack )
	{
		pStack->insertWidget(idx, w);
		
		if ( pEntries )
		{
			pEntries->insertItem(idx, s);
			
			if ( count() > 1 )
				pEntries->show();
		} else if ( pTabs ) {
			pTabs->insertTab(idx, s);
			
			if ( count() > 1 )
				pTabs->show();
		}
	} else if ( pToolBox ) {
		pToolBox->insertItem(idx, w, s);
	}
}

void QWidgetStack::addWidget(const QString& s, QWidget *w)
{
	insertWidget(count(), s, w);
}

void QWidgetStack::removeWidget(int idx)
{
	delete takeWidget(idx);
}

void QWidgetStack::removeWidget(const QString& s)
{
	delete takeWidget(s);
}

void QWidgetStack::clear()
{
	while ( count() )
		removeWidget(0);
	
}

void QWidgetStack::contextMenuEvent(QContextMenuEvent *e)
{
	e->accept();
	
	QStringList lbls = QStringList()
		<< tr("Tabs")
		<< tr("Toolbox")
		<< tr("Combobox");
	
	QMenu m(tr("Display mode"));
	m.addAction(lbls.at(0));
	m.addAction(lbls.at(1));
	m.addAction(lbls.at(2));
	
	QAction *a = m.exec(e->globalPos());
	
	if ( a )
	{
		QString s = a->text();
		m_tmp = lbls.indexOf(s);
		
		QTimer::singleShot(100, this, SLOT( safeModeChange() ));
	}
}

void QWidgetStack::safeModeChange()
{
	if ( m_tmp == -1 )
		return;
	
	setMode(m_tmp);
	
	m_tmp = -1;
}
