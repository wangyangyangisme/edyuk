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

#ifndef _GDB_RESULT_H_
#define _GDB_RESULT_H_

/*!
	\file gdbresult.h
	
	\brief Definition of RecordNode and GDBResultHandler
*/

#include <QList>
#include <QString>

struct Breakpoint
{
	inline Breakpoint() : line(-1), times(0), ignore(0), enabled(true) {}
	
	int id;
	int line;
	QString file;
	QString function;
	QString address;
	QString condition;
	int times;
	int ignore;
	bool enabled;
};

struct RecordNode
{
	enum
	{
		Simple,
		Composite
	};
	
	inline RecordNode(const QString& n) : type(Simple), id(0), name(n) {}
	
	~RecordNode()
	{ qDeleteAll(children); }
	
	RecordNode* field(const QString& name) const
	{
		foreach ( RecordNode* c, children )
			if ( c->name == name )
				return c;
		
		return 0;
	}
	
	QString fieldValue(const QString& name) const
	{
		RecordNode* f = field(name);
		
		return (f && (f->type == Simple)) ? f->value : QString();
	}
	
	int type;
	quintptr id;
	QString name;
	
	QString value;
	QList<RecordNode*> children;
};

class GDBResultHandler
{
	public:
		virtual ~GDBResultHandler() {}
		
		virtual bool discardable() const { return false; }
		virtual bool result(RecordNode *root, int type) = 0;
};

#endif // !_GDB_RESULT_H_
