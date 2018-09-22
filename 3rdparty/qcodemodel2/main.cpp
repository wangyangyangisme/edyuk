
#include "qcodenode.h"
#include "qcodemodel.h"
#include "qcodenodepool.h"
#include "qcodeserializer.h"

#include "qcppparser.h"

#include <QDir>
#include <QFile>
#include <QTime>
#include <QTreeView>
#include <QTextStream>
#include <QApplication>

template <typename T, int S>
class StaticBuffer
{
	public:
		inline operator T* () { return static_cast<T*>((void*)d); }
		
		inline int size() const { return S; }
		
	private:
		char d[sizeof(T) * S];
};

//static QCodeNode static_data[25000];
static StaticBuffer<QCodeNode, 50000> static_data;

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	
	qDebug("Using a %ib buffer", sizeof(static_data));
	
	QCodeModel model;
	QCodeNodePool pool(static_data, static_data.size());
	
	QCodeSerializer serializer;
	serializer.setNodePool(&pool);
	serializer.setTargetModel(&model);
	
	QTime time;
	QDir d = QDir::current();
	
	time.start();
	
	if ( (argc == 1) || (QLatin1String("-s") == argv[1]) )
	{
		for ( int i = 1; i < argc; ++i )
			serializer.deserialize(argv[i]);
		
	} else {
		QCppParser parser(&model);
		parser.setNodePool(&pool);
		
		QStringList ext;
		ext << "h" << "hpp" << "hxx";
		
		for ( int i = 1; i < argc; ++i )
			parser.parseDirectory(argv[i], ext);
		
		foreach ( QCodeNode *n, model.topLevelNodes() )
		{
			QFile f(n->role(QCodeNode::Name) + ".tag");
			
			if ( !f.open(QFile::Text | QFile::WriteOnly) )
				continue;
			
			QTextStream s(&f);
			
			serializer.serialize(n, s);
		}
	}
	
	qDebug("model filled in %i ms.", time.elapsed());
	qDebug("Pool : used=%i%%; free=%i%%", pool.occupied(), pool.remaining());
	
	QTreeView view;
	view.setModel(&model);
	view.show();
	
	return app.exec();
}
