/* Ext3 C++ wrapper
 * (c) 2001-2011 Bernhard Rosenkraenzer <bero@arklinux.org>
 * Parts adapted from tune2fs 1.23 source code (c) 1995-2000 Theodore Ts'o
 */
#include "ext3.h"
#include <QFile>
#include <stdint.h>

Ext3FS::Ext3FS(QString const &dev)
{
	ext2fs_open(QFile::encodeName(dev), EXT2_FLAG_RW, 0, 0, unix_io_manager, &fs);
	sb=fs->super;
	fs->flags |= EXT2_FLAG_SUPER_ONLY;
}

Ext3FS::~Ext3FS()
{
	ext2fs_close(fs);
}

void Ext3FS::addJournal(int size)
{
	unsigned long journal_blocks;
	if(sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL)
		return;
	// Create Journal inode
	journal_blocks = journalSize(size);
	ext2fs_add_journal_inode(fs, journal_blocks, 0);
	fs->flags &= ~EXT2_FLAG_SUPER_ONLY;
}

int Ext3FS::journalSize(int size)
{
	if(fs->super->s_blocks_count < 2048) // FS too small
		return 0;
	if(size>=0) {
		uint32_t j_blocks = size * 1024 / (fs->blocksize / 1024);
		if(j_blocks < 1024)
			j_blocks = 1024;
		else if(j_blocks > 102400)
			j_blocks = 102400;
		if(j_blocks > sb->s_free_blocks_count)
			return 0;
		return j_blocks;
	}
	if(sb->s_blocks_count < 32768)
		return 1024;
	else if(sb->s_blocks_count < 262144)
		return 4096;
	else
		return 8192;
}

void Ext3FS::setCheckInterval(int interval)
{
	sb->s_checkinterval = interval;
	ext2fs_mark_super_dirty(fs);
}

void Ext3FS::setCheckMountcount(int mountcount)
{
	sb->s_max_mnt_count = mountcount;
	ext2fs_mark_super_dirty(fs);
}

void Ext3FS::setDirIndex(bool dir_index)
{
	if(dir_index && !(sb->s_feature_compat & EXT2_FEATURE_COMPAT_DIR_INDEX)) {
		sb->s_feature_compat |= EXT2_FEATURE_COMPAT_DIR_INDEX;
		ext2fs_mark_super_dirty(fs);
	} else if(!dir_index && (sb->s_feature_compat & EXT2_FEATURE_COMPAT_DIR_INDEX)) {
		sb->s_feature_compat &= ~EXT2_FEATURE_COMPAT_DIR_INDEX;
		ext2fs_mark_super_dirty(fs);
	}
}
