#include <QString>
#include <QStringList>
#include <QFile>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/// Create a directory, if necessary with subdirectories
bool mkdir_p(QString path, mode_t perms=0755)
{
	QStringList parts=path.split('/');
	QString dir="/";
	struct stat s;
	for(QStringList::Iterator it=parts.begin(); it!=parts.end(); it++) {
		dir += *it;
		if(stat(QFile::encodeName(dir), &s)) {
			if(mkdir(QFile::encodeName(dir), perms))
				return false;
		} else if(!S_ISDIR(s.st_mode))
			return false;
		dir += "/";
	}
	return true;
}

