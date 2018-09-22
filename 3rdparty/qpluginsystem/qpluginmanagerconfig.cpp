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

#include "qpluginmanagerconfig.h"

/*!
	\file qpluginmanagerconfig.cpp
	\brief Implementation of the QPluginManagerConfig class.
*/

#include "qpluginconfig.h"
#include "qpluginmanager.h"
#include "qpluginconfigwidget.h"

#include "ui_pluginconfigentry.h"

#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QListWidget>
#include <QDialogButtonBox>

class QPluginConfigEntry : public QWidget, private Ui::PluginConfigEntry
{
	Q_OBJECT
	
	public:
		QPluginConfigEntry(QPluginConfig::Entry entry, int idx = -1, QWidget *parent = 0)
		 : QWidget(parent), m_idx(idx), m_entry(entry)
		{
			setupUi(this);
			
			setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
			
			m_lib = entry.owner()->library() + "@" + entry.name();
			cbEnabled->setChecked(QPluginManager::instance()->isPluginEnabled(m_lib));
			
			lName->setText(QString("%1").arg(entry.label()));
			lType->setText(QString("<i>(%1)</i>").arg(entry.type()));
			lIcon->setPixmap(QPixmap(entry.icon()));
			lDescription->setText(entry.description());
			
			bSettings->setEnabled(entry.hasSettings());
		}
		
		virtual ~QPluginConfigEntry()
		{
			//qDebug("entry deleted...");
		}
		
		inline int index() const
		{ return m_idx; }
		
		const QPluginConfig::Entry& entry() const
		{ return m_entry; }
		
	signals:
		void showSettings(QPluginConfigEntry *e);
		
	private slots:
		void on_cbEnabled_toggled(bool y)
		{
			QPluginManager::instance()->setPluginEnabled(m_lib, y);
		}
		
		void on_bSettings_clicked()
		{
			emit showSettings(this);
		}
		
	private:
		int m_idx;
		QString m_lib;
		QPluginConfig::Entry m_entry;
};

/*!
	\class QPluginManagerConfig
	\brief Widget allowing configuration of a QPluginManager
*/

/*!
	\brief ctor
*/
QPluginManagerConfig::QPluginManagerConfig(QPluginManager *m, QWidget *p)
 : QStackedWidget(p), m_manager(m ? m : QPluginManager::instance())
{
	m_plugins = new QListWidget(this);
	m_plugins->setResizeMode(QListView::Adjust);
	m_plugins->setUniformItemSizes(false);
	m_plugins->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	addWidget(m_plugins);
	
	m_cfgw = 0;
	m_cfgidx = -1;
	
	#if 0
	QWidget *w = new QWidget(this);
	m_cfgl = new QVBoxLayout(w);
	QDialogButtonBox *bt = new QDialogButtonBox(w);
	bt->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
	bt->addButton(QDialogButtonBox::Cancel);
	m_cfgl->addWidget(bt);
	addWidget(w);
	
	connect(bt	, SIGNAL( accepted() ),
			this, SLOT  ( commit() ) );
	
	connect(bt	, SIGNAL( rejected() ),
			this, SLOT  ( discard() ) );
	#endif
	
	setCurrentIndex(0);
}

/*!
	\brief dtor
*/
QPluginManagerConfig::~QPluginManagerConfig()
{
	
}

/*!

*/
void QPluginManagerConfig::retranslate()
{
	
}

/*!
	\return The modification state of the current plugin config widget or false if none
	
	\see QPluginConfigWidget::isContentModified()
*/
bool QPluginManagerConfig::isContentModified() const
{
	QPluginConfigWidget *w = qobject_cast<QPluginConfigWidget*>(m_cfgw);
	
	return w ? w->isContentModified() : false;
}

/*!
	\brief Save changes in the current plugin config widget, if any
*/
void QPluginManagerConfig::commit()
{
	QPluginConfigWidget *w = qobject_cast<QPluginConfigWidget*>(m_cfgw);
	
	if ( w )
		w->commit();
	
	#if 0
	m_cfgl->removeWidget(m_cfgw);
	delete m_cfgw;
	m_cfgw = 0;
	
	setCurrentIndex(0);
	#endif
}

/*!
	\brief Discard changes in the current plugin config widget, if any
*/
void QPluginManagerConfig::discard()
{
	QPluginConfigWidget *w = qobject_cast<QPluginConfigWidget*>(m_cfgw);
	
	if ( w )
		w->discard();
	
	#if 0
	m_cfgl->removeWidget(m_cfgw);
	delete m_cfgw;
	m_cfgw = 0;
	
	setCurrentIndex(0);
	#endif
}

/*!
	\brief Send fake "close" event to make sure user has a chance to save changes
*/
void QPluginManagerConfig::tryCommit()
{
	QPluginConfigWidget *w = qobject_cast<QPluginConfigWidget*>(m_cfgw);
	
	if ( w )
		w->tryCommit();
	
}

void QPluginManagerConfig::showSettings(QPluginConfigEntry *e)
{
	#if 0
	m_cfgw = e->entry().widget();
	m_cfgl->insertWidget(0, m_cfgw);
	setCurrentIndex(1);
	#else
	
	if ( m_cfgw )
	{
		QPluginConfigWidget *w = qobject_cast<QPluginConfigWidget*>(m_cfgw);
		
		if ( w )
			w->tryCommit();
		
		QWidget *p = w->parentWidget();
		bool self = p == e;
		
		m_cfgw->setParent(0);
		delete m_cfgw;
		m_cfgw = 0;
		
		QListWidgetItem *i = m_plugins->item(m_cfgidx);
		i->setSizeHint(p->sizeHint());
	}
	
	int index = e->index();
	QPluginConfig::Entry entry = e->entry();
	
	QListWidgetItem *i = m_plugins->takeItem(index);
	
	if ( !i )
		return;
	
	e = new QPluginConfigEntry(entry, index, m_plugins);
	
	connect(e	, SIGNAL( showSettings(QPluginConfigEntry*) ),
			this, SLOT  ( showSettings(QPluginConfigEntry*) ) );
	
	if ( index != m_cfgidx )
	{
		m_cfgidx = index;
		
		QLayout *l = e->layout();
		QBoxLayout *b = qobject_cast<QBoxLayout*>(l);
		QGridLayout *g = qobject_cast<QGridLayout*>(l);
		
		if ( b )
		{
			m_cfgw = entry.widget();
			
			b->insertWidget(b->count() - 1, m_cfgw);
			
			//qDebug("box layout...");
		} else if ( g ) {
			m_cfgw = entry.widget();
			
			g->addWidget(m_cfgw, g->rowCount() - 1, 0, 1, g->columnCount());
			
			//qDebug("grid layout...");
		} else if ( l ) {
			m_cfgw = entry.widget();
			
			l->addWidget(m_cfgw);
			//qDebug("layout...");
		}
	} else {
		m_cfgidx = -1;
	}
	
	i->setSizeHint(e->sizeHint());
	m_plugins->insertItem(index, i);
	m_plugins->setItemWidget(i, e);
	m_plugins->update();
	#endif
}

/*!
	\internal
*/
void QPluginManagerConfig::showEvent(QShowEvent *e)
{
	m_plugins->clear();
	
	QHash<QPlugin*, QPluginConfig*>::const_iterator
		it = m_manager->m_plugins.constBegin(),
		end = m_manager->m_plugins.constEnd();
	
	QStringList types, labels, tmp;
	types << tr("(All)");
	
	m_cfgw = 0;
	m_cfgidx = -1;
	
	int idx = 0;
	
	while ( it != end )
	{
		QPluginConfig::Entry entry;
		QList<QPluginConfig::Entry> l = (*it)->entries();
		
		foreach ( entry, l )
		{
			// add filter entry
			if ( !types.contains(entry.type()) )
				types << entry.type();
			
			int n = m_plugins->count();
			QWidget *w = new QPluginConfigEntry(entry, idx, m_plugins);
			
			connect(w	, SIGNAL( showSettings(QPluginConfigEntry*) ),
					this, SLOT  ( showSettings(QPluginConfigEntry*) ) );
			
			QListWidgetItem *i = new QListWidgetItem;
			i->setSizeHint(w->sizeHint());
			
			m_plugins->addItem(i);
			m_plugins->setItemWidget(i, w);
			
			++idx;
		}
		
		++it;
	}
	
	qSort(types);
	
	//m_filter->clear();
	//m_filter->addItems(types);
	
	QWidget::showEvent(e);
}

/*!
	\internal
*/
void QPluginManagerConfig::hideEvent(QHideEvent *e)
{
	QPluginConfigWidget *w = qobject_cast<QPluginConfigWidget*>(m_cfgw);
	
	if ( w )
		w->tryCommit();
	
	QWidget::hideEvent(e);
}

#include "qpluginmanagerconfig.moc"
