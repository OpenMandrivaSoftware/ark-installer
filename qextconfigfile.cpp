#include "qextconfigfile.h"
#include <qfile.h>
#include <iostream>
using namespace std;
QextConfigFile::QextConfigFile(QString const &filename, bool unquote)
{
	QFile f(filename);
	if (f.open(QFile::ReadOnly))
	{
		QString line, key, value;
		while(!f.atEnd()) {
			line=f.readLine().trimmed();
			if(line.contains('=')) {
				key=line.section('=', 0, 0);
				value=line.section('=', 1);
				if(unquote) {
					if(value.left(1)=="\"")
						value=value.mid(1);
					if(value.right(1)=="\"")
						value=value.left(value.length()-1);
				}
				_configs.insert(key, value);
			}
		}
		f.close();
	}
}
