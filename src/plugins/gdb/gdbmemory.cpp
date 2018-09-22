/****************************************************************************
**
** 	Created using Edyuk 1.1.0
**
** File : gdbmemory.cpp
** Date : Wed Dec 31 13:18:58 2008
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

/*!
	\file gdbmemory.cpp
	
	\brief Implementation of
*/

#include "gdbmemory.h"

#include "gdbdriver.h"
#include "gdbdriverthread.h"

#include <QTimerEvent>

GDBMemoryReader::GDBMemoryReader(GDBDriver *p, QObject *o)
 : QObject(o), m_pipe(p)
{
	
}

void GDBMemoryReader::addBlock(GDBMemoryBlock *b)
{
	if ( !m_pipe )
		return;
	
	m_blocks << b;
	
	bool ok = true;
	QString cond;
	QStringList params;
	
	if ( b->conditionals.count() )
	{
		//qDebug("conditional read.");
		
		cond = b->conditionals.at(0);
		
		for ( int i = 1; i < b->conditionals.count(); ++i )
			params << b->conditionals.at(i);
	}
	
	QString cmd = "20-data-read-memory ";
	cmd += b->address;
	cmd += " x ";
	
	b->blockSize.toInt(&ok, 0);
	
	if ( ok  )
		cmd += b->blockSize;
	else if ( b->blockSize.isEmpty() )
		cmd += '1';
	else {
		cmd += '%';
		cmd += QString::number(params.count() + 1);
		params << b->blockSize;
	}
	
	cmd += " 1 ";
	
	b->size.toInt(&ok, 0);
	
	if ( ok )
		cmd += b->size;
	else if ( b->size.isEmpty() )
		cmd += '1';
	else {
		cmd += '%';
		cmd += QString::number(params.count() + 1);
		params << b->size;
	}
	
	m_pipe->command(cmd, cond, params, this);
}

bool GDBMemoryReader::result(RecordNode *root, int type)
{
	if ( root->id != 20 || m_blocks.isEmpty() )
		return false;
	
	if ( type != GDBDriverThread::Success )
	{
		// TODO : figure out why
		m_blocks.removeFirst();
		return true;
	}
	
	RecordNode *mem = root->field("memory");
	
	if ( !mem || mem->children.isEmpty() )
		return false;
	
	RecordNode *data = mem->children.at(0)->field("data");
	
	if ( !data )
		return false;
	
	QByteArray d;
	d.resize(root->fieldValue("nr-bytes").toInt());
	int blockSize = d.size() / data->children.count();
	
	for ( int i = 0; i < data->children.count(); i += blockSize )
	{
		QString chunk = data->children.at(i)->value;
		int thisBlockSize = (chunk.size() / 2 - 1);
		
		if ( blockSize != thisBlockSize )
			qWarning("inconsistent block size");
		
		if ( blockSize == 1 )
		{
			d[i] = chunk.toInt(0, 0);
		} else {
			qWarning("unsupported block size : %i", blockSize);
			
			for ( int j = 0; j < blockSize; ++j )
				d[i + j] = chunk.mid((j + 1) * 2, 2).toInt(0, 16);
		}
	}
	
	GDBMemoryBlock *b = m_blocks.takeFirst();
	
	b->valid = true;
	b->data = d;
	
	emit blockReadyRead(b);
	
	return true;
}

void GDBMemoryReader::timerEvent(QTimerEvent *e)
{
	if ( e->timerId() != m_timerId )
	{
		QObject::timerEvent(e);
		return;
	}
	
	/*
	QWriteLocker locker(&m_lock);
	
	for ( int i = 0; i < m_blocks.count(); ++i )
	{
		GDBMemoryBlock *b = m_blocks.at(i);
		
		if ( b && b->valid )
		{
			m_blocks.removeAt(i);
			--i;
		}
	}
	*/
}

