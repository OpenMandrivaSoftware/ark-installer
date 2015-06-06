#ifndef __QEXTCONFIGFILE_H__
#define __QEXTCONFIGFILE_H__ 1
#include <qstring.h>
#include <qmap.h>
// TODO: support read-write
class QextConfigFile {
public:
	QextConfigFile(QString const &filename, bool unquote=true);
	QString const value(QString const &key) const { if(_configs.contains(key)) return _configs[key]; else return ""; }
	QString operator[](QString const &key) const { return value(key); }
private:
	QMap<QString,QString>	_configs;
	bool			_cs;
};
#endif
