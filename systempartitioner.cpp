// Grab all available diskspace for Linux
// (c) 2002-2011 Bernhard Rosenkraenzer <bero@arklinux.org>
#include "systempartitioner.moc"
#include "partedtimer.h"
#include "ext3.h"
#include "meminfo.h"
#include "modules.h"
#include <QMessageBox>
#include <QProcess>
#include <QFile>
#include <fcntl.h>
#include <errno.h>

//#define PARTDEBUG
#ifdef PARTDEBUG
#define MSG(x) QMessageBox::information(0, "debug", x);
#else
#define MSG(x) { }
#endif

extern "C" {
#include <parted/parted.h>
}
SystemPartitioner::SystemPartitioner(QWidget *parent, Qt::WindowFlags f):Partitioner(parent, f)
{
}
void SystemPartitioner::resizeEvent(QResizeEvent *e)
{
	if(e)
		Partitioner::resizeEvent(e);
	if(!help)
		return;
	help->setGeometry(10, 10, width()-20, help->sizeHint().height());
	if(!progress)
		return;
	progress->setGeometry(10, 10+help->height()+help->y(), width()-20, progress->sizeHint().height());
}
void SystemPartitioner::init()
{
#if NONDESTRUCTIVE
	emit done();
	return;
#endif
	// Check # of harddisks to see if RAID0-ing them makes any sense
	ped_device_probe_all();
	PedDevice *tmp=NULL;
	int disks=0;
	QString installsource(getenv("INSTALLSOURCE"));
	MSG("Install source is " + installsource);
	while((tmp=ped_device_get_next(tmp))) {
		// FIXME workaround for parted breakage
		// Skip CD-ROMs parted accidentally marks as harddisk
		QString p(tmp->path);
		if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || access(QFile::encodeName("/proc/ide/" + p.section('/', 2, 2) + "/cache"), R_OK)))
			continue;
		// It's not a good idea to install on a USB stick we're installing from...
		if(installsource.startsWith(p))
			continue;
		MSG(QString("Found disk: ") + QString(tmp->path) + ":" + "/proc/ide/" + p.section('/', 2, 2) + "/" + "cache" + ":" + QString::number(access(QFile::encodeName("/proc/ide/" + p.section('/', 2, 2) + "/cache"), R_OK)));
#if 0
		// Check if the drive is actually there -- some empty CF
		// readers misidentify themselves...
		int fd=open(tmp->path, O_RDONLY);
		if(fd < 0)
			continue;
		char test;
		if(read(fd, &test, 1) <= 0) {
			close(fd);
			continue;
		}
		close(fd);
#endif
		disks++;
	}
	ped_device_free_all();

	// Grab all disks
	ped_device_probe_all();

	PedFileSystemType const *ext3=ped_file_system_type_get("ext2"); // parted doesn't support ext3 creation yet, so we need this hack
	PedFileSystemType const *bootext3=ped_file_system_type_get("ext2");
	PedFileSystemType const *swap=ped_file_system_type_get("linux-swap");

	Meminfo m;
	unsigned long long swapsize=m.suggested_swap();
	QStringList raidPartitions;

	PedDevice *dev=NULL;
	PedPartition *fspart=NULL;
	PedGeometry *fsgeo=NULL;
	setHelp(tr("Removing other OSes..."));
	resizeEvent(0);

	uint32_t bootsize=0;
#ifndef NO_RAID0
	if(disks>1)
		bootsize=32*1024*2; /* size is in 512 kB blocks --> we use 32 MB */
#endif
	while((dev=ped_device_get_next(dev))) {
		// FIXME workaround for parted breakage
		QString p(dev->path);
		if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || access(QFile::encodeName("/proc/ide/" + p.section('/', 2, 2) + "/cache"), R_OK)))
			continue;
		// It's not a good idea to install on a USB stick we're installing from...
		if(installsource.startsWith(p))
			continue;
		//unsigned long long disksize=dev->length*dev->sector_size;
		PedDiskType *type=ped_disk_type_get("msdos");
		PedDisk *disk=ped_disk_new_fresh(dev, type);

		PedGeometry *part=ped_geometry_new(dev, 0, dev->length);
		if(swapsize && _swap.isEmpty() && ((unsigned long long)part->length > swapsize + bootsize)) {
			// Split disk in swap and fs partitions
			PedGeometry *swapgeo=ped_geometry_new(dev, 0, swapsize);
			PedGeometry *bootgeo=NULL;
			uint32_t fssize=part->end - swapsize;
			uint32_t fsstart=swapsize+1;
			if(bootsize) {
				bootgeo=ped_geometry_new(dev, swapsize+1, bootsize);
				fssize -= bootsize;
				fsstart += bootsize;
			}
			fsgeo=ped_geometry_new(dev, fsstart, fssize);

			PedPartition *swappart=ped_partition_new(disk, (PedPartitionType)0, swap, swapgeo->start, swapgeo->end);
			PedPartition *bootpart=NULL;
			if(bootsize)
				bootpart=ped_partition_new(disk, (PedPartitionType)0, bootext3, bootgeo->start, bootgeo->end);

			fspart=ped_partition_new(disk, (PedPartitionType)0, ext3, fsgeo->start, fsgeo->end);
			if(bootsize)
				ped_partition_set_flag(fspart, PED_PARTITION_RAID, 1);
			ped_disk_add_partition(disk, swappart, ped_constraint_any(dev));
			ped_disk_commit(disk);

			ped_geometry_destroy(swapgeo);
			
			setHelp(tr("Creating swap filesystem"));
			PedFileSystem *swapfs=ped_file_system_create(&(swappart->geom), swap, NULL /*timer->timer()*/);
			ped_file_system_close(swapfs);
			_swap = dev->path + QString::number(swappart->num);

			// Parted's swap creator is buggy
			QProcess::execute("/sbin/mkswap " + _swap);

			if(bootpart) {
				setHelp(tr("Creating boot filesystem"));
				ped_disk_add_partition(disk, bootpart, ped_constraint_any(dev));
				ped_disk_commit(disk);
				PedFileSystem *bootfs=ped_file_system_create(&(bootpart->geom), bootext3, timer->timer());
				ped_file_system_close(bootfs);
				ped_partition_set_flag(bootpart, PED_PARTITION_BOOT, 1);
				ped_geometry_destroy(bootgeo);

				QString devname=dev->path + QString::number(bootpart->num);
				_size.insert(devname, bootpart->geom.length);
				if(_rootfs == "ext3") {
					Ext3FS e3(devname);
					e3.addJournal(0);
					e3.setCheckInterval(0);
					e3.setCheckMountcount(-1);
					e3.setDirIndex();
				} else {
					QProcess::execute("/sbin/mkfs." + _rootfs + " " + _mkfsopts + " " + devname);
				}
				if(!_postmkfs.isEmpty())
					QProcess::execute(_postmkfs + " " + devname);

				_partitions.insert(devname, FileSystem("/boot", _rootfs));
			}
		} else {
			// Grab the whole disk for filesystem
			fsgeo=ped_constraint_solve_max(ped_constraint_any(dev));
			fspart=ped_partition_new(disk, (PedPartitionType)0, ext3, part->start, part->end);
			if(bootsize)
				ped_partition_set_flag(fspart, PED_PARTITION_RAID, 1);
		}
		ped_disk_add_partition(disk, fspart, ped_constraint_any(dev));
		if(!bootsize)
			ped_partition_set_flag(fspart, PED_PARTITION_BOOT, 1);
		ped_disk_commit(disk);
		if(!bootsize) {
			setHelp(tr("Creating Linux filesystem"));
			PedFileSystem *fs=ped_file_system_create(&(fspart->geom), ext3, timer->timer());
			_totalSize += fspart->geom.length;
			ped_file_system_close(fs);
		}
		ped_geometry_destroy(fsgeo);
		// Convert to ext3 and turn off checking
		QString devname=dev->path + QString::number(fspart->num);
		if(bootsize)
			raidPartitions.append(devname);
		else if(!_partitions.hasMountpoint("/"))
			_partitions.insert(devname, FileSystem("/", _rootfs));
		else {
			QString mp(devname);
			mp.replace("/dev", "/mnt");
			_partitions.insert(devname, FileSystem(mp, _rootfs));
		}
		_size.insert(devname, fspart->geom.length);
		if(!bootsize) {
			if(_rootfs == "ext3") {
				Ext3FS e3(devname);
				e3.addJournal(0);
				e3.setCheckInterval(0);
				e3.setCheckMountcount(-1);
				e3.setDirIndex();
			} else {
				QProcess::execute("/sbin/mkfs." + _rootfs + " " + _mkfsopts + " " + devname);
			}
			if(!_postmkfs.isEmpty())
				QProcess::execute(_postmkfs + " " + devname);
			ped_disk_destroy(disk);
		}
	}

	if(bootsize) {
		setHelp(tr("Combining disks..."));
		// Make sure we can read the array we're building first...
		Modules::instance()->loadWithDeps("md");
		Modules::instance()->loadWithDeps("raid0");
		// Now create it
		FILE *f=fopen("/tmp/mdadm.conf", "w");
		if(!f)
			QMessageBox::information(0, "debug", QString("Failed to create mdadm.conf: ") + strerror(errno));
		fprintf(f, "DEVICE partitions");
		fprintf(f, "ARRAY /dev/md0 name=ArkLinux devices=%s level=0 num-devices=%u\n", qPrintable(raidPartitions.join(",")), raidPartitions.count());
		fprintf(f, "MAILADDR root@localhost\n");
		fclose(f);

		QString command="/sbin/mdadm --create -e 1.2 --chunk=32 --level=0 --raid-devices=" + QString::number(raidPartitions.count()) + " --name=ArkLinux --force /dev/md0 " + raidPartitions.join(" ");
		QProcess::execute(command);
		
		if(_rootfs == "ext3") {
			QProcess::execute("/sbin/mke2fs -j /dev/md0");
			Ext3FS e3("/dev/md0");
			e3.setCheckInterval(0);
			e3.setCheckMountcount(-1);
			e3.setDirIndex();
		} else {
			QProcess::execute("/sbin/mkfs." + _rootfs + " " + _mkfsopts + " /dev/md0");
		}
		if(!_postmkfs.isEmpty())
			QProcess::execute(_postmkfs + " /dev/md0");

		_partitions.insert("/dev/md0", FileSystem("/", _rootfs));
		_size.clear();
		_size.insert("/dev/md0", _totalSize);
	}

	// We don't need a UI to take the whole system - we're done.
	emit done();
}
