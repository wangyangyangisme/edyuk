
#ifndef $HEADER_GUARD$
#define $HEADER_GUARD$

/*!
	\file $FILE_NAME$
	\brief Definition of $CLASS_NAME$
*/

#include <$BASE_HEADER$>
#include "$UI_HEADER$"

class $CLASS_NAME$ : public $BASE_CLASS$, public Ui::$UI_CLASS$
{
	Q_OBJECT
	
	public:
		$CLASS_NAME$(QWidget *parent = 0, Qt::WFlags f = 0);
	
	private slots:
		$SLOTS_SIG$
};

#endif
