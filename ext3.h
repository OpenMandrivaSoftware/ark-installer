/* This file is part of the Ark Linux installer.
 * It is released under the terms of the
 * GNU General Public License (GPL).
 * For exceptions, contact bero@arklinux.org
 */

#ifndef _ARK_EXT3_H_
#define _ARK_EXT3_H_ 1
#include <QString>
#include <cstdio> // Required by ext2fs.h ATM (for FILE*)
#include <cstring> // memcpy() required by ext2fs.h
#include <ext2fs/ext2fs.h>

/** @class Ext3FS ext3.h ext3.h
 * Handling of ext3 filesystems
 * @author Bernhard Rosenkraenzer <bero@arklinux.org>
 */
class Ext3FS
{
public:
	/** Create an ext3 filesystem handler
	 * @param dev partition device name (e.g. /dev/hda1)
	 */
	Ext3FS(QString const &dev);
	/** Destructor. Don't call this manually.
	 */
	~Ext3FS();
	/** Add a journal to the filesystem.
	 * This method adds a journal to a filesystem,
	 * effectively converting an ext2 filesystem to
	 * ext3.
	 * @param size Size of journal or @c -1 for default
	 */
	void addJournal(int size=-1);
	/** Set the check interval (time).
	 * This method controls the check interval (force
	 * detailed file system checks every X days)
	 * @param interval interval or 0 for no forced checks
	 */
	void setCheckInterval(int interval=0);
	/** Set the check interval (mounts).
	 * This method controls the check interval (force
	 * detailed file system checks every X mounts)
	 * @param mountcount mount count or -1 for no forced checks
	 */
	void setCheckMountcount(int mountcount=-1);
	/** Enable/disable directory indexing (dir_index)
	 * @param dir_index @c true to enable, @c false to disable
	 */
	void setDirIndex(bool dir_index=true);
private:
	/** Get the default journal size for a given
	 * partition size
	 * @param size partition size
	 */
	int journalSize(int size=-1);
	ext2_filsys fs;
	ext2_super_block *sb;
};
#endif
