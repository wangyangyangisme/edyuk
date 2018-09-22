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

#include "edyukcreatedialog.h"

#include "edyuktemplatemanager.h"

#include "edyukgui.h"
#include "edyukapplication.h"

#include <QIcon>
#include <QFileInfo>
#include <QShowEvent>
#include <QFileDialog>
#include <QMessageBox>

/*!
	\file edyukcreatedialog.cpp
	\brief Implementation of the EdyukCreateDialog class.
	
	\see EdyukCreateDialog
*/

/*!
	\class EdyukCreateDialog
	\brief A dialog that collect templates from plugins and let the user choose
	one.
	
*/

EdyukCreateDialog::EdyukCreateDialog(EdyukTemplateManager *m, QWidget *p)
 : QDialog(p), pTemplates(m)
{
	setupUi(this);
	
	connect(this, SIGNAL( accepted() ),
			this, SLOT  ( creation() ) );
	
}

EdyukCreateDialog::~EdyukCreateDialog()
{
	;
}

EdyukCreateDialog::Filter EdyukCreateDialog::filter() const
{
	return m_filter;
}

void EdyukCreateDialog::setFilter(Filter f)
{
	m_filter = f;
}

void EdyukCreateDialog::showEvent(QShowEvent *e)
{
	if ( !pTemplates )
	{
		e->ignore();
		return;
	}
	
	e->accept();
	
	QStringList cat;
	
	switch ( filter() )
	{
		case Project :
			cat << "projects";
			break;
			
		case File :
			cat << "files";
			break;
			
		case Extra :
			cat << "extras";
			break;
			
		default:
			cat << tr("(All)");
			cat << pTemplates->categories();
			
			//qDebug("%s", qPrintable(cat.join("|")));
			break;
	}
	
	cbFilter->clear();
	cbFilter->addItems(cat);
	cbFilter->setCurrentIndex(0);
	on_cbFilter_currentIndexChanged(cat.at(0));
	cbAddToProject->setChecked(filter() != Project);
	leLocation->setText(QFileInfo(COMPONENT(gui)->activeProject()).path());
}

void EdyukCreateDialog::creation()
{
	if ( !lwTemplates->currentItem() )
		return;
	
	EdyukTemplate t = m_links[ lwTemplates->currentItem() ];
	
	QString n = leLocation->text(),
			pro = COMPONENT(gui)->activeProject();
	
	if ( n.isEmpty() )
	{
		if ( !cbAddToProject->isChecked() )
		{
			QMessageBox::warning(this, "Error", "Please specify a location");
			return (void)exec();
		} else {
			n = QFileInfo(pro).path();
		}
	}
	
	n.replace("\\", "/");
	
	if ( !n.endsWith("/") )
		n += "/";
	
	n += leName->text();
	
	QStringList created_files;
	
	pTemplates->create(t, n, &created_files);
	
	foreach ( const QString& file, created_files )
	{
		int m = file.section(':', 0, 0).toInt();
		QString f = file.section(':', 1);
		
		if ( cbOpen->isChecked() )
		{
			if ( m == Edyuk::File )
				COMPONENT(gui)->fileOpen(f);
			else
				COMPONENT(gui)->projectOpen(f);
			
		}
		
		if ( cbAddToProject->isChecked() )
		{
			//qDebug("adding %s to %s", qPrintable(f), qPrintable(pro));
			COMPONENT(gui)->projectAdd(pro, QStringList(f));
		}
	}
}

void EdyukCreateDialog::on_cbFilter_currentIndexChanged(const QString& filter)
{
	m_links.clear();
	lwTemplates->clear();
	
	foreach ( EdyukTemplate t, pTemplates->templates() )
	{
		if ( filter == tr("(All)") || filter == t.category )
		{
			QListWidgetItem *i = new QListWidgetItem(t.name);
			i->setIcon(QIcon(t.icon));
			
			m_links[i] = t;
			
			lwTemplates->addItem(i);
		}
	}
	
	lwTemplates->setCurrentRow(0);
}

void EdyukCreateDialog::on_lwTemplates_itemClicked(QListWidgetItem *i)
{
	if ( !i )
		return;
	
	QHash<QListWidgetItem*, EdyukTemplate>::const_iterator it = m_links.find(i);
	
	if ( it == m_links.end() )
		return;
	
	EdyukTemplate t = *it;
	
	leType->setText(t.category);
	eDescription->setText(t.description);
}

void EdyukCreateDialog::on_tbLocation_clicked()
{
	QString home = QFileInfo(COMPONENT(gui)->activeProject()).path();
	
	QString dir = QFileDialog::getExistingDirectory(this,
													tr(""),
													home);
	leLocation->setText(dir);
}
