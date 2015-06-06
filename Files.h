#ifndef _FILES_H_
#define _FILES_H_ 1
#include <qstringlist.h>
class Files {
public:
	enum Types { File	= 0x1,
		     Dir	= 0x2,
		     Link	= 0x4,
		     CharDevice	= 0x8,
		     BlockDevice= 0x10,
		     Device	= CharDevice | BlockDevice,
		     Fifo	= 0x20,
		     Socket	= 0x40,
		     All	= File | Dir | Link | Device | Fifo | Socket,
		     Any	= All
	};
	static QStringList &glob(QString name, enum Types types=All, bool dotfiles=true, bool braces=true);
};
#endif
