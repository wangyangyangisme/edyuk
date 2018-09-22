
#include "qsingleapplication.h"
#include "qinterprocesschannel.h"

#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QApplication>
#include <QErrorMessage>

int main(int argc, char **argv)
{
	QApplication::setApplicationName("networktest");
	
	if ( argc < 2 )
	{
		QApplication app(argc, argv);
		QInterProcessChannel channel;
		
		//channel.start();
		
		QWidget top;
		QVBoxLayout l(&top);
		
		QLabel label(&top);
		label.setText(channel.isServer() ?
						"Network test : Server\n\nMonitored messages : " :
						"Network test : client\n\n  Send a message : ");
		
		l.addWidget(&label);
		
		QLineEdit input(&top);
		input.setVisible(!channel.isServer());
		
		channel.connect(&input,	SIGNAL( textEdited(QString) ),
								SLOT  ( setMessageBuffer(QString) ) );
		
		channel.connect(&input,	SIGNAL( editingFinished() ),
								SLOT  ( sendMessage() ) );
		
		channel.connect(&input, SIGNAL( editingFinished() ),
						&input, SLOT  ( clear() ) );
		
		input.connect(&channel,	SIGNAL( gotServerRole() ),
								SLOT  ( hide() ) );
		
		l.addWidget(&input);
		
		QTextEdit output(&top);
		output.setReadOnly(true);
		output.setVisible(channel.isServer());
		
		output.connect(&channel, SIGNAL( message(QString) ),
								SLOT  ( append(QString) ) );
		
		output.connect(&channel, SIGNAL( gotServerRole() ),
								SLOT  ( show() ) );
		
		l.addWidget(&output);
		
		QPushButton quit(&top);
		quit.setText("&Quit");
		
		top.connect(&quit,	SIGNAL( clicked() ),
							SLOT  ( close() ) );
		
		l.addWidget(&quit);
		
		channel.connect(&channel,	SIGNAL( connectionLost() ),
									SLOT  ( reconnect() ) );
		
		/*
		top.connect(&channel, SIGNAL( connectionLost() ),
								SLOT( close() ) );
		*/
		
		top.show();
		
		return app.exec();
		
	} else if ( QLatin1String("single") == argv[1] ) {
	
		QSingleApplication app(argc, argv);
		app.setInstanciationPolicy(QSingleApplication::ForwardArguments);
		
		if ( app.isInstanceAllowed() )
		{
			QErrorMessage msg;
			
			msg.connect(&app, SIGNAL( request(QString) ),
						SLOT  ( showMessage(QString) ) );
			
			msg.showMessage("This is a test, new text will be displayed as new instances are created...");
			
			return app.exec();
			
		} else if ( argc > 3 && QLatin1String("-m") == argv[2] ) {
			app.sendRequest(argv[3]);
		} else {
			//app.sendRequest("Instance creation has been refused...");
			app.exec();
		}
		
	} else {
		qDebug("Unhandled mode : %s", argv[1]);
	}
}
