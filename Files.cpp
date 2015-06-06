#include "Files.h"
#include <QFile>
#include <glob.h>
#include <sys/stat.h>
#define ISSET(x,y) ((x&y)==y)
QStringList &Files::glob(QString name, enum Types types, bool dotfiles, bool braces)
{
	glob_t gl;
	int flags=0;
	QStringList *result;
	if(dotfiles)
		flags |= GLOB_PERIOD;
	if(braces)
		flags |= GLOB_BRACE;
	::glob(QFile::encodeName(name), flags, NULL, &gl);
	if(types==Any) {
		result=new QStringList();
		for(unsigned int i=0; i<gl.gl_pathc; i++)
			result->append(gl.gl_pathv[i]);
	} else {
		struct stat s;
		result=new QStringList;
		for(unsigned int i=0; i<gl.gl_pathc; i++) {
			if(!lstat(gl.gl_pathv[i], &s)) {
				if(S_ISLNK(s.st_mode) && !ISSET(types,Link))
					continue;
				if(S_ISREG(s.st_mode) && !ISSET(types,File))
					continue;
				if(S_ISDIR(s.st_mode) && !ISSET(types,Dir))
					continue;
				if(S_ISCHR(s.st_mode) && !ISSET(types,CharDevice))
					continue;
				if(S_ISBLK(s.st_mode) && !ISSET(types,BlockDevice))
					continue;
				if(S_ISFIFO(s.st_mode) && !ISSET(types,Fifo))
					continue;
				if(S_ISSOCK(s.st_mode) && !ISSET(types,Socket))
					continue;
				result->append(gl.gl_pathv[i]);
			}
		}
	}
	globfree(&gl);
	return *result;
}
