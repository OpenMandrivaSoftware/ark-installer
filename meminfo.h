// Parse /proc/meminfo and return a sane swap size for
// an end user system.
// (c) 2002 Bernhard Rosenkraenzer <bero@arklinux.org>

#ifndef _MEMINFO_H_
#define _MEMINFO_H_ 1
#include <QMap>
#include <QString>
/**
 * Parse /proc/meminfo
 * @author Bernhard Rosenkraenzer <bero@arklinux.org>
 */
class Meminfo {
public:
	Meminfo() { update(); }
	unsigned long long total() const { return _values["MemTotal"]; }
	unsigned long long free() const { return _values["MemFree"]; }
	unsigned long long shared() const { return _values["MemShared"]; }
	unsigned long long buffers() const { return _values["Buffers"]; }
	unsigned long long cached() const { return _values["Cached"]; }
	unsigned long long swapCached() const { return _values["SwapCached"]; }
	unsigned long long active() const { return _values["Active"]; }
	unsigned long long inact_dirty() const { return _values["Inact_dirty"]; }
	unsigned long long inact_clean() const { return _values["Inact_clean"]; }
	unsigned long long inact_target() const { return _values["Inact_target"]; }
	unsigned long long hightotal() const { return _values["HighTotal"]; }
	unsigned long long highfree() const { return _values["HighFree"]; }
	unsigned long long lowtotal() const { return _values["LowTotal"]; }
	unsigned long long lowfree() const { return _values["LowFree"]; }
	unsigned long long swaptotal() const { return _values["SwapTotal"]; }
	unsigned long long swapfree() const { return _values["SwapFree"]; }
	unsigned long long committed_as() const { return _values["Committed_AS"]; }
	void update();
	unsigned long long operator[](QString const &s) const { return _values[s]; }
	/**
	 * Calculate suggested size of a swap partition, in disk sectors.
	 */
	unsigned long long suggested_swap() const;
private:
	QMap<QString, unsigned long long> _values;
};
#endif
