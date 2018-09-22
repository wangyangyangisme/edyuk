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

#include "uisubclass.h"

#include "edyuktemplatemanager.h"

#include <QDir>
#include <QHash>
#include <QList>
#include <QUiLoader>
#include <QMetaObject>
#include <QMetaMethod>
#include <QMessageBox>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

class FormSignalsModel : public QAbstractItemModel
{
	public:
		struct Node
		{
			inline Node(Node *p = 0)
			 : parent(p), signal(false), implemented(false) {}
			inline Node(const QString& n, Node *p = 0)
			 : parent(p), name(n), signal(false), implemented(false) {}
			
			Node *parent;
			
			QString name;
			
			bool signal;
			bool implemented;
		};
		
		typedef QList<Node*> NodeList;
		typedef QHash<Node*, NodeList> NodeTree;
		
		FormSignalsModel(QObject *p = 0)
		 : QAbstractItemModel(p)
		{
		
		}
		
		virtual ~FormSignalsModel()
		{
			clear();
		}
		
		void clear()
		{
			NodeTree::const_iterator it = m_data.constBegin();
			
			while ( it != m_data.constEnd() )
			{
				qDeleteAll(*it);
				
				++it;
			}
			
			m_data.clear();
		}
		
		QString load(const QString& form)
		{
			clear();
			
			QUiLoader loader;
			QFile file(form);
			file.open(QFile::ReadOnly);
			QWidget *preview = loader.load(&file, 0);
			file.close();
			
			load(preview);
			
			m_class = preview->objectName();
			m_base = preview->metaObject()->className();
			m_uiHeader = "ui_" + QFileInfo(form).baseName() + ".h";
			
			delete preview;
			
			return m_class;
		}
		
		void load(QWidget *w, Node *n = 0)
		{
			NodeList l = m_data[n];
			const QObjectList& cl = w->children();
			
			foreach ( QObject *o, cl )
			{
				QWidget *c = qobject_cast<QWidget*>(o);
				
				if ( !c || c->objectName().isEmpty() )
					continue;
				
				Node *cn = new Node(c->objectName(), n);
				l << cn;
				
				load(c, cn);
			}
			
			if ( n )
			{
				const QMetaObject *mo = w->metaObject();
				
				load(mo, l, n);
			}
			
			m_data[n] = l;
		}
		
		
		void load(const QMetaObject *mo, NodeList& l, Node *n)
		{
			int off = mo->methodOffset(), count = mo->methodCount();
			
			count += off;
			
			while ( off < count )
			{
				QMetaMethod method = mo->method(off);
				
				if ( method.methodType() == QMetaMethod::Signal )
				{
					Node *cn = new Node(method.signature(), n);
					cn->signal = true;
					l << cn;
				}
				
				++off;
			}
			
			if ( mo->superClass() )
				load(mo->superClass(), l, n);
			
		}
		
		void save(const QString& className, const QString& fileName, QStringList *ol)
		{
			QString shdr = fileName + ".h", ssrc = fileName + ".cpp";
			
			QFile hdr(shdr), t_hdr(":/default-cpp-qt4/ui-impl.h");
			QFile src(ssrc), t_src(":/default-cpp-qt4/ui-impl.cpp");
			
			QStringList sl = slotList();
			QHash<QString, QString> macros;
			
			macros["CLASS_NAME"] = className;
			macros["BASE_CLASS"] = m_base;
			macros["UI_CLASS"] = m_class;
			macros["BASE_HEADER"] = m_base;
			macros["UI_HEADER"] = m_uiHeader;
			
			macros["HEADER_NAME"] = QFileInfo(shdr).fileName();
			macros["SOURCE_NAME"] = QFileInfo(ssrc).fileName();
			
			int idx = 0;
			QString guard = className;
			
			while ( idx < guard.count() )
			{
				if ( guard.at(idx).isUpper() )
				{
					guard.insert(idx, '_');
					++idx;
				} else if ( guard.at(idx).isLower() ) {
					guard.replace(idx, 1, guard.at(idx).toUpper());
				}
				
				++idx;
			}
			
			guard += "_H_";
			
			if ( guard.at(0) != '_' )
				guard.append('_');
			
			macros["HEADER_GUARD"] = guard;
			
			QStringList slotsSig, slotsImpl;
			
			foreach ( QString slot, sl )
			{
				slotsSig += "void " + slot + ";";
				
				slotsImpl += "void " + className + "::" + slot + "\n{\n\t\n}\n";
			}
			
			macros["SLOTS_SIG"] = slotsSig.join("\n\t\t");
			macros["SLOTS_IMPL"] = slotsImpl.join("\n");
			
			if ( hdr.open(QFile::WriteOnly | QFile::Text) && t_hdr.open(QFile::ReadOnly | QFile::Text) )
			{
				QString in = QString::fromLocal8Bit(t_hdr.readAll());
				t_hdr.close();
				
				macros["FILE_NAME"] = macros["HEADER_NAME"];
				
				EdyukTemplateManager::macro_substitution(in, macros);
				
				hdr.write(in.toLocal8Bit());
				hdr.close();
				
				if ( ol )
					ol->append(shdr);
			} else {
				qWarning("Unable to create UI subclass header.");
			}
			
			if ( src.open(QFile::WriteOnly | QFile::Text) && t_src.open(QFile::ReadOnly | QFile::Text) )
			{
				QString in = QString::fromLocal8Bit(t_src.readAll());
				t_src.close();
				
				macros["FILE_NAME"] = macros["SOURCE_NAME"];
				
				EdyukTemplateManager::macro_substitution(in, macros);
				
				src.write(in.toLocal8Bit());
				src.close();
				
				if ( ol )
					ol->append(ssrc);
			} else {
				qWarning("Unable to create UI subclass implementation.");
			}
		}
		
		QStringList slotList(Node *n = 0) const
		{
			QStringList sl;
			NodeList cl = m_data.value(n);
			
			foreach ( Node *c, cl )
			{
				if ( c->signal )
				{
					if ( c->implemented )
						sl << QString("on_%1_%2").arg(c->parent->name).arg(c->name);
				} else {
					sl += slotList(c);
				}
			}
			
			return sl;
		}
		
		Node* node(const QModelIndex& index) const
		{
			return static_cast<Node*>(index.internalPointer());
		}
		
		virtual Qt::ItemFlags flags(const QModelIndex& index) const
		{
			Node *n = 0;
			Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
			
			if ( index.isValid() )
				n = node(index);
			
			if ( n && n->signal )
				f |= Qt::ItemIsUserCheckable;
			
			return f;
		}
		
		virtual int rowCount(const QModelIndex& parent = QModelIndex()) const
		{
			Node *n = 0;
			
			if ( parent.isValid() )
				n = node(parent);
			
			return m_data.value(n).count();
		}
		
		virtual int columnCount(const QModelIndex& parent = QModelIndex()) const
		{
			return 1;
		}
		
		virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const
		{
			if ( section != 0 || orientation != Qt::Horizontal || role != Qt::DisplayRole )
				return QVariant();
			
			return "Signals";
		}
		
		virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const
		{
			Node *n = 0;
			
			if ( index.isValid() )
				n = node(index);
			
			if ( !n )
				return QVariant();
			
			if ( role == Qt::DisplayRole )
			{
				if ( n->parent && n->signal )
				{
					return QString("on_%1_%2").arg(n->parent->name).arg(n->name);
				} else {
					return n->name;
				}
			} else if ( n->signal && role == Qt::CheckStateRole ) {
				return n->implemented ? Qt::Checked : Qt::Unchecked;
			}
			
			return QVariant();
		}
		
		virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole)
		{
			if ( role != Qt::CheckStateRole || !index.isValid() )
				return false;
			
			Node *n = node(index);
			
			if ( !n || !n->signal )
				return false;
			
			n->implemented = value.toBool();
			
			return true;
		}
		
		virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const
		{
			if ( column != 0 )
				return QModelIndex();
			
			Node *n = 0;
			
			if ( parent.isValid() )
				n = node(parent);
			
			NodeList l = m_data.value(n);
			
			if ( l.count() > row )
				return createIndex(row, column, l.at(row));
			
			return QModelIndex();
		}
		
		virtual QModelIndex parent(const QModelIndex& index) const
		{
			Node *n = 0;
			
			if ( index.isValid() )
				n = node(index);
			
			Node *p = n ? n->parent : 0;
			
			return p ? createIndex(m_data[p->parent].indexOf(p), 0, p) : QModelIndex();
		}
		
	private:
		NodeTree m_data;
		QString m_base, m_class, m_uiHeader;
};

UiSubclass::UiSubclass(const QString& form, QWidget *p)
 : QDialog(p)
{
	setupUi(this);
	
	lePath->setText(QFileInfo(form).absolutePath());
	
	m_model = new FormSignalsModel(this);
	leClassName->setText(m_model->load(form) + "Impl");
	
	m_proxy = new QSortFilterProxyModel(this);
	m_proxy->setSourceModel(m_model);
	m_proxy->setDynamicSortFilter(true);
	
	tvSlots->setModel(m_proxy);
}

QStringList UiSubclass::createdFiles() const
{
	return m_files;
}

void UiSubclass::on_buttonBox_accepted()
{
	static QRegExp validator("[a-zA-Z_][a-zA-Z0-9_]*");
	QString name = leClassName->text(), loc = lePath->text();
	
	if ( name.isEmpty() || !validator.exactMatch(name) )
	{
		QMessageBox::warning(0, tr("Invalid class name"), tr("Please provide a valid class name."));
		
		return;
	}
	
	QString filename = QDir(loc).absoluteFilePath(name.toLower());
	
	m_model->save(name, filename, &m_files);
	
	accept();
}

void UiSubclass::on_leClassName_textChanged()
{
	QString cn = leClassName->text();
	
	if ( cn.isEmpty() )
	{
		//leClassName->setStyleSheet("QLineEdit { background-color : red }");
		leFileName->clear();
	} else {
		//leClassName->setStyleSheet("");
		leFileName->setText(tr("%1.h and %1.cpp").arg(cn.toLower()));
	}
}

void UiSubclass::on_leFilter_textChanged()
{
	m_proxy->setFilterRegExp(QRegExp(leFilter->text(), Qt::CaseInsensitive, QRegExp::FixedString));
}
