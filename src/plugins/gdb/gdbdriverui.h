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

#ifndef _GDB_DRIVER_UI_H_
#define _GDB_DRIVER_UI_H_

#include <QQueue>
#include <QWidget>

#include "ui_gdb.h"
#include "gdbresult.h"

class GDBDriver;
class GDBMemoryBlock;
class GDBMemoryReader;
class BreakpointTable;

class GDBDriverUi : public QWidget, protected Ui::GDB, public GDBResultHandler
{
	Q_OBJECT
	
	public:
		GDBDriverUi(GDBDriver *d);
		virtual ~GDBDriverUi();
		
		virtual bool event(QEvent *e);
		
		void enqueueCommand(const QString& cmd, const QString& var, bool handle);
		
	public slots:
		void clear();
		void retranslate();
		void autoUpdateTick();
		void updateBreakpointTable();
		
	protected slots:
		void on_twLocals_();
		void on_chkAutoUpdateLocals_toggled(bool y);
		void on_pbUpdateLocals_clicked();
		
		void on_bAddWatch_clicked();
		void on_bRemoveWatch_clicked();
		void on_bClearWatch_clicked();
		void on_bRefreshWatch_clicked();
		
		void on_twVariableWatch_itemExpanded(QTreeWidgetItem *i);
		void on_twVariableWatch_itemPressed(QTreeWidgetItem *i, int column);
		void on_twVariableWatch_itemActivated(QTreeWidgetItem *i, int column);
		void on_twVariableWatch_itemChanged(QTreeWidgetItem *i, int column);
		
		void on_tvBreak_activated(const QModelIndex& index);
		
		void on_bRemoveBreak_clicked();
		void on_bClearBreak_clicked();
		
		void on_twRegisters_cellChanged(int row, int col);
		
		void on_bEvaluate_clicked();
		void on_bGoMem_clicked();
		
		void on_bGoDisasm_clicked();
		
		void blockReadyRead(GDBMemoryBlock *b);
		
		virtual bool result(RecordNode *root, int type);
		
	private:
		struct QueuedCommand
		{
			bool handle;
			QString command;
		};
		
		struct WatchField
		{
			inline WatchField() : column(0), item(0) {}
			
			int column;
			QTreeWidgetItem *item;
		};
		
		struct MemoryWriteBackUserData
		{
			int index;
			QString type;
			QTreeWidgetItem *item;
		};
		
		void updateLocals();
		void fetchAndWriteData(const QString& name, const QString& type, int idx, QTreeWidgetItem *item);
		
		bool m_refreshing;
		GDBDriver *m_driver;
		GDBMemoryReader *m_memory;
		WatchField m_currentWatch;
		QTreeWidgetItem *m_current;
		QQueue<QueuedCommand> m_queue;
		BreakpointTable *m_breakTable;
};

#endif // !_GDB_DRIVER_UI_H_
