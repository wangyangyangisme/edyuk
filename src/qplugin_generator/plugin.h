/****************************************************************************
**
** Generated
**    by QPlugin Generator $version$, (C) fullmetalcoder 2007
**    from $input$
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#ifndef $header_guard$
#define $header_guard$

#include "qpluginsystem.h"

/*!
	\file $output$.h
	\brief Definition of a QPlugin-based plugin class : $class$
*/

#include <QHash>
#include <QList>
#include <QVariant>
#include <QCoreApplication>

$forward_component_list$

typedef void (*PropertyWatch)(const QVariant& v);
typedef QList<PropertyWatch> PropertyWatchList;

class $class$ : public QPlugin
{
	Q_DECLARE_TR_FUNCTIONS($class$)
	
	public:
		$class$();
		virtual ~$class$();
		
		virtual void retranslate();
		virtual QString tr(const QString& txt) const;
		
		virtual QString configScheme() const;
		virtual void setConfigKey(const QString& key, const QVariant& value);
		
		virtual QStringList types();
		virtual QStringList keys(const QString& t);
		virtual void* object(const QString& id, const QString& t);
		
		template <typename T>
		inline static T configKey(const QString& k, const T& dv)
		{
			if ( !m_keys.contains(k) )
				return dv;
			
			QVariant v = m_keys[k];
			
			if ( v.isNull() || !v.isValid() || !qVariantCanConvert<T>(v) )
				return dv;
			
			return qvariant_cast<T>(v);
		}
		
		static void addWatch(const QString& k, PropertyWatch w)
		{
			watches()[k] << w;
		}
		
	private:
$members_component_list$
		
		static QHash<QString, PropertyWatchList>& watches();
		
		void stubTrForSchemeStrings();
		
		static QHash<QString, QVariant> m_keys;
		
};

#endif // !$header_guard$
