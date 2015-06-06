#include "meminfo.h"
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <stdint.h>
#include <iostream> // Not needed, but useful for adding debug messages
#define max(x,y) x > y ? x : y
#define min(x,y) x > y ? y : x
void Meminfo::update()
{
	_values.clear();
	FILE *f=fopen("/proc/meminfo", "r");
	if(!f) {
		// YUCK... Make sure /proc is what it should be.
		mkdir("/proc", 0555);
		mount("none", "/proc", "proc", 0, NULL);
		Q_ASSERT(f=fopen("/proc/meminfo", "r"));
	}
	char buf[1024];
	while(!feof(f)) {
		QString currentLine = fgets(buf, 1024, f);
		// We care only about the lines in Foo: X kB format, skip the rest
		if(currentLine.contains(':')) {
			QString key=currentLine.section(':', 0, 0);
			QString value=currentLine.section(':', 1).simplified();
			if(value.right(3) == " kB")
				value=value.left(value.length()-3);
			if(!value.contains(' '))
				_values[key]=value.toULong();
		}
	}
	fclose(f);
}

// Suggested swap size in disk sectors.
// This is meant for a desktop system, and should not be used for a server
// (which usually needs significantly more swap).
// The basic idea is to get at least 512 MB memory to ensure memory intensive
// apps run will, while not wasting disk space on ideas like "swap = 2 * memory".
unsigned long long Meminfo::suggested_swap() const
{
	int64_t swapsize=-1;
	// First of all, check if the user requested something...
	QString cmdline;
	FILE *f=fopen("/proc/cmdline", "r");
	if(f) {
		char buf[1024];
		fgets(buf, 1024, f);
		fclose(f);
		cmdline=buf;
	}
	QStringList cmds=cmdline.split(' ');
	for(QStringList::ConstIterator it=cmds.begin(); it!=cmds.end(); ++it) {
		if((*it).startsWith("swap=")) // swap= takes MBs, we need 512k blocks
			swapsize=(*it).section('=', 1, 1).toUInt()*1024*2;
		else if((*it)=="noswap")
			swapsize=0;
	}
	if(swapsize>=0)
		return swapsize;
	
	unsigned long long swap_kb;
	// There's no point in wasting diskspace on giant swap partitions
	// unless they're hibernation partitions at the same time. Given the
	// eternally experimental state of hibernation, we don't support it
	// by default.
	if(total() >= 1048576)         // We already have loads of memory - a small swap partition is sufficient
		swap_kb = 262144;
	else
		swap_kb = 1048576 - total();
	// Convert to sectors: simply multiply by 2 (sector = 512 bytes)
	return swap_kb*2;
}
