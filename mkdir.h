#ifndef _MKDIR_H_
#define _MKDIR_H_ 1
#include <qstring.h>
#include <sys/types.h>

bool mkdir_p(QString path, mode_t perms=0755);
#endif
