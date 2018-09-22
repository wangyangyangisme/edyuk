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

#include "qrcedit.h"

#include "qmdiserver.h"

#include <QtUiTools>

#include <QMenu>
#include <QFile>
#include <QLabel>
#include <QAction>
#include <QLayout>
#include <QDomText>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QTextStream>
#include <QFileDialog>
#include <QTreeWidget>
#include <QMessageBox>
#include <QScrollArea>
#include <QDomDocument>
#include <QInputDialog>
#include <QImageReader>

QRCEdit::QRCEdit(QWidget *w)
 : qmdiWidget(w)
{
	setup();
}

QRCEdit::QRCEdit(const QString& f, QWidget *w)
 : qmdiWidget(w)
{
	setup();
	
	read(f);
}

QRCEdit::~QRCEdit()
{
	delete pDoc;
}

void QRCEdit::setup()
{
	pDoc = new QDomDocument("QRC");
	
	pMenu = new QMenu(this);
	
	aAddResource = new QAction(QIcon(":/add.png"), tr("Add resource"), this);
	connect(aAddResource, SIGNAL( triggered() ),
			this		, SLOT  ( addResource() ) );
	
	aRemResource = new QAction(QIcon(":/remove.png"), tr("Remove resource"), this);
	connect(aRemResource, SIGNAL( triggered() ),
			this		, SLOT  ( remResource() ) );
	
	aPrefix = new QAction(QIcon(":/edit.png"), tr("Resource prefix"), this);
	connect(aPrefix	, SIGNAL( triggered() ),
			this	, SLOT  ( resourcePrefix() ) );
	
	aAddFile = new QAction(QIcon(":/add.png"), tr("Add resource"), this);
	connect(aAddFile, SIGNAL( triggered() ),
			this	, SLOT  ( addFile() ) );
	
	aRemFile = new QAction(QIcon(":/remove.png"), tr("Add resource"), this);
	connect(aRemFile, SIGNAL( triggered() ),
			this	, SLOT  ( remFile() ) );
	
	
	menus["Edit"]->addAction(aAddResource);
	menus["Edit"]->addAction(aRemResource);
	menus["Edit"]->addAction(aPrefix);
	menus["Edit"]->addSeparator();
	menus["Edit"]->addAction(aAddFile);
	menus["Edit"]->addAction(aRemFile);
	
	toolbars["Edit"]->addAction(aAddResource);
	toolbars["Edit"]->addAction(aRemResource);
	toolbars["Edit"]->addAction(aPrefix);
	toolbars["Edit"]->addSeparator();
	toolbars["Edit"]->addAction(aAddFile);
	toolbars["Edit"]->addAction(aRemFile);
	
	QGridLayout *grid = new QGridLayout(this);
	
	pTree = new QTreeWidget(this);
	pTree->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(pTree, SIGNAL( currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*) ),
			this , SLOT  ( fileChanged(QTreeWidgetItem*,QTreeWidgetItem*) ) );
	
	connect(pTree, SIGNAL( customContextMenuRequested(const QPoint&) ),
			this , SLOT  ( contextMenuRequested(const QPoint&) ) );
	
	grid->addWidget(pTree, 0, 0, 4, 1);
	
	pRelative = new QLabel(this);
	pRelative->setText("<h4></h4>");
	pRelative->setFrameShape(QFrame::StyledPanel);
	pRelative->setFrameShadow(QFrame::Raised);
	grid->addWidget(pRelative, 0, 1, 1, 2);
	
	labelAbsolute = new QLabel(this);
	grid->addWidget(labelAbsolute, 1, 1, 1, 1);
	
	pAbsolute = new QLineEdit(this);
	pAbsolute->setReadOnly(true);
	grid->addWidget(pAbsolute, 1, 2, 1, 1);
	
	labelAlias = new QLabel(this);
	grid->addWidget(labelAlias, 2, 1, 1, 1);
	
	pAlias = new QLineEdit(this);
	pAlias->setReadOnly(true);
	grid->addWidget(pAlias, 2, 2, 1, 1);
	
	QFrame *frame = new QFrame(this);
	frame->setFrameShape(QFrame::StyledPanel);
	frame->setFrameShadow(QFrame::Raised);
	
	pLayout = new QVBoxLayout(frame);
	pLayout->setSpacing(2);
	pLayout->setMargin(0);
	
	labelPreview = new QLabel(frame);
	pLayout->addWidget(labelPreview);
	
	pPreview = new QScrollArea(frame);
	pPreview->setWidgetResizable(true);
	pLayout->addWidget(pPreview);
	
	grid->addWidget(frame, 3, 1, 1, 2);
	
	setLayout(grid);
	
	retranslate();
}

void QRCEdit::retranslate()
{
	pTree->setHeaderLabels( QStringList(tr("Resource content")) );
	
	labelAbsolute->setText(tr("Absolute :"));
	labelAlias->setText(tr("Alias :"));
	
	labelPreview->setText(tr("<h5>Preview :</h5>"));
	
	aAddResource->setText(tr("Add resource"));
	aRemResource->setText(tr("Remove resource"));
	aPrefix->setText(tr("Resource prefix"));
	
	aAddFile->setText(tr("Add file"));
	aRemFile->setText(tr("Remove file"));
	
	menus.setTranslation("Edit", tr("Edit"));
	toolbars.setTranslation("Edit", tr("Edit"));
}

void QRCEdit::save()
{
	if ( fileName().isEmpty() )
	{
		if ( server() )
			server()->saveClientAs(this);
		
		return;
	}
	
	write(fileName());
}

void QRCEdit::read(const QString& f)
{
	setFileName(f);
	
	QFile file(f);
	
	if ( !file.open(QFile::ReadOnly | QFile::Text) )
		return (void)QMessageBox::warning(0, "Default plugin",
										tr("Unable to read %1").arg(f));
	
	pDoc->setContent(&file);
	
	QDomNodeList l = pDoc->elementsByTagName("qresource");
	
	for ( int i = 0; i < l.size(); i++ )
	{
		QDomElement e = l.at(i).toElement();
		
		if ( e.isNull() )
			continue;
		
		QTreeWidgetItem *res = new QTreeWidgetItem(QStringList(
														e.attribute("prefix")
														),
													QResource);
		pTree->addTopLevelItem(res);
		
		QDomElement elem = e.firstChildElement("file");
		
		while ( !elem.isNull() )
		{
			QDomText t = elem.firstChild().toText();
			QString fileName = t.data();
			
			new QTreeWidgetItem(res, QStringList(fileName), File);
			
			elem = elem.nextSiblingElement("file");
		}
	}
	
	setContentModified(false);
}

void QRCEdit::write(const QString& f)
{
	QFile file(f);
	
	if ( !file.open(QFile::WriteOnly | QFile::Text) )
		return (void)QMessageBox::warning(0, "Default plugin",
										tr("Unable to write %1").arg(f));
	
	//qDebug() << "saving " << f;
	
	QTextStream out(&file);
	
	out << pDoc->toString(4).replace("    ", "\t");
	
	setContentModified(false);
}

void QRCEdit::contextMenuRequested(const QPoint& pos)
{
	pMenu->clear();
	pMenu->addAction(aAddResource);
	
	QTreeWidgetItem *i = pTree->itemAt(pos);
	
	if ( i )
	{
		if ( i->type() == QResource )
		{
			pMenu->addAction(aRemResource);
			pMenu->addAction(aPrefix);
			pMenu->addSeparator();
			pMenu->addAction(aAddFile);
		} else if ( i->type() == File ) {
			pMenu->addSeparator();
			pMenu->addAction(aRemFile);	
		}
	}
	
	pMenu->exec( pTree->viewport()->mapToGlobal(pos) );
}

void QRCEdit::addResource()
{
	QString prefix = QInputDialog::getText(	this,
											tr("Creating new resource"),
											tr("Enter the resource prefix :"));
	
	if ( prefix.isNull() )
		return;
	
	QDomElement res = pDoc->createElement("qresource");
	res.setAttribute("prefix", prefix);
	pDoc->documentElement().appendChild(res);
	
	new QTreeWidgetItem(pTree, QStringList(prefix), QResource);
	
	setContentModified(true);
}

void QRCEdit::remResource()
{
	QTreeWidgetItem *i = pTree->currentItem();
	
	if ( !i )
		return;
	
	QString prefix = i->text(0);
	
	QDomNodeList l = pDoc->elementsByTagName("qresource");
	
	for ( int i = 0; i < l.size(); i++ )
	{
		QDomElement e = l.at(i).toElement();
		
		if ( e.isNull() )
			continue;
		
		if ( e.attribute("prefix") == prefix )
		{
			pDoc->removeChild(e);
			delete pTree->takeTopLevelItem(i);
			break;
		}
	}
	
	setContentModified(true);
}

void QRCEdit::resourcePrefix()
{
	QTreeWidgetItem *i = pTree->currentItem();
	
	if ( !i )
		return;
	
	QString old = i->text(0),
			pre = QInputDialog::getText(this,
										tr("Setting resource prefix"),
										tr("Enter the new resource prefix :"));
	
	if ( pre.isNull() )
		return;
	
	QDomNodeList l = pDoc->elementsByTagName("qresource");
	
	for ( int i = 0; i < l.size(); i++ )
	{
		QDomElement e = l.at(i).toElement();
		
		if ( e.isNull() )
			continue;
		
		if ( e.attribute("prefix") == old )
		{
			e.setAttribute("prefix", pre);
			break;
		}
	}
	
	setContentModified(true);
}

void QRCEdit::addFile()
{
	QTreeWidgetItem *p = pTree->currentItem();
	
	if ( !p )
		return;
	
	QStringList files = QFileDialog::getOpenFileNames(	this,
											tr("Ading file(s) to resource"),
											"",
											"All files (*)");
	
	if ( files.isEmpty() )
		return;
	
	QString prefix = p->text(0);
	
	QDomNodeList l = pDoc->elementsByTagName("qresource");
	
	for ( int i = 0; i < l.size(); i++ )
	{
		QDomElement e = l.at(i).toElement();
		
		if ( e.isNull() )
			continue;
		
		if ( e.attribute("prefix") == prefix )
		{
			foreach ( QString file, files )
			{
				file = QDir(QFileInfo(fileName()).path()).relativeFilePath(file);
				
				QDomElement elem = pDoc->createElement("file");
				e.appendChild(elem);
				
				QDomText text = pDoc->createTextNode(file);
				elem.appendChild(text);
				
				new QTreeWidgetItem(p, QStringList(file), File);
			}
			
			break;
		}
	}
	
	setContentModified(true);
}

void QRCEdit::remFile()
{
	QTreeWidgetItem *i = pTree->currentItem(),
					*p = i ? i->parent() : 0;
	
	if ( !i || !p )
		return;
	
	QString prefix = p->text(0),
			file = i->text(0);
	
	QDomNodeList l = pDoc->elementsByTagName("qresource");
	
	for ( int i = 0; i < l.size(); i++ )
	{
		QDomElement e = l.at(i).toElement();
		
		if ( e.isNull() )
			continue;
		
		if ( e.attribute("prefix") == prefix )
		{
			QDomNodeList f = e.elementsByTagName("file");
			
			for ( int j = 0; j < f.size(); j++ )
			{
				QDomElement elem = f.at(j).toElement();
				
				if ( elem.isNull() )
					continue;
				
				QDomText t = elem.firstChild().toText();
				
				if ( t.data() == file )
				{
					e.removeChild(elem);
					delete p->takeChild(i);
					break;
				}
			}
		}
	}
	
	setContentModified(true);
}

void QRCEdit::fileChanged(QTreeWidgetItem *n, QTreeWidgetItem *p)
{
	QString r, a;
	
	if ( p )
	{
		pRelative->setText("<h4></h4>");
		pAbsolute->setText("");
		
		pAlias->setText("");
		
		delete pPreview->takeWidget();
	}
	
	if ( n && n->type() == File )
	{
		r = n->text(0);
		a = QDir(QFileInfo(fileName()).path()).absoluteFilePath(r);
		
		pRelative->setText(QString("<h4>%1</h4>").arg(r));
		pAbsolute->setText(a);
		
		//pAlias->setText();
		
		
		QFile file(a);
		FileType type = this->type(a);
		QFile::OpenMode m = QFile::ReadOnly;
		
		if ( type == Text || type == Html )
			m |= QFile::Text;
		
		if ( !file.open(m) || type == None )
			return;
		
		QString data = QTextStream(&file).readAll();
		
		QLabel *label;
		QTextEdit *edit;
		QWidget *pCur = 0;
		
		QFont f = 
		#ifdef Q_OS_WIN
		QFont("Monospace", 10)
		#else
		QFont("Courier New", 9)
		#endif
		;
		
		switch ( type )
		{
			case Text :
				edit = new QTextEdit(pPreview);
				edit->document()->setDefaultFont(f);
				edit->setFrameStyle(QFrame::NoFrame);
				edit->setLineWrapMode(QTextEdit::NoWrap);
				edit->setWordWrapMode(QTextOption::NoWrap);
				edit->setPlainText(data);
				edit->setReadOnly(true);
				
				pCur = (QWidget*)edit;
				break;
				
			case Html :
				edit = new QTextEdit(pPreview);
				edit->setHtml(data);
				
				pCur = (QWidget*)edit;
				break;
				
			case Image :
				label = new QLabel(pPreview);
				label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
				label->setPixmap( QPixmap(a) );
				
				pCur = (QWidget*)label;
				break;
				
			case Ui :
				pCur = QUiLoader().load(&file, pPreview);
				break;
				
			default:
				break;
		}
		
		if ( pCur )
		{
			pPreview->setWidget(pCur);
			pCur->show();
		}
		
		aRemResource->setEnabled(false);
		aPrefix->setEnabled(false);
		aAddFile->setEnabled(false);
		
		aRemFile->setEnabled(true);
	} else {
		aRemResource->setEnabled(n);
		aPrefix->setEnabled(n);
		aAddFile->setEnabled(n);
		
		aRemFile->setEnabled(false);
	}
}

QRCEdit::FileType QRCEdit::type(const QString& f)
{
	if ( !QFile::exists(f) )
		return None;
	
	if ( QImageReader::supportedImageFormats().contains(
				QImageReader::imageFormat(f) 
			)
		)
		return Image;
	
	if ( f.endsWith(".ui") )
		return Ui;
	
	if ( f.endsWith(".htm") || f.endsWith(".html") )
		return Html;
	
	return Text;
}
