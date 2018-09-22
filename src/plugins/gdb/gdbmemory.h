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

#ifndef _GDB_MEMORY_H_
#define _GDB_MEMORY_H_

/*!
	\file gdbmemory.h
	
	\brief Definition of GDB plugin memory reading classes
*/

#include "gdbresult.h"

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QReadWriteLock>

class GDBDriver;

struct GDBMemoryBlock
{
	inline GDBMemoryBlock(void *addr, int size)
	 : valid(false), userData(0), address(void2str(addr)), size(QByteArray::number(size))
	{
		
	}
	
	inline GDBMemoryBlock(const QString& addr, const QString& sz)
	 : valid(false), userData(0), address(addr), size(sz)
	{
		
	}
	
	static QString void2str(void *x) { return QString::number(reinterpret_cast<quintptr>(x)); }
	static void* str2void(const QString& a) { return reinterpret_cast<void*>(((quintptr)a.toULongLong())); }
	
	bool valid;
	void *userData;
	QStringList conditionals;
	QString address, size, blockSize;
	QByteArray data;
};

class GDBMemoryReader : public QObject, public GDBResultHandler
{
	Q_OBJECT
	
	public:
		GDBMemoryReader(GDBDriver *p, QObject *o = 0);
		
		virtual bool result(RecordNode *root, int type);
		
	public slots:
		void addBlock(GDBMemoryBlock *b);
		
	signals:
		void blockReadyRead(GDBMemoryBlock *b);
		
	protected:
		virtual void timerEvent(QTimerEvent *e);
		
	private:
		int m_timerId;
		GDBDriver *m_pipe;
		QList<GDBMemoryBlock*> m_blocks;
		
		QReadWriteLock m_lock;
};

#endif // !_GDB_MEMORY_H_
