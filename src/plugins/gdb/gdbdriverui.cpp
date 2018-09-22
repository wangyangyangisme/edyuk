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

#include "gdbdriverui.h"

#include "gdbdriver.h"
#include "gdbdriverthread.h"

#include "gdbmemory.h"

#include <QStack>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QAbstractTableModel>

class BreakpointTable : public QAbstractTableModel
{
	public:
		BreakpointTable(GDBDriver *d, QObject *p = 0)
		 : QAbstractTableModel(p), m_driver(d)
		{
			
		}
		
		virtual int columnCount(const QModelIndex& parent) const
		{
			return 7;
		}
		
		virtual int rowCount(const QModelIndex& parent) const
		{
			return parent.isValid() ? 0 : m_driver->breakpoints().count();
		}
		
		virtual Qt::ItemFlags flags(const QModelIndex& idx) const
		{
			Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
			
			if ( idx.column() == 3 || idx.column() == 5 )
				f |= Qt::ItemIsEditable;
			else if ( idx.column() == 6 )
				f |= Qt::ItemIsUserCheckable;
			
			return f;
		}
		
		virtual QVariant headerData(int section, Qt::Orientation o, int role) const
		{
			if ( o != Qt::Horizontal || role != Qt::DisplayRole || section < 0 || section > 6 )
				return QVariant();
			
			QStringList hdr = QStringList()
				<< tr("File")
				<< tr("Line/Func")
				<< tr("Address")
				<< tr("Condition")
				<< tr("Times")
				<< tr("Ignore")
				<< tr("Enabled")
				;
			
			return hdr.at(section);
		}
		
		virtual QVariant data(const QModelIndex& parent, int role) const
		{
			if ( parent.isValid() )
			{
				const Breakpoint& bkpt = m_driver->breakpoints().at(parent.row());
				
				QString file = QFileInfo(bkpt.file).fileName();
				
				if ( role == Qt::DisplayRole || role == Qt::EditRole )
				{
					switch ( parent.column() )
					{
						case 0 :
							return file;
							
						case 1 :
							return 	bkpt.line != -1
									?
										QString::number(bkpt.line)
									:
										bkpt.function
									;
							
						case 2 :
							return bkpt.address;
							
						case 3 :
							return bkpt.condition;
							
						case 4 :
							return bkpt.times;
							
						case 5 :
							return bkpt.ignore;
							
						case 6 :
							return tr("enabled");
							
						default:
							break;
					}
				} else if ( role == Qt::CheckStateRole && parent.column() == 6 ) {
					return bkpt.enabled ? Qt::Checked : Qt::Unchecked;
				}
			}
			
			return QVariant();
		}
		
		virtual bool setData(const QModelIndex& index, const QVariant & value, int role = Qt::EditRole)
		{
			int row = index.row(),
				col = index.column();
			
			if ( row < 0 || row >= m_driver->breakpoints().count() )
				return false;
			
			Breakpoint& bkpt = m_driver->breakpoints()[index.row()];
			
			if ( col == 3 && role == Qt::EditRole )
			{
				bkpt.condition = value.toString();
				m_driver->command(QString("-break-condition %1 %2").arg(bkpt.id).arg(bkpt.condition));
				return true;
			} else if ( col == 5 && role == Qt::EditRole ) {
				bkpt.ignore = value.toInt();
				m_driver->command(QString("-break-after %1 %2").arg(bkpt.id).arg(bkpt.ignore));
				return true;
			} else if ( col == 6 && role == Qt::CheckStateRole ) {
				//qDebug("setting check state to %i [%s]", value.toBool(), qPrintable(value.toString()));
				bkpt.enabled = value.toBool();
				
				m_driver->command(QString("-break-%1 %2").arg(bkpt.enabled ? "enable" : "disable").arg(bkpt.id));
				return true;
			}
			
			return false;
		}
		
		inline void update() { reset(); }
		
	private:
		GDBDriver *m_driver;
};

extern void dump(RecordNode *n, QString indent = QString());

GDBDriverUi::GDBDriverUi(GDBDriver *d)
: QWidget(0), m_refreshing(false), m_driver(d)
{
	m_memory = new GDBMemoryReader(d, this);
	
	connect(m_memory, SIGNAL( blockReadyRead(GDBMemoryBlock*) ),
			this	, SLOT  ( blockReadyRead(GDBMemoryBlock*) ) );
	
	m_breakTable = new BreakpointTable(d, this);
	
	setupUi(this);
	
	twVariableWatch->setEditTriggers(QAbstractItemView::NoEditTriggers);
	
	twRegisters->horizontalHeader()->hide();
	
	tvBreak->setModel(m_breakTable);
	tvBreak->verticalHeader()->hide();
	
	twMemory->verticalHeader()->hide();
	twMemory->horizontalHeader()->hide();
	
	twAssembly->setShowGrid(false);
	twAssembly->verticalHeader()->hide();
	twAssembly->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
}

GDBDriverUi::~GDBDriverUi()
{
	twAssembly = 0;
	twRegisters = 0;
	twVariableWatch = 0;
}

bool GDBDriverUi::event(QEvent *e)
{
	switch ( e->type() )
	{
		case Edyuk::RunTimeTranslation :
			retranslateUi(this);
			break;
			
		default:
			break;
	}
	
	return QWidget::event(e);
}

void GDBDriverUi::clear()
{
	m_current = 0;
	
	if ( twLocals )
		twLocals->clear();
	
	if ( twVariableWatch )
	{
		twVariableWatch->clear();
	}
	
	if ( twRegisters )
	{
		twRegisters->clearContents();
		twRegisters->setColumnCount(0);
	}
	
	if ( twMemory )
	{
		twMemory->setRowCount(0);
		twMemory->setColumnCount(0);
	}
	
	if ( twAssembly )
	{
		twAssembly->clearContents();
		twAssembly->setRowCount(0);
	}
	
	m_queue.clear();
}

void GDBDriverUi::retranslate()
{
	cbMemSize->clear();
	
	retranslateUi(this);
}

void GDBDriverUi::autoUpdateTick()
{
	if ( !m_driver || !m_driver->isRunning() )
		return;
	
	if ( true ) //m_autoUpdateVar )
		m_driver->command("-var-update --all-values *", this);
	
	if ( true ) //m_autoUpdateReg )
		m_driver->command("-data-list-changed-registers", this);
	
	if ( chkAutoUpdateLocals->isChecked() )
		updateLocals();
	
}

void GDBDriverUi::updateBreakpointTable()
{
	m_breakTable->update();
	tvBreak->resizeColumnsToContents();
}

void GDBDriverUi::enqueueCommand(const QString& cmd, const QString& var, bool handle)
{
	QueuedCommand cobj;
	cobj.handle = handle;
	cobj.command = cmd;
	m_queue.enqueue(cobj);
	
	m_driver->command(QString("-var-info-path-expression %1").arg(var), this);
}

// local variables
void GDBDriverUi::updateLocals()
{
	twLocals->clear();
	
	if ( !m_driver || !m_driver->isRunning() )
		return;
	
	// params, with values (1), for topmost stack frame only (0-0)
	m_driver->command("-stack-list-arguments 1 0 0", this);
	
	m_driver->command("-stack-list-locals --simple-values", this);
}

void GDBDriverUi::on_twLocals_()
{
	
}

void GDBDriverUi::on_chkAutoUpdateLocals_toggled(bool y)
{
	if ( y )
	{
		updateLocals();
	}
	
	twLocals->clear();
}

void GDBDriverUi::on_pbUpdateLocals_clicked()
{
	updateLocals();
}

// variable watches
void GDBDriverUi::on_bAddWatch_clicked()
{
	QString expr = QInputDialog::getText(0, tr("Add variable watch"), tr("Expression to watch"));
	
	if ( expr.isEmpty() )
		return;
	
	m_driver->command(QString("-var-create %1 * %1").arg(expr), this);
}

void GDBDriverUi::on_bRemoveWatch_clicked()
{
	QList<QTreeWidgetItem*> sel = twVariableWatch->selectedItems();
	
	foreach ( QTreeWidgetItem *it, sel )
	{
		m_driver->command(QString("-var-delete %1").arg(it->data(0, Qt::UserRole).toString()));
		delete it;
	}
}

void GDBDriverUi::on_bClearWatch_clicked()
{
	for ( int i = 0; i < twVariableWatch->topLevelItemCount(); ++i )
	{
		m_driver->command(QString("-var-delete %1").arg(twVariableWatch->topLevelItem(i)->data(0, Qt::UserRole).toString()));
	}
	
	twVariableWatch->clear();
}

void GDBDriverUi::on_bRefreshWatch_clicked()
{
	m_driver->command("-var-update --all-values *", this);
}

void GDBDriverUi::on_twVariableWatch_itemExpanded(QTreeWidgetItem *i)
{
	if ( i && !i->childCount() )
	{
		m_current = i;
		
		m_driver->command(
						QString("-var-list-children --all-values %1")
							.arg(i->data(0, Qt::UserRole).toString()),
						this
					);
	} else if ( !i ) {
		m_current = 0;
	}
}

void GDBDriverUi::on_twVariableWatch_itemPressed(QTreeWidgetItem *i, int column)
{
	
}

static bool isAssignable(const QString& s)
{
	static QStringList basic = QStringList()
		<< "int"
		<< "uint"
		<< "char"
		<< "uchar"
		<< "short"
		<< "ushort"
		<< "long"
		<< "float"
		<< "double"
		<< "wchar_t"
		;
	
	return basic.contains(s) || s.endsWith('*');
}

void GDBDriverUi::on_twVariableWatch_itemActivated(QTreeWidgetItem *i, int column)
{
	if ( !i )
	{
		m_current = i;
		return;
	}
	
	if ( column == 2 && !isAssignable(i->text(1)) )
		return;
	
	if ( column >= 2 )
	{
		m_currentWatch.item = i;
		m_currentWatch.column = column;
		
		i->setFlags(i->flags() | Qt::ItemIsEditable);
		twVariableWatch->editItem(i, column);
		i->setFlags(i->flags() & (~Qt::ItemIsEditable));
	} else {
		m_current = i;
		m_driver->command(
						QString("-var-update --all-values %1")
							.arg(i->data(0, Qt::UserRole).toString()),
						this
					);
	}
}

void GDBDriverUi::on_twVariableWatch_itemChanged(QTreeWidgetItem *i, int column)
{
	if ( !i )
	{
		m_current = i;
		return;
	}
	
	if ( m_refreshing )
		return;
	
	QString name = i->data(0, Qt::UserRole).toString();
	
	if ( column == 2 )
	{
		// check type before going mad...
		if ( !isAssignable(i->text(1)) )
			return;
		
		m_driver->command(QString("-var-assign %1 %2").arg(name).arg(i->text(2)));
	} else if ( column == 3 ) {
		Qt::CheckState s = i->checkState(3);
		int id = i->data(3, Qt::UserRole).toInt();
		
		if ( id >= 0 )
			m_driver->command(QString("-break-delete %1").arg(id));
		
		if ( s == Qt::Unchecked )
		{
			i->setData(3, Qt::UserRole, -1);
			
		} else if ( s == Qt::Checked ) {
			
			QStringList l = QStringList()
				<< tr("Triggers on write access")
				<< tr("Triggers on read access")
				<< tr("Triggers on both read and write access");
			
			bool ok = false;
			QString s = QInputDialog::getItem(
								0,
								tr("Create a watchpoint from variable"),
								tr("Select watchpoint type"),
								l,
								0,
								false,
								&ok
							);
			
			if ( !ok )
			{
				i->setData(3, Qt::CheckStateRole, Qt::Unchecked);
				return;
			}
			
			int idx = l.indexOf(s);
			
			QString pattern = "-break-watch%1\"%3%2\"";
			
			switch ( idx )
			{
				case 0 :
					pattern = pattern.arg(" ");
					break;
					
				case 1 :
					pattern = pattern.arg(" -r ");
					break;
					
				case 2 :
					pattern = pattern.arg(" -a ");
					break;
					
				default:
					//pattern = pattern.arg(" ");
					i->setData(3, Qt::CheckStateRole, Qt::Unchecked);
					return;
			}
			
			m_current = i;
			pattern = pattern.arg(i->text(3) != "has a watchpoint" ? QString(" ") + i->text(3) : QString());
			enqueueCommand(pattern, name, true);
		}
			
	}
}

void GDBDriverUi::on_bRemoveBreak_clicked()
{
	QList<int> ids;
	QModelIndexList l = tvBreak->selectionModel()->selectedRows();
	
	foreach ( const QModelIndex& idx, l )
	{
		int number = m_driver->breakpoints().at(idx.row()).id;
		
		if ( !ids.contains(number) )
			ids << number;
	}
	
	//qDebug("removing %i bkpts [%i indices]", ids.count(), l.count());
	
	foreach ( int id, ids )
	{
		m_driver->command(QString("-break-delete %1").arg(id));
	}
}

void GDBDriverUi::on_bClearBreak_clicked()
{
	//qDebug("clearing bkpts.");
	
	const QList<Breakpoint> l = m_driver->breakpoints();
	
	for ( int i = 0; i < l.count(); ++i )
	{
		m_driver->command(QString("-break-delete %1").arg(l.at(i).id));
	}
}

void GDBDriverUi::on_tvBreak_activated(const QModelIndex& index)
{
	if ( !index.isValid() )
		return;
	
	const Breakpoint& bkpt = m_driver->breakpoints().at(index.row());
	
	m_driver->setLocation(bkpt.file, bkpt.line -1, false);
}

void GDBDriverUi::on_twRegisters_cellChanged(int row, int col)
{
	if ( row != 3 || m_refreshing )
		return;
	
	QTableWidgetItem *it = twRegisters->item(row, col);
	
	Qt::CheckState s = it->checkState();
	int id = it->data(Qt::UserRole).toInt();
	
	QString reg = twRegisters->item(0, col)->text();
	
	if ( s == Qt::Unchecked )
	{
		if ( id >= 0 )
			m_driver->command(QString("-break-delete %1").arg(id));
		
		it->setData(Qt::UserRole, -1);
		
	} else if ( s == Qt::Checked ) {
		
		QStringList l = QStringList()
			<< tr("Triggers on write access")
			<< tr("Triggers on read access")
			<< tr("Triggers on both read and write access");
		
		QString s = QInputDialog::getItem(
							0,
							tr("Create a watchpoint from variable"),
							tr("Select watchpoint type"),
							l
						);
		
		int idx = l.indexOf(s);
		
		QString pattern = "%3-break-watch%1$%2";
		
		switch ( idx )
		{
			case 0 :
				pattern = pattern.arg(" ");
				break;
				
			case 1 :
				pattern = pattern.arg(" -r ");
				break;
				
			case 2 :
				pattern = pattern.arg(" -a ");
				break;
				
			default:
				pattern = pattern.arg(" ");
				break;
		}
		
		m_driver->command(pattern.arg(reg).arg(1000 + col), this);
	}
}

void GDBDriverUi::on_bEvaluate_clicked()
{
	QString expr = leExpression->text();
	
	if ( expr.isEmpty() )
		return;
	
	m_driver->command(QString("15-data-evaluate-expression \"%1\"").arg(expr), this);
}

void GDBDriverUi::on_bGoMem_clicked()
{
	twMemory->clearContents();
	
	QString addr = leMemAddress->text();
	
	if ( addr.isEmpty() )
		return;
	
	twMemory->setRowCount(sbMemRows->value());
	twMemory->setColumnCount(sbMemColumns->value());
	
	m_driver->command(
				QString("-data-read-memory %1 x %4 %2 %3")
					.arg(addr)
					.arg(sbMemRows->value())
					.arg(sbMemColumns->value())
					.arg(1 << cbMemSize->currentIndex()),
				this
			);
	
}

void GDBDriverUi::on_bGoDisasm_clicked()
{
	QString file = leFileDisasm->text();
	int line = sbLineDisasm->value(), length = sbLengthDisasm->value();
	
	m_driver->command(QString("-data-disassemble -f %1 -l %2 -- 1").arg(file).arg(line), this);
	//m_driver->command(QString("-data-disassemble -f %1 -l %2 -n %3 -- 1").arg(file).arg(line).arg(length), this);
}

bool strToBool(const QString& s)
{
	if ( s == "true" )
		return true;
	else if ( s == "false" )
		return false;
	
	return s.toInt();
}

QList<QTreeWidgetItem*> findItems(QTreeWidget *w, const QString& name)
{
	// brute force : recursive visitor (use path extrapolation instead?)
	QList<QTreeWidgetItem*> l, cl;
	QStack<QTreeWidgetItem*> tree;
	
	for ( int i = 0; i < w->topLevelItemCount(); ++i )
		tree.push(w->topLevelItem(i));
	
	while ( tree.count() )
	{
		QTreeWidgetItem *it = tree.pop();
		
		if ( it->data(0, Qt::UserRole).toString() == name )
			l << it;
		else
			for ( int i = 0; i < it->childCount(); ++i )
				tree.push(it->child(i));
	}
	
	return l;
}

bool GDBDriverUi::result(RecordNode *root, int nt)
{
	if ( nt == GDBDriverThread::Success )
	{
		//qDebug("forward");
		
		RecordNode *rn = root->field("changelist");
		
		if ( rn )
		{
			// -var-update
			//qDebug("result guess : -var-update");
			//dump(rn);
			m_refreshing = true;
			
			foreach ( RecordNode *c, rn->children )
			{
				QString name = c->fieldValue("name"),
						value = c->fieldValue("value"),
						in_scope = c->fieldValue("in_scope"),
						type_changed = c->fieldValue("type_changed");
				
				QList<QTreeWidgetItem*> l = findItems(twVariableWatch, name);
				
				if ( l.count() == 1 )
				{
					l.at(0)->setText(2, value);
					l.at(0)->setDisabled(!strToBool(in_scope));
					
					if ( strToBool(type_changed) )
					{
						//m_current = l.at(0);
						//m_driver->command(QString("-var-info-type %1").arg(name), this);
					}
					
				} else {
					qDebug("error : %s variable object (%s)", l.isEmpty() ? "no such" : "duplicate", qPrintable(name));
				}
			}
			
			m_refreshing = false;
			
			return true;
		}
		
		rn = root->field("stack-args");
		
		if ( rn )
		{
			//qDebug("result guess : -stack-list-arguments");
			
			if ( rn->children.count() )
			{
				rn = rn->children.at(0)->field("args");
				
				if ( rn )
				{
					QTreeWidgetItem *item = 0;
					QList<QTreeWidgetItem*> items;
					
					for ( int i = 0; i < rn->children.count(); ++i )
					{
						RecordNode *c = rn->children.at(i);
						QString name = c->fieldValue("name"),
								value = c->fieldValue("value");
						
						item = new QTreeWidgetItem(
											QStringList(name)
												<< QString()
												<< value
										);
						
						//fetchAndWriteData(name, type, 2, item);
						
						items << item;
					}
					
					twLocals->addTopLevelItems(items);
				}
			}
			
			return true;
		}
		
		rn = root->field("locals");
		
		if ( rn )
		{
			//qDebug("result guess : -stack-list-locals");
			
			QTreeWidgetItem *item = 0;
			QList<QTreeWidgetItem*> items;
			
			for ( int i = 0; i < rn->children.count(); ++i )
			{
				RecordNode *c = rn->children.at(i);
				QString name = c->fieldValue("name"),
						type = c->fieldValue("type"),
						value = c->fieldValue("value");
				
				item = new QTreeWidgetItem(
									QStringList(name)
										<< type
										<< value
								);
				
				fetchAndWriteData(name, type, 2, item);
				
				items << item;
			}
			
			twLocals->addTopLevelItems(items);
			
			return true;
		}
		
		rn = root->field("wpt");
		
		if ( rn )
		{
			//dump(root);
			//qDebug("result guess : watchpoints stuff");
			
			m_refreshing = true;
			
			if ( root->id >= 1000 )
			{
				QTableWidgetItem *it = twRegisters->item(3,
														qBound(
															0,
															int(root->id - 1000),
															twRegisters->columnCount()
														)
													);
				
				if ( it )
					it->setData(Qt::UserRole, rn->fieldValue("number").toInt());
				//it->setCheckState(Qt::Checked);
			} else if ( m_current ) {
				m_current->setData(3, Qt::UserRole, rn->fieldValue("number").toInt());
				//m_current->setCheckState(3, Qt::Checked);
				m_current = 0;
			}
			
			m_refreshing = false;
			
			return true;
		}
		
		rn = root->field("register-names");
		
		if ( rn )
		{
			//qDebug("result guess : -data-list-register-names");
			
			twRegisters->clearContents();
			twRegisters->setColumnCount(rn->children.count());
			
			for ( int i = 0; i < rn->children.count(); ++i )
			{
				RecordNode *c = rn->children.at(i);
				
				QTableWidgetItem *it;
				
				it = new QTableWidgetItem;
				it->setText(c->value);
				twRegisters->setItem(0, i, it);
				
				it = new QTableWidgetItem;
				twRegisters->setItem(1, i, it);
				
				it = new QTableWidgetItem;
				twRegisters->setItem(2, i, it);
				
				it = new QTableWidgetItem;
				it->setText("on");
				it->setData(Qt::UserRole, -1);
				it->setCheckState(Qt::Unchecked);
				twRegisters->setItem(3, i, it);
			}
			
			return true;
		}
		
		rn = root->field("changed-registers");
		
		if ( rn )
		{
			//qDebug("result guess : -data-list-changed-registers");
			
			QStringList regs;
			
			for ( int i = 0; i < rn->children.count(); ++i )
			{
				regs << rn->children.at(i)->value;
			}
			
			//qDebug("changed registers : %s", qPrintable(regs.join(", ")));
			
			if ( regs.count() )
			{
				QString cmd = QString("%2-data-list-register-values %1 %3").arg(regs.join(" "));
				
				m_driver->command(cmd.arg(11).arg("x"), this);
				m_driver->command(cmd.arg(12).arg("N"), this);
			}
			
			return true;
		}
		
		rn = root->field("register-values");
		
		if ( rn )
		{
			//qDebug("result guess : -data-list-register-values");
			
			int row = root->id - 10;
			
			if ( row < 1 || row > 2 )
				row = 2;
			
			for ( int i = 0; i < rn->children.count(); ++i )
			{
				RecordNode *c = rn->children.at(i);
				QTableWidgetItem *it = twRegisters->item(row, c->fieldValue("number").toInt());
				
				if ( it )
					it->setText(c->fieldValue("value"));
			}
			
			twRegisters->resizeColumnsToContents();
			
			return true;
		}
		
		rn = root->field("memory");
		
		if ( rn )
		{
			//qDebug("result guess : -data-read-memory");
			
			int row = 0, col = 0;
			
			//dump(rn);
			
			twMemory->clearContents();
			twMemory->setRowCount(rn->children.count());
			
			foreach ( RecordNode *r, rn->children )
			{
				RecordNode *d = r->field("data");
				
				if ( !d )
					continue;
				
				col = 0;
				twMemory->setColumnCount(qMax(twMemory->columnCount(), d->children.count()));
				
				foreach ( RecordNode *c, d->children )
				{
					QTableWidgetItem *i = new QTableWidgetItem;
					i->setText(c->value);
					twMemory->setItem(row, col, i);
					
					++col;
				}
				
				++row;
			}
			
			twMemory->resizeColumnsToContents();
			
			return true;
		}
		
		rn = root->field("asm_insns");
		
		if ( rn )
		{
			//qDebug("result guess : -data-disassemble");
			
			twAssembly->clearContents();
			int length = sbLengthDisasm->value();
			
			//dump(root);
			
			int row = 0, lc = 0;
			
			QFont f = twAssembly->font();
			f.setStyleHint(QFont::Courier);
			twAssembly->setFont(f);
			f.setWeight(QFont::Bold);
			
			foreach ( RecordNode *l, rn->children )
			{
				if ( l->name != "src_and_asm_line" )
					continue;
				
				QString ln = l->fieldValue("line");
				
				if ( ln.toInt() < sbLineDisasm->value() )
				{
					//qDebug("line %i discarded.", l->fieldValue("line").toInt());
					continue;
				}
				
				RecordNode *ins = l->field("line_asm_insn");
				
				if ( !ins || ins->children.isEmpty() )
				{
					//qDebug("line %i has no instructions.", l->fieldValue("line").toInt());
					continue;
				}
				
				QTableWidgetItem *i;
				
				i = new QTableWidgetItem;
				i->setFont(f);
				i->setText(ln);
				twAssembly->setRowCount(row + ins->children.count() + 1);
				twAssembly->setItem(row, 0, i);
				
				++lc;
				++row;
				
				foreach ( RecordNode *c, ins->children )
				{
					i = new QTableWidgetItem;
					i->setBackground(twAssembly->palette().window());
					i->setText(c->fieldValue("address"));
					twAssembly->setItem(row, 0, i);
					
					i = new QTableWidgetItem;
					i->setBackground(twAssembly->palette().base());
					i->setText(c->fieldValue("inst").replace("\\t", "\t"));
					twAssembly->setItem(row, 1, i);
					++row;
				}
				
				if ( length > 0 && lc >= length )
					break;
				
			}
			
			return true;
		}
		
		rn = root->field("children");
		
		if ( rn && m_current )
		{
			// -var-list-children
			//qDebug("result guess : -var-list-children");
			
			m_refreshing = true;
			
			foreach ( RecordNode *c, rn->children )
			{
				bool ok = false;
				int n = c->fieldValue("numchild").toUInt(&ok, 10);
				
				QString name = c->fieldValue("name"),
						type = c->fieldValue("type"),
						value = c->fieldValue("value");
				
				
				QTreeWidgetItem *it = new QTreeWidgetItem(m_current, QStringList(name.section('.', -1, -1)) << type << value << tr("has a watchpoint"));
				it->setFlags(it->flags() | Qt::ItemIsEditable);
				it->setCheckState(3, Qt::Unchecked);
				it->setData(0, Qt::UserRole, name);
				it->setData(3, Qt::UserRole, -1);
				
				if ( ok && n )
					it->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
				
			}
			
			m_refreshing = false;
			
			m_current = 0;
			return true;
		}
		
		bool ok = false;
		int n = root->fieldValue("numchild").toUInt(&ok, 10);
		QString name = root->fieldValue("name"),
				type = root->fieldValue("type"),
				value = root->fieldValue("value");
		
		if ( name.count() && type.count() && ok )
		{
			// -var-create
			//qDebug("result guess : -var-create");
			//dump(root);
			
			m_refreshing = true;
			QTreeWidgetItem *it = new QTreeWidgetItem(
											QStringList(name)
											<< type
											<< QString()
											<< tr("has a watchpoint")
										);
			
			it->setFlags(it->flags()); // | Qt::ItemIsEditable);
			it->setCheckState(3, Qt::Unchecked);
			it->setData(0, Qt::UserRole, name);
			it->setData(3, Qt::UserRole, -1);
			
			if ( ok && n )
				it->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
			
			twVariableWatch->addTopLevelItem(it);
			m_refreshing = false;
			
			m_current = it;
			//m_driver->command(QString("-var-evaluate-expression %1").arg(name), this);
			
			if ( type == "QString" )
			{
				fetchAndWriteData(name, type, 2, it);
			} else {
				enqueueCommand("-var-evaluate-expression %1", name, this);
			}
			
			return true;
		} else if ( value.count() ) {
			
			if ( root->id == 15 )
			{
				// -data-evaluate-expression
				
				QMessageBox::information(0,
								tr("Expression evaluated"), 
								tr("%1\n=\n%2")
								.arg(leExpression->text())
								.arg(value)
							);
				
				return true;
			} else if ( m_current ) {
				// -var-evaluate-expression
				m_refreshing = true;
				m_current->setText(2, value);
				m_refreshing = false;
				
				m_current = 0;
			}
			
			return true;
		} else if ( type.count() && m_current ) {
			// -var-info-type
			
			m_refreshing = true;
			m_current->setText(1, type);
			m_refreshing = false;
			
			m_current = 0;
			return true;
		}
		
		QString path_expr = root->fieldValue("path_expr");
		
		if ( path_expr.count() && m_queue.count() )
		{
			QueuedCommand cmd = m_queue.dequeue();
			
			m_driver->command(cmd.command.arg(path_expr), cmd.handle ? this : 0);
			return true;
		}
		
	} else {
		//qDebug("failure.");
	}
	
	return false;
}

void GDBDriverUi::fetchAndWriteData(const QString& name, const QString& type, int idx, QTreeWidgetItem *item)
{
	if ( type != "QString" )
		return;
	
	GDBMemoryBlock *b = new GDBMemoryBlock(name + ".d->data", name + ".d->size * 2");
	MemoryWriteBackUserData *d = new MemoryWriteBackUserData;
	d->type = type;
	d->index = idx;
	d->item = item;
	b->userData = d;
	// lame trick to avoid bad surprises on uninitialized variables
	b->conditionals << "%1 > 0 && %1 < 100" << name + ".d->size";
	m_memory->addBlock(b);
}

void GDBDriverUi::blockReadyRead(GDBMemoryBlock *b)
{
	//qDebug("memory block read.");
	MemoryWriteBackUserData *d = reinterpret_cast<MemoryWriteBackUserData*>(b->userData);
	
	if ( b->valid )
	{
		QTreeWidgetItem *it = d->item;
		
		if ( it && d->type == "QString" )
		{
			QString str = QString::fromUtf16(
										reinterpret_cast<const ushort*>(b->data.constData()),
										b->data.length() / 2
									);
			
			it->setText(d->index, str);
		}
	}
	
	delete d;
	delete b;
}
