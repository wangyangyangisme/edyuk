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

#include "qpluginconfigwidget.h"

/*!
	\file qpluginconfigwidget.cpp
	\brief Implementation of the QPluginManagerConfig and QPluginConfigWidget classes.
*/

#include "qpluginconfig.h"
#include "qpluginmanager.h"

#include <QMenu>
#include <QLabel>
#include <QAction>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QListWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QContextMenuEvent>

/*!
	\internal
*/
class QPropLabel : public QLabel
{
	public:
		QPropLabel(const QString& s, QWidget *p)
		 : QLabel(s, p)
		{
			setFrameShadow(QFrame::Sunken);
			setFrameStyle(QFrame::StyledPanel);
		}
};

/*!
	\internal
*/
class QPropEdit : public QTextEdit
{
	public:
		QPropEdit(const QString& s, QWidget *p)
		 : QTextEdit(p)
		{
			setPlainText(s);
			setMaximumHeight(80);
			setWordWrapMode(QTextOption::NoWrap);
			setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
		}
};

/*!
	\internal
*/
class QPropText : public QLineEdit
{
	Q_OBJECT
	
	public:
		QPropText(const QString& prop, const QString& val, QWidget *p = 0)
		 : QLineEdit(val, p), m_prop(prop)
		{
			connect(this, SIGNAL( textEdited(QString) ),
					this, SLOT  ( emitValueChanged(QString) ) );
			
		}
		
	signals:
		void contentModified();
		void valueChanged(const QString& p, const QString& v);
		
	private slots:
		void emitValueChanged(const QString& v)
		{
			emit contentModified();
			emit valueChanged(m_prop, v);
		}
		
	private:
		QString m_prop;
};

/*!
	\internal
 */
class QPropOptionList : public QComboBox
{
	Q_OBJECT
	
	public:
		QPropOptionList(const QString& prop, bool sendAsIndex, QWidget *p = 0)
		 : QComboBox(p), m_prop(prop)
		{
			if ( sendAsIndex )
			{
				connect(this, SIGNAL( currentIndexChanged(int) ),
						this, SLOT  ( emitValueChanged(int) ) );
			} else {
				connect(this, SIGNAL( currentIndexChanged(QString) ),
						this, SLOT  ( emitValueChanged(QString) ) );
			}
		}
		
	signals:
		void contentModified();
		void valueChanged(const QString& p, const QString& v);
		
	private slots:
		void emitValueChanged(int idx)
		{
			emit contentModified();
			emit valueChanged(m_prop, QString::number(idx));
		}
		
		void emitValueChanged(const QString& v)
		{
			emit contentModified();
			emit valueChanged(m_prop, v);
		}
		
	private:
		QString m_prop;
};

/*!
	\internal
 */
class QPropEntryList : public QListWidget
{
	Q_OBJECT
	
	public:
		QPropEntryList(const QString& prop, QWidget *p = 0)
		 : QListWidget(p), m_prop(prop)
		{
			
		}
		
	signals:
		void contentModified();
		void valueChanged(const QString& p, const QString& v);
		
	protected:
		virtual void contextMenuEvent(QContextMenuEvent *e)
		{
			QListWidgetItem *i = itemAt(e->pos());
			
			QMenu m;
			m.addAction(tr("Add entry"));
			
			if ( i )
				m.addAction(tr("Remove entry"));
			
			if ( count() )
				m.addAction(tr("Clear"));
			
			QAction *a = m.exec(e->globalPos());
			
			if ( !a )
				return;
			
			QString val, act = a->text();
			
			if ( act == tr("Add entry") )
			{
				QString e = QInputDialog::getText(0, tr("Add new entry"), tr("New entry :"));
				addItem(e);
			} else if ( act == tr("Remove entry") ) {
				takeItem(row(i));
				delete i;
			} else if ( act == tr("Clear") ) {
				clear();
			}
			
			for ( int idx = 0; idx < count(); ++idx )
				val += item(idx)->text() + "\n";
			
			if ( val.count() )
				val.chop(1);
			
			emitValueChanged(val);
		}
		
	private slots:
		void emitValueChanged(const QString& v)
		{
			emit contentModified();
			emit valueChanged(m_prop, v);
		}
		
	private:
		QString m_prop;
};

/*!
	\internal
*/
class QPropRange : public QSpinBox
{
	Q_OBJECT
	
	public:
		QPropRange(const QString& prop, QWidget *p = 0)
		 : QSpinBox(p), m_prop(prop)
		{
			connect(this, SIGNAL( valueChanged(int) ),
					this, SLOT  ( emitValueChanged(int) ) );
			
		}
		
	signals:
		void contentModified();
		void valueChanged(const QString& p, const QString& v);
		
	private slots:
		void emitValueChanged(int idx)
		{
			emit contentModified();
			emit valueChanged(m_prop, QString::number(idx));
		}
		
	private:
		QString m_prop;
};


/*!
	\internal
*/
class QPropBool : public QCheckBox
{
	Q_OBJECT
	
	public:
		QPropBool(const QString& prop, QWidget *p = 0)
		 : QCheckBox(tr("Enabled"), p), m_prop(prop)
		{
			connect(this, SIGNAL( toggled(bool) ),
					this, SLOT  ( emitValueChanged(bool) ) );
			
		}
		
	signals:
		void contentModified();
		void valueChanged(const QString& p, const QString& v);
		
	private slots:
		void emitValueChanged(bool enabled)
		{
			emit contentModified();
			emit valueChanged(m_prop, enabled ? "true" : "false");
		}
		
	private:
		QString m_prop;
};

/*!
	\class QPluginConfigWidget
	\brief Subset for per-component configuration
*/

/*!
	\brief Construct a config widget from XML element
*/
QPluginConfigWidget* QPluginConfigWidget::create(const QPluginConfig::Entry& entry)
{
	QDomElement e;
	QGridLayout *grid;
	
	QPluginConfigWidget *w = new QPluginConfigWidget(entry);
	w->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	
	QVBoxLayout *box = new QVBoxLayout(w);
	box->setMargin(0);
	
	// <settings>
	e = entry.m_elem; //.firstChildElement("Config");
	
	QPluginConfig *c = entry.m_owner;
	
	if ( !e.isNull() )
	{
		QGroupBox *settings = new QGroupBox(tr("Settings"), w);
		settings->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
		
		grid = new QGridLayout(settings);
		
		int row = 0;
		QDomNodeList l = e.elementsByTagName("Key");
		
		for ( int i = 0; i < l.count(); ++i )
		{
			QDomElement k = l.at(i).toElement();
			QDomElement t = k.firstChildElement("Type");
			
			QString prop = k.attribute("id");
			QString ctrl = t.attribute("control");
			QString v = k.firstChildElement("Value").firstChild().toText().data();
			
			v = QPluginConfig::substitute(v);
			
			if ( ctrl.isEmpty() )
				continue;
			
			QWidget *sub = 0;
			
			if ( ctrl == "text" )
			{
				sub = new QPropText(prop, v, settings);
				
			} else if ( ctrl == "bool" ) {
				
				QPropBool *pb = new QPropBool(prop, settings);
				
				pb->setChecked(v != "false");
				
				sub = pb;
			} else if ( ctrl == "number" ) {
				
				QPropRange *pr = new QPropRange(prop, settings);
				
				QDomElement attr = t.firstChildElement();
				
				while ( !attr.isNull() )
				{
					QString at = attr.tagName(),
							va = attr.firstChild().toText().data();
					
					if ( at == "Minimum" )
						pr->setMinimum(va.toInt());
					else if ( at == "Maximum" )
						pr->setMaximum(va.toInt());
					else if ( at == "SingleStep" )
						pr->setSingleStep(va.toInt());
					else if ( at == "Prefix" )
						pr->setPrefix(va);
					else if ( at == "Suffix" )
						pr->setSuffix(va);
					
					attr = attr.nextSiblingElement();
				}
				
				pr->setValue(v.toInt());
				
				sub = pr;
			} else if ( ctrl == "option-list" ) {
				
				bool useIdx = (t.attribute("sendAs") == "index");
				QPropOptionList *pl = new QPropOptionList(prop, useIdx, settings);
				
				QDomNodeList il = t.elementsByTagName("Item");
				
				for ( int j = 0; j < il.count(); ++j )
				{
					QDomElement it = il.at(j).toElement();
					
					pl->addItem(it.firstChild().toText().data().trimmed());
				}
				
				if ( useIdx )
				{
					pl->setCurrentIndex(v.toInt());
				} else {
					pl->setCurrentIndex(pl->findText(v));
				}
				
				sub = pl;
			} else if ( ctrl == "entry-list" ) {
				QPropEntryList *el = new QPropEntryList(prop, settings);
				QStringList entries = v.split("\n");
				
				foreach ( const QString& e, entries )
					el->addItem(e.trimmed());
				
				sub = el;
			} else {
				qWarning("QPluginConfigWidget : unhandled control type %s", qPrintable(ctrl));
			}
			
			if ( !sub )
				continue;
			
			connect(sub	, SIGNAL( contentModified() ),
					w	, SLOT  ( setContentModified() ) );
			
			connect(sub	, SIGNAL( valueChanged(QString, QString) ),
					w	, SLOT  ( setProperty(QString, QString) ) );
			
			grid->addWidget(new QPropLabel(c->tr(k.attribute("label")) + " :", settings),
							row,
							0
							);
			
			grid->addWidget(sub, row, 1);
			
			++row;
		}
		
		box->addWidget(settings);
	}
	// </settings>
	
	// spacer
	//box->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
	
	return w;
}

/*!
	\brief Let the user an opportunity to commit changes
 */
void QPluginConfigWidget::tryCommit()
{
	if ( !isContentModified() )
		return;
	
	int ret = QMessageBox::warning(	0,
									tr("Commit changes?"),
									tr(
										"You have modified some plugin properties.\n"
										"Would you like to commit your changes?"
									),
									QMessageBox::Yes | QMessageBox::No
									);
	
	if ( ret == QMessageBox::Yes )
		commit();
	else
		discard();
}

/*!
	\brief Save modified properties
*/
void QPluginConfigWidget::commit()
{
	m_entry.applyConfigChanges();
	
	m_modified = false;
}

/*!
	\brief Discard all changes so far.
	
	\note This function does not reset the content of
	subcontrols.
*/
void QPluginConfigWidget::discard()
{
	m_entry.discardConfigChanges();
	
	m_modified = false;
}

/*!
	\return Whether the widget has "local modifications"
*/
bool QPluginConfigWidget::isContentModified() const
{
	return m_modified;
}

/*!
	\brief Set the modification state to true
	
	The only way to set the modification state to false is
	to either save() or discard() the changes.
*/
void QPluginConfigWidget::setContentModified()
{
	m_modified = true;
}

/*!
	\brief Set a plugin property
	\param p property identifier
	\param v property value
*/
void QPluginConfigWidget::setProperty(const QString& p, const QString& v)
{
	m_entry.setProperty(p, v);
}

/*!
	\brief ctor
*/
QPluginConfigWidget::QPluginConfigWidget(const QPluginConfig::Entry& entry, QWidget *p)
 : QWidget(p), m_modified(false), m_entry(entry)
{
	
}

/*!
	\brief dtor
*/
QPluginConfigWidget::~QPluginConfigWidget()
{
	
}

#include "qpluginconfigwidget.moc"
