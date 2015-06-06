#include "expresspartitioner.moc"
extern "C" {
#include <parted/parted.h>
}
#include "meminfo.h"
// Some immensely broken versions of libext2fs use
// "private" as a variable name in the public API
// Workaround...
#define private privat
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#undef private
#include "ext3.h"
#include "dist.h"
#include <QMessageBox>
#include <QProcess>
#include <QFile>

#define MAX(x,y) x > y ? x : y

ExpressPartitioner::ExpressPartitioner(QWidget *parent, Qt::WindowFlags f):Partitioner(parent, f),SystemInstall(0L),WindowsInstall(0L),Abort(0L)
{
}
void ExpressPartitioner::resizeEvent(QResizeEvent *e)
{
	if(e)
		Partitioner::resizeEvent(e);
	if(!help)
		return;

	int y=10;
	help->setGeometry(10, y, width()-20, help->sizeHint().height()); y+= help->height()+10;
	if(SystemInstall && WindowsInstall && Abort) {
		int bh=MAX(MAX(SystemInstall->sizeHint().height(), WindowsInstall->sizeHint().height()),Abort->sizeHint().height());
		SystemInstall->setGeometry(10, y, width()-20, bh); y+=SystemInstall->height()+10;
		WindowsInstall->setGeometry(10, y, width()-20, bh); y+=WindowsInstall->height()+10;
		Abort->setGeometry(10, y, width()-20, bh);
		SystemInstall->show();
		WindowsInstall->show();
		Abort->show();
	} else if(progress) {
		progress->setGeometry(10, y, width()-20, progress->sizeHint().height());
	}
}
void ExpressPartitioner::init()
{
#if NONDESTRUCTIVE
	emit done();
	return;
#endif
	// Scan all disks
	ped_device_probe_all();
	PedDevice *dev=NULL;
	PedGeometry *fsgeo=NULL;
	PedFileSystemType const *ext3=ped_file_system_type_get("ext2"); // parted doesn't support ext3 creation yet :/
	PedFileSystemType const *swap=ped_file_system_type_get("linux-swap");

	Meminfo m;
	unsigned long long swapsize=m.suggested_swap();
	
	PedPartition *fspart=NULL;
	setHelp(tr("Looking for free space..."));
	resizeEvent(0);
	QString installsource(getenv("INSTALLSOURCE"));
	while((dev=ped_device_get_next(dev))) {
		// FIXME workaround for parted breakage
		QString p(dev->path);
		if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || !QFile::exists("/proc/ide/" + p.section('/', 2, 2) + "/cache")))
			continue;
		if(installsource.startsWith(p)) // It's not a good idea to install to the memory stick we're installing from
			continue;
		PedDisk *disk=ped_disk_new(dev);
		PedPartition *part=NULL;
		while((part=ped_disk_next_partition(disk, part))) {
			// Some ****ed Windoze installations leave the 1st
			// cylinder blank. Don't grab that space.
			// Furthermore, some ultimately broken proprietary tools (PartitionMagic)
			// leave the last cylinder blank for no reason whatsoever.
			if((part->type & PED_PARTITION_FREESPACE) && (part->geom.length >= 32768) && (part->geom.start != part->geom.end)) {
//DEBUG				QMessageBox::information(0, "PartitionMagic is broken!", "Grabing partition table:\nStart: " + QString::number(int(part->geom.start)) + "\nLength: " + QString::number(int(part->geom.length)) + "\nEnd: " + QString::number(int(part->geom.end)));
				PedGeometry geo=part->geom;
				PedPartitionType p((PedPartitionType(0)));
				if(part->type & PED_PARTITION_LOGICAL)
					p = PED_PARTITION_LOGICAL;
				if(swapsize && _swap.isEmpty() && ((unsigned long long)part->geom.length > swapsize)) { // Allocate a swap partition
					PedGeometry *swapgeo=ped_geometry_new(dev, geo.start, swapsize);
					fsgeo=ped_geometry_new(dev, geo.start+swapsize+1, geo.end-geo.start-swapsize);
					PedPartition *swappart=ped_partition_new(disk, p, swap, swapgeo->start, swapgeo->end);
					ped_disk_add_partition(disk, swappart, ped_constraint_any(dev));
					ped_disk_commit(disk);
					setHelp("Creating swap filesystem");
					PedFileSystem *swapfs=ped_file_system_create(&(swappart->geom), swap, timer->timer());
					ped_file_system_close(swapfs);
					ped_geometry_destroy(swapgeo);
					_swap=dev->path + QString::number(swappart->num);
					// parted's swap generator is buggy
					QProcess::execute("/sbin/mkswap " + _swap);

					fspart=ped_partition_new(disk, p, ext3, fsgeo->start, fsgeo->end);
				} else {
					fspart=ped_partition_new(disk, p, ext3, geo.start, geo.end);
				}
				ped_disk_add_partition(disk, fspart, ped_constraint_any(dev));
				ped_partition_set_flag(fspart, PED_PARTITION_BOOT, 1);
				ped_disk_commit(disk);
				setHelp("Creating Linux filesystem");
				PedFileSystem *fs=ped_file_system_create(&(fspart->geom), ext3, timer->timer());
				_totalSize += fspart->geom.length;
				ped_file_system_close(fs);
				ped_geometry_destroy(fsgeo);
				// Convert to ext3 and turn off checking
				QString devname=dev->path + QString::number(fspart->num);
				if(_partitions.hasMountpoint("/")) {
					QString mp(devname);
					mp.replace("/dev", "/mnt");
					_partitions.insert(devname, mp);
				} else
					_partitions.insert(devname, FileSystem("/", _rootfs));
				_size.insert(devname, fspart->geom.length);
				if(_rootfs == "ext3") {
					Ext3FS e3(devname);
					e3.addJournal(0);
					e3.setCheckInterval(0);
					e3.setCheckMountcount(-1);
					e3.setDirIndex();
				} else
					QProcess::execute("/sbin/mkfs." + _rootfs + " " + _mkfsopts + " " + devname);
				if(!_postmkfs.isEmpty())
					QProcess::execute(_postmkfs + " " + devname);
				break;
			} else if(part->type & PED_PARTITION_FREESPACE) {
//DEBUG				QMessageBox::information(0, "PartitionMagic is broken!", "Skipped broken PartitionMagic mess:\nStart: " + QString::number(int(part->geom.start)) + "\nLength: " + QString::number(int(part->geom.length)) + "\nEnd: " + QString::number(int(part->geom.end)));
			} else if((part->type==0 || (part->type & PED_PARTITION_LOGICAL)) && part->fs_type && part->fs_type->name && QString(part->fs_type->name).startsWith("fat", Qt::CaseInsensitive)) {
				// Check if it's a DOS partition we might want to nuke

				_dosSize += part->geom.length;
			}
		}
		ped_disk_destroy(disk);
//		ped_device_close(dev);
	}
	if(_totalSize > DISTSIZE) {
		emit done();
	} else {
		setHelp(tr("You do not have a sufficient amount of unallocated space. You need at least %1 megabytes of free diskspace.").arg(QString::number(DISTSIZE)));
		SystemInstall=new QextRtPushButton(tr("Restart in &system install mode (This will delete all other operating systems)"), this);
		connect(SystemInstall, SIGNAL(clicked()), this, SIGNAL(restartSys()));
		WindowsInstall=new QextRtPushButton(tr("Restart in &parallel install mode (This will resize an existing Windows or DOS partition to make room for Linux)"), this);
		connect(WindowsInstall, SIGNAL(clicked()), this, SIGNAL(restartWin()));
		Abort=new QextRtPushButton(tr("&Abort installation"), this);
		connect(Abort, SIGNAL(clicked()), this, SLOT(abort()));
		resizeEvent(0);
	}
}
