#include "modules.h"
#include <QFile>
#include <QStringList>

extern "C" {
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
}

extern "C" {
	extern long init_module(void *addr, unsigned long len, const char *args);
	extern long delete_module(const char *name, unsigned int flags);
}

Modules *Modules::_instance = 0;

Modules::Modules()
{
	utsname u;
	uname(&u);
	_modDir=QString("/lib/modules/") + u.release + "/";
}

bool Modules::load(QString name, QString args)
{
	char const * const fname=QFile::encodeName(filename(name));

	struct stat s;
	if(stat(fname, &s)<0) {
#ifdef DEBUG
		perror(fname);
#endif
		return false;
	}

	unsigned char mod[s.st_size];
	int fd=open(fname, O_RDONLY);
	if(fd<0) {
#ifdef DEBUG
		perror(fname);
#endif
		return false;
	}
	read(fd, mod, s.st_size);
	close(fd);
	char *cargs=strdup(args.isEmpty() ? "" : args.toLatin1().data());
	long ret=init_module(mod, s.st_size, cargs);
	free(cargs);
	if(ret!=0) {
		if(errno==EEXIST)
			// Module already loaded -- this is not a fatal
			// error
			return true;
		perror(fname);
		return false;
	}
	return true;
}

bool Modules::loadWithDeps(QString name, QString args)
{
	QFile modules("/proc/modules");
	if(!modules.open(QFile::ReadOnly)) {
		perror("modules");
	} else {
		// Prevent dependency loops -- abort if we're already loaded.
		QString mn=modname(name) + " ";
		QString line;
		while(!modules.atEnd()) {
			line=modules.readLine();
			if(line.startsWith(mn)) // Already loaded
				return true;
		}
		modules.close();
	}

	QString const fn=filename(name);
	QFile modulesDep(_modDir + "modules.dep");
	if(!modulesDep.open(QFile::ReadOnly)) {
		perror("modules.dep");
	} else {
		QString line;
		while(!modulesDep.atEnd()) {
			line=modulesDep.readLine();
			if(!line.contains(':'))
				continue;
			QString module=line.section(':', 0, 0);
			if(!module.startsWith('/'))
				module = _modDir + module;
			if(module != fn)
				continue;
			QStringList deps=line.section(':', 1).simplified().split(' ', QString::SkipEmptyParts);
			foreach(QString dep, deps) {
				if(!dep.startsWith('/') && dep.contains('/'))
					dep = _modDir + dep;
				loadWithDeps(dep);
			}
		}
		modulesDep.close();
	}
	return load(fn, args);
}

QString Modules::filename(QString const &name)
{
	if(name.startsWith('/')) // Already a path... good
		return name;
	else if(name.contains('/')) // Relative path -- also good
		return _modDir + name;

	// This is pretty bad, we have to search modules.dep (or worse, the
	// _modDir filesystem tree) for the module...
	QString fn(name);
	if(!fn.endsWith(".ko"))
		fn += ".ko";

	QFile modulesDep(_modDir + "modules.dep");
	if(!modulesDep.open(QFile::ReadOnly)) {
		perror("modules.dep");
		return name;
	}
	QString line;
	while(!modulesDep.atEnd()) {
		line=modulesDep.readLine();
		if(!line.contains(':'))
			continue;
		QString module=line.section(':', 0, 0);
		if(module.section('/', -1) == fn) {
			if(!module.startsWith('/'))
				module = _modDir + module;
			modulesDep.close();
			return module;
		}
	}
	modulesDep.close();
	return fn;
}

QString Modules::modname(QString const &file)
{
	QString result=file.section('/', -1);
	if(result.endsWith(".ko"))
		result=result.left(result.length()-3);
	result.replace(QChar('-'), "_");
	return result;
}

bool Modules::unload(char const * const name, bool force, bool wait)
{
	// TODO Check /proc/modules for module usage
	return delete_module(name, (force ? O_TRUNC : 0) | (wait ? 0 : O_NONBLOCK)) == 0;
}

Modules *Modules::instance()
{
	if(!_instance)
		_instance=new Modules();
	return _instance;
}
