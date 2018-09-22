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

#include "qshortcutdialog.h"

#include "qshortcutmanager.h"

/*!
	\file qshortcutdialog.cpp
	\brief Implementation of the QShortcutDialog class
	
	\see QShortcutDialog
*/

#include <QDebug>
#include <QHash>
#include <QLabel>
#include <QString>
#include <QShortcut>
#include <QLineEdit>
#include <QKeyEvent>
#include <QKeySequence>
#include <QPushButton>
#include <QToolButton>
#include <QTreeWidgetItem>
#include <QDialogButtonBox>
#include <QTreeWidgetItemIterator>

/*!
	\ingroup gui
	@{
	
	\class QShortcutDialog
	\brief A simple dialog used to set shortcuts on run-time.
	
	\note Changes are immediately propagated to the registered actions
	
	\see QShortcutDialog
*/

class ShortcutGetter : public QDialog
{
	public:
		ShortcutGetter(QWidget *p = 0)
		 : QDialog(p)
		{
			setWindowTitle(QShortcutDialog::tr("Shortcut getter"));
			
			QVBoxLayout *vbox = new QVBoxLayout(this);
			vbox->setMargin(2);
			vbox->setSpacing(4);
			
			QLabel *l = new QLabel(this);
			l->setText(QShortcutDialog::tr("Press the key combination\nyou want to assign."));
			vbox->addWidget(l);
			
			leKey = new QLineEdit(this);
			leKey->setReadOnly(true);
			leKey->installEventFilter(this);
			QHBoxLayout *hle = new QHBoxLayout;
			// Clear-Button: otherwise how could Backspace both
			// clear and be a shortcut?
			hle->addWidget(leKey);
			QToolButton *tbClear = new QToolButton;
			tbClear->setIcon(QIcon(":/clear.png"));
			leKey->connect(tbClear, SIGNAL(clicked()), SLOT(clear()));
			hle->addWidget(tbClear);
			hle->setMargin(2);
			hle->setSpacing(4);
			vbox->addLayout(hle);
			
			lDuplicate = new QLabel(QShortcutDialog::tr("<b>This shortcut is already in use.</b>"), this);
			lDuplicate->setVisible(false);
			vbox->addWidget(lDuplicate);
			
			QDialogButtonBox *b = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
			
			connect(b	, SIGNAL( accepted() ),
					this, SLOT  ( accept() ) );
			
			connect(b	, SIGNAL( rejected() ),
					this, SLOT  ( reject() ) );
			
			vbox->addWidget(b);
		}
		
		QString exec(const QString& s, const QStringList& others)
		{
			leKey->setText(s);
			m_others = others;
			
			QString shortcut = s;
			
			if ( QDialog::exec() == QDialog::Accepted )
				shortcut = leKey->text();
			
			// keep old value if aborted
			return shortcut;
		}
		
	protected:
		bool event(QEvent *e)
		{
			QString key;
			QStringList mods;
			QKeyEvent *k = static_cast<QKeyEvent*>(e);
			
			int keys = 0;
			
			// check modifiers pressed
			if ( k->modifiers() & Qt::ControlModifier )
				keys |= Qt::ControlModifier;
			if ( k->modifiers() & Qt::AltModifier )
				keys |= Qt::AltModifier;
			if ( k->modifiers() & Qt::ShiftModifier )
				keys |= Qt::ShiftModifier;
			if ( k->modifiers() & Qt::MetaModifier )
				keys |= Qt::MetaModifier;
			
			switch( k->key() )
			{
				// this keys can't be used
				case Qt::Key_Shift:
				case Qt::Key_Control:
				case Qt::Key_Meta:
				case Qt::Key_Alt:
				case Qt::Key_AltGr:
				case Qt::Key_Super_L:
				case Qt::Key_Super_R:
				case Qt::Key_Menu:
				case Qt::Key_Hyper_L:
				case Qt::Key_Hyper_R:
				case Qt::Key_Help:
				case Qt::Key_Direction_L:
				case Qt::Key_Direction_R:
					break;
					
				default:
					keys |= k->key();
					break;
			}
			
			switch ( e->type() )
			{
				case QEvent::ShortcutOverride :
				{	
					m_key = QKeySequence(keys);
					
					QString ks = m_key.toString();
					
					if ( !ks.endsWith('+') )
						setText();
					
					lDuplicate->setVisible(m_others.contains(ks));
					break;
				}
					
				case QEvent::KeyPress:
				case QEvent::KeyRelease :
					break;
					
				default:
					return QDialog::event(e);
					break;
			}
			
			return true;
		}
		
		bool eventFilter(QObject *o, QEvent *e)
		{
			if (
					e->type() == QEvent::KeyPress
				||
					e->type() == QEvent::KeyRelease
				||
					e->type() == QEvent::ShortcutOverride
				)
				return event(e);
			else
				return QDialog::eventFilter(o, e);
		}
		
		void setText()
		{
			leKey->setText(m_key.toString());
		}
		
	private:
		QLineEdit *leKey;
		QLabel *lDuplicate;
		QKeySequence m_key;
		QStringList m_others;
};

enum NodeType
{
	Null,
	Menu,
	Action
};

QShortcutDialog::QShortcutDialog(QShortcutManager *m, QWidget *p)
 : QDialog(p), pManager(m)
{
	setupUi(this);
}

void QShortcutDialog::retranslate()
{
	retranslateUi(this);
}

void QShortcutDialog::exec()
{
	twShortcuts->clear();
	
	m_used.clear();
	
	QHash<QString, QList<QAction*> >::const_iterator i;
	
	for ( i = pManager->m_actions.constBegin(); i != pManager->m_actions.constEnd(); ++i )
	{
		if ( i->isEmpty() )
			continue;
		
		//qDebug("%s : %s", qPrintable(i.key()), qPrintable(i->at(0)->text()));
		
		QString scxt = i.key();
		scxt = scxt.left(scxt.lastIndexOf('/'));
		QString tcxt = pManager->m_translations.value(scxt);
		
		QStringList cxt = (tcxt.count() ? tcxt : scxt).split("/");
		
		if ( cxt.isEmpty() || i->isEmpty() )
			continue;
		
		QString name = i->at(0)->text(),
				shortcut = i->at(0)->shortcut();
		
		QList<QTreeWidgetItem*> l;
		l = twShortcuts->findItems(cxt.at(0), Qt::MatchExactly, 0);
		
		QTreeWidgetItem *child, *item;
		
		if ( l.count() )
		{
			item = l.at(0);
		} else {
			item = new QTreeWidgetItem(QStringList(cxt.at(0)), Menu);
			twShortcuts->addTopLevelItem(item);
		}
		
		cxt.removeAt(0);
		
		// navigate hierarchy and create it as needed
		
		foreach ( QString s, cxt )
		{
			if ( !item->childCount() )
			{
				child = new QTreeWidgetItem(QStringList(s), Menu);
				item->addChild(child);
				item = child;
				continue;
			}
			
			for ( int j = 0; j < item->childCount(); j++ )
			{
				child = item->child(j);
				
				if ( child->text(0) == s )
				{
					item = child;
					break;
				} else if ( j == (item->childCount() - 1) ) {
					child = new QTreeWidgetItem(QStringList(s), Menu);
					item->addChild(child);
					item = child;
					break;
				}
			}
		}
		
		// create leaf
		
		child = new QTreeWidgetItem(QStringList(name) << shortcut, Action);
		item->addChild(child);
		
		++m_used[shortcut];
	}
	
	twShortcuts->expandAll();
	twShortcuts->resizeColumnToContents(0);
	twShortcuts->resizeColumnToContents(1);
	
	updateAmbiguousList();
	QDialog::exec();
	
	pManager->writeXml();
}

void QShortcutDialog::on_twShortcuts_itemDoubleClicked(QTreeWidgetItem *i,
														int col)
{
	if ( !i || col != 1 )
		return;
	
	if ( i->type() != Action )
		return;
	
	
	QStringList cxt;
	QString name = i->text(0);
	
	QTreeWidgetItem *p = i->parent();
	
	while ( p )
	{
		cxt.prepend(p->text(0));
		
		p = p->parent();
	}
	
	QString ns, ks = i->text(1);
	QStringList ksl = m_used.keys();
	
	ksl.removeAll(ks);
	
	ns = ShortcutGetter().exec(ks, ksl);
	
	if ( ns.isEmpty() || (ns == ks) )
		return;
	
	--m_used[ks];
	++m_used[ns];
	
	pManager->apply(ns, cxt.join("/") + "/" + name);
	i->setText(1, ns);
	
	updateAmbiguousList();
}

/*!
	\brief Determine the shortcuts which are used for more than one
	action, i.e. which are ambiguous.

	If there are ambiguous shortcuts, display the warnings box
	and list the shortcuts.
	If there are no ambiguous shortcuts, the warnings box
	is (or will be) hidden.
*/
void QShortcutDialog::updateAmbiguousList()
{
	/*
		optimized ambiguity tracking strategy :
		
		1) keep track of use of each shortcut (integer count)
		initialized on context tree fill and updated upon
		shortcut assignment. Use QHash for speed
		
		2) conservative ambiguous shortcut list (no clear) :
		
			* O(n) removal of disambiguated entries (where n
			is the number of disambiguated entries)
			
			* O(n) insertion of ambiguated entries (where n
			is the number of ambiguated entries)
		
		instead of :
		
			* O(k*n*log(m)) population of shortcut map (where n
			is the number of nodes in action hierarchy, k the,
			average depth of the hierarchy and m the number of
			different shortcuts)
			
			* O(n) insertion of ambiguated entries (where n
			is the number of different shortcuts)
		
	*/
	
	{
	int i = 0;
	QHash<QString, int>::const_iterator it;
	
	while ( i < lwAmbiguous->count() )
	{
		it = m_used.constFind(lwAmbiguous->item(i)->text());
		
		if ( it == m_used.constEnd() || *it > 1 )
		{
			++i;
		} else {
			delete lwAmbiguous->takeItem(i);
		}
	}
	}
	
	{
	QHash<QString, int>::iterator it = m_used.begin();
	
	while ( it != m_used.end() )
	{
		if ( *it > 1 && it.key().count() )
		{
			// ambiguous shortcut
			lwAmbiguous->addItem(it.key());
			
			++it;
		} else if ( *it ) {
			++it;
		} else {
			// useless entry
			it = m_used.erase(it);
		}
	}
	}
	
	gbAmbiguous->setVisible(lwAmbiguous->count());
	
	/*
	// we depend upon the fact that identical shortcuts will have
	// identical texts in text(1)
	
	typedef QMap<QString,QStringList> ShortCutsMap;
	ShortCutsMap mapShortcuts;

	bool bIsAmbiguous=false;
	QTreeWidgetItemIterator it(twShortcuts);
	while (*it)
	{
		QTreeWidgetItem *item = *it++;
		QString sShortcut = item->text(1);
		if (sShortcut.isEmpty()) continue;

		// determine the 'path' to our item
		QString path;
		while (item)
		{
			if (!path.isEmpty()) path='.'+path;
			path = item->text(0) + path;
			item = item->parent();
		}

		QStringList &lActions = mapShortcuts[sShortcut];
		lActions.append(path);
		if (lActions.size()>1)
			bIsAmbiguous=true;
	}

	gbAmbiguous->setVisible( bIsAmbiguous );
	if (bIsAmbiguous)
	{ // for ambiuous shortcuts: add an entry to the tree widget
		twAmbiguous->clear();
		ShortCutsMap::const_iterator scm_it, scm_end=mapShortcuts.end();
		for (scm_it=mapShortcuts.begin(); scm_it!=scm_end; ++scm_it)
		{
			const QString& shortcut = scm_it.key();
			const QStringList& lActions = scm_it.value();

			if (lActions.size()>1)
			{
				new QTreeWidgetItem(twAmbiguous,
					QStringList() << shortcut << (lActions.join(", ")));
			}
		}
	}
	*/
}

/*! @} */
