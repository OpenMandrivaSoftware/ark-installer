#include "windowspartitioner.moc"
#include "partedtimer.h"
#include <QLabel>
#include <QFile>
#include <QProcess>
#include "meminfo.h"
extern "C" {
#include <parted/parted.h>
}
// Some immensely broken versions of libext2fs use
// "private" as a variable name in the public API
// Workaround...
#define private privat
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#undef private
#include "ext3.h"
#define max(x,y) x > y ? x : y
#include "dist.h"
WindowsPartitioner::WindowsPartitioner(QWidget *parent, Qt::WindowFlags f):Partitioner(parent, f),_status(Starting),retain(0L),win_dev(0)
{
}
void WindowsPartitioner::init()
{
#if NONDESTRUCTIVE
	emit done();
	return;
#endif
	// Scan all disks
	ped_device_probe_all();
	PedDevice *dev=NULL;

	int winparts=0;
	
	while((dev=ped_device_get_next(dev))) {
		// FIXME workaround for parted breakage
		QString p(dev->path);
		if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || !QFile::exists("/proc/ide/" + p.section('/', 2, 2) + "/cache")))
			continue;
		PedDisk *disk=ped_disk_new(dev);
		PedPartition *part=NULL;
		while((part=ped_disk_next_partition(disk, part))) {
			if((part->type==0 || (part->type & PED_PARTITION_LOGICAL)) && QString(part->fs_type->name).left(3).toLower() == "fat") {
				PedFileSystem *fs=ped_file_system_open(&part->geom);
				if(fs) {
					PedConstraint *min=ped_file_system_get_resize_constraint(fs);
					windrive[winparts]=dev->path;
					winpart[winparts]=part->num;
					winsize[winparts]=part->geom.length/2048;
					minsize[winparts++]=min->min_size/2048;
					ped_file_system_close(fs);
				}
			}
		}
		ped_disk_destroy(disk);
	}

	if(winparts==0) {
		_status=NoWin;
		setHelp(tr("Your system doesn't seem to have any resizable Windows drives (this may happen, for example, if you're using Windows NT, 2000 or XP and the NTFS filesystem)."));
		SystemInstall=new QextRtPushButton(tr("Restart in &system install mode (this will delete Windows)"), this);
		connect(SystemInstall, SIGNAL(clicked()), this, SIGNAL(restartSys()));
		ExpressInstall=new QextRtPushButton(tr("Restart in e&xpress install mode (uses only \"unpartitioned\" space)"), this);
		connect(ExpressInstall, SIGNAL(clicked()), this, SIGNAL(restartExp()));
		Abort=new QextRtPushButton(tr("&Abort installation"), this);
		connect(Abort, SIGNAL(clicked()), this, SLOT(abort()));
	} else if(winparts==1) {
		drv_resize();
	} else {
		_status=MultiWin;
		setHelp(tr("You have several Windows drives. Please select the one you wish to resize."));
		drives=new QComboBox(this);
		for(char drive='C'; drive<'C'+winparts; drive++)
			drives->addItem(tr("%1: [%2 MB]").arg(QString(QChar(drive))).arg(QString::number(winsize[drive-'C'])));
		ok=new QextRtPushButton(tr("&Continue"), this);
		connect(ok, SIGNAL(clicked()), this, SLOT(driveSelected()));
	}
	resizeEvent(0);
}
void WindowsPartitioner::resizeEvent(QResizeEvent *e)
{
	if(e)
		Partitioner::resizeEvent(e);
	if(!help)
		return;
	int y=10;
	help->setGeometry(10, y, width()-20, help->sizeHint().height()); y+=help->height()+10;
	help->show();
	switch(_status) {
		case NoWin:
		case NoSpace: {
			int bh=max(max(SystemInstall->sizeHint().height(), ExpressInstall->sizeHint().height()), Abort->sizeHint().height());
			SystemInstall->setGeometry(10, y, width()-20, bh); y+=SystemInstall->height()+10;
			ExpressInstall->setGeometry(10, y, width()-20, bh); y+=ExpressInstall->height()+10;
			Abort->setGeometry(10, y, width()-20, bh);
			SystemInstall->show();
			ExpressInstall->show();
			Abort->show();
			break;
		} case OneWin: {
			if(retain) {
				retain->setGeometry(10, y, width()-20, retain->sizeHint().height()); y+=retain->height()+10;
				retain->show();
			}
			if(current) {
				current->setGeometry(10, y, width()-20, current->sizeHint().height()); y+=current->height()+10;
				current->show();
			}
			if(ok) {
				ok->setGeometry(10, y, width()-20, ok->sizeHint().height()); y+=ok->height()+10;
				ok->show();
			}
			if(progress)
				progress->setGeometry(10, y, width()-20, ok->sizeHint().height());
			break;
		} case MultiWin: {
			drives->setGeometry(10, y, width()-20, drives->sizeHint().height()); y+=drives->height()+10;
			ok->setGeometry(10, y, width()-20, ok->sizeHint().height());
			drives->show();
			ok->show();
			break;
		} case Resizing: {
			progress->setGeometry(10, y, width()-20, progress->sizeHint().height());
			progress->show();
			break;
		}
		default: // can't happen... in theory
			break;
	}
}
void WindowsPartitioner::driveSelected()
{
	QChar driveletter=drives->currentText().at(0);
	int drive=driveletter.toAscii()-'C';
	_status=Starting;
	help->hide();
	delete help;
	help=0;
	drives->hide();
	delete drives;
	ok->hide();
	delete ok;
	win_dev=drive;
	drv_resize();
	resizeEvent(0);
}
void WindowsPartitioner::drv_resize()
{
	if(winsize[win_dev]-minsize[win_dev] < DISTSIZE) {
		help=new QLabel(tr("Your Windows drive does not have sufficient free space.\nYou need at least %1 megabytes of free space.").arg(QString::number(DISTSIZE)), this);
		SystemInstall=new QextRtPushButton(tr("Restart in &system install mode (this will delete Windows)"), this);
		connect(SystemInstall, SIGNAL(clicked()), this, SIGNAL(restartSys()));
		ExpressInstall=new QextRtPushButton(tr("Restart in e&xpress install mode (uses only \"unpartitioned\" space)"), this);
		connect(ExpressInstall, SIGNAL(clicked()), this, SIGNAL(restartExp()));
		Abort=new QextRtPushButton(tr("&Abort installation"), this);
		connect(Abort, SIGNAL(clicked()), this, SLOT(abort()));
		_status=NoSpace;
		resizeEvent(0);
		return;
	}
	// First of all, get the exact constraints of the drive...

	setHelp(tr("Please select the amount of space you'd like to retain for Windows (in megabytes).<br><table border=\"0\" width=\"100%\"><tr><td align=\"left\">%1 MB</td><td align=\"right\">%2 MB</td></tr></table>").arg(QString::number(this->minsize[win_dev])).arg(QString::number(winsize[win_dev])));
	help->setTextFormat(Qt::RichText);
	retain=new QSlider(Qt::Horizontal, this);
	retain->setTickInterval((winsize[win_dev]-minsize[win_dev]-DISTSIZE)/20);
	retain->setRange(this->minsize[win_dev], winsize[win_dev]-DISTSIZE);
	retain->setTickPosition(QSlider::TicksBothSides);
	retain->setValue(minsize[win_dev]+25);
	connect(retain, SIGNAL(valueChanged(int)), this, SLOT(changed(int)));
	current=new QLabel(this);
	current->setTextFormat(Qt::RichText);
	changed(retain->value());
	ok=new QextRtPushButton(tr("&Continue"), this);
	connect(ok, SIGNAL(clicked()), this, SLOT(do_resize()));
	_status=OneWin;
}
void WindowsPartitioner::do_resize()
{
	PedDevice	*dev=ped_device_get(windrive[win_dev]);
	PedDisk		*disk=ped_disk_new(dev);
	PedPartition	*part=ped_disk_get_partition(disk, winpart[win_dev]);
	PedFileSystem	*fs=ped_file_system_open(&part->geom);
	int		old_end=part->geom.end;
	PedConstraint	*constraint=ped_file_system_get_resize_constraint(fs);
	PedGeometry	*newgeo=ped_geometry_new(dev, part->geom.start, retain->value()*2048);
	PedPartitionType type=(PedPartitionType)0;
	if(part->type & PED_PARTITION_LOGICAL)
		type = PED_PARTITION_LOGICAL;
	ped_disk_set_partition_geom(disk, part, constraint, newgeo->start, newgeo->end);

	if(retain) {

		retain->hide();
		current->hide();
		ok->hide();
		delete retain; retain=0;
		delete current; current=0;
		delete ok; ok=0;
	}

	setHelp(tr("Resizing filesystem... This may take a while."));
	_status=Resizing;
	resizeEvent(0);

	ped_file_system_resize(fs, &part->geom, timer->timer());
	ped_partition_set_system(part, fs->type); // Just in case (FAT16<->FAT32)
	ped_file_system_close(fs);
	ped_disk_commit(disk);
	ped_constraint_destroy(constraint);
	ped_geometry_destroy(newgeo);

	// Create Linux filesystems...
	Meminfo m;
	unsigned long long swapsize=m.suggested_swap();
	PedFileSystemType *swap=ped_file_system_type_get("linux-swap");
	PedFileSystemType *ext3=ped_file_system_type_get("ext2");
	PedGeometry *fsgeo=NULL;
	if(swapsize) {
		PedGeometry *swapgeo=ped_geometry_new(dev, part->geom.end, swapsize);
		fsgeo=ped_geometry_new(dev, part->geom.end+swapsize+1, old_end-part->geom.end-swapsize);
		PedPartition *swappart=ped_partition_new(disk, type, swap, swapgeo->start, swapgeo->end);
		ped_disk_add_partition(disk, swappart, ped_constraint_any(dev));
		ped_disk_commit(disk);
		setHelp(tr("Creating swap filesystem"));
		PedFileSystem *swapfs=ped_file_system_create(&(swappart->geom), swap, timer->timer());
		ped_file_system_close(swapfs);
		ped_geometry_destroy(swapgeo);
		_swap=dev->path + QString::number(swappart->num);
		// parted's swap generator is buggy
		QProcess::execute("/sbin/mkswap " + _swap);
	} else
		fsgeo=ped_geometry_new(dev, part->geom.end, old_end-part->geom.end);
	
	PedPartition *fspart=ped_partition_new(disk, type, ext3, fsgeo->start, fsgeo->end);
	ped_disk_add_partition(disk, fspart, ped_constraint_any(dev));
	ped_partition_set_flag(fspart, PED_PARTITION_BOOT, 1);
	ped_disk_commit(disk);
	setHelp(tr("Creating Linux filesystem"));
	PedFileSystem *ext3fs=ped_file_system_create(&(fspart->geom), ext3, timer->timer());
	delete timer;
	_totalSize += fspart->geom.length;
	ped_file_system_close(ext3fs);
	ped_geometry_destroy(fsgeo);
	// Convert to ext3 and turn off checking
	QString devname=dev->path + QString::number(fspart->num);
	if(_partitions.hasMountpoint("/")) {
		QString mp(devname);
		mp.replace("/dev", "/mnt");
		_partitions.insert(devname, FileSystem(mp, _rootfs));
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
	ped_disk_destroy(disk);
	//ped_device_close(dev);
	emit done();
}
void WindowsPartitioner::changed(int value)
{
	current->setText(tr("<left>Windows: %1 MB</left><right>Linux: %2 MB</right>").arg(QString::number(value)).arg(QString::number(winsize[win_dev]-value)));
}
