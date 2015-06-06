#include "expertpartitioner.moc"
#include "partedtimer.h"
#include <QApplication>
#include <QLabel>
#include <QMessageBox>
#include <QFile>
#include <QProcess>
#include "meminfo.h"

#include <iostream>
using namespace std;

extern "C" {
#include <parted/parted.h>
// Some immensely broken versions of libext2fs use
// "private" as a variable name in the public API
// Workaround...
#define private privat
#include <ext2fs/ext2_fs.h>
#include <ext2fs/ext2fs.h>
#undef private
}
#include "ext3.h"
#define max(x,y) x > y ? x : y
#include "dist.h"

#ifdef DEBUG
#define MSG(x) QMessageBox::information(0, "debug", x);
#else
#define MSG(x) { }
#endif

ExpertPartitioner::ExpertPartitioner(QWidget *parent, Qt::WindowFlags f):Partitioner(parent, f),_layout(this)
{
	_layout.setSpacing(5);
	_layout.setMargin(10);
	_layout.addWidget(_rerunQtParted=new QextRtPushButton(tr("Rerun partition editor"), this));
	connect(_rerunQtParted, SIGNAL(clicked()), this, SLOT(qtParted()));
	_layout.addWidget(_mpLabel=new QLabel(tr("Mount points:"), this));
	_layout.addWidget(_mountPoints=new QTableWidget(this));
	_layout.addWidget(_grubLayout=new QWidget(this));
	QHBoxLayout *grubLayout=new QHBoxLayout(_grubLayout);
	grubLayout->addWidget(_grubLbl=new QLabel(tr("Install boot loader to:"), _grubLayout));
	grubLayout->addWidget(_grub=new QComboBox(_grubLayout));
	_grub->addItems(QStringList() << tr("Master Boot Record (MBR)") << tr("Partition boot sector") << tr("Don't install boot loader"));
	_layout.addWidget(_proceed=new QextRtPushButton(tr("Proceed"), this));
	connect(_proceed, SIGNAL(clicked()), this, SLOT(proceed()));
}
void ExpertPartitioner::init()
{
#if NONDESTRUCTIVE
	emit done();
	return;
#endif
}

void ExpertPartitioner::proceed()
{
	bool haveRoot=false;
	QStringList dupes;
	_mountPoints->setCurrentCell(0, 0); // Necessary to make sure the currently selected cell is updated in text() results
	_partitions.clear();
	for(int i=0; i<_mountPoints->rowCount(); i++) {
		if(_mountPoints->item(i, 1)->text().isEmpty())
			continue;
		
		for(int j=0; j<i; j++)
			if(_mountPoints->item(i, 1)->text() == _mountPoints->item(j, 1)->text() && !_mountPoints->item(i, 2)->text().startsWith("linux-swap") && _mountPoints->item(i, 1)->text() != "swap") {
				dupes.append(_mountPoints->item(i, 1)->text());
			}
		// FIXME determine partition type
		QString fs(_rootfs);
		if(_fs.contains(_mountPoints->item(i, 0)->text()))
			fs=_fs[_mountPoints->item(i, 0)->text()];
		_partitions.insert(_mountPoints->item(i, 0)->text(), FileSystem(_mountPoints->item(i, 1)->text(), fs));
		if(_mountPoints->item(i, 1)->text()=="/") {
			haveRoot=true;
		}
	}
	if(!dupes.isEmpty())
		QMessageBox::information(0, tr("Invalid data"), tr("The mount point %1 is assigned several times.").arg(dupes.join(" ")));
	else if(!haveRoot)
		QMessageBox::information(0, tr("Invalid data"), tr("You need a / partition."));
	else {
		_rerunQtParted->hide();
		_grubLayout->hide();
		_mpLabel->setText(tr("Formatting partitions"));
		_mountPoints->hide();
		_proceed->hide();
		while(qApp->hasPendingEvents())
			qApp->processEvents();

		QStringList formatParts;
		for(int i=0; i<_mountPoints->rowCount(); i++) {
			QTableWidgetItem *it=static_cast<QTableWidgetItem*>(_mountPoints->item(i, 2));
			if(it && it->checkState())
				formatParts.append(_mountPoints->item(i, 0)->text());
		}

		ped_device_probe_all();
		PedDevice *dev=NULL;

		while((dev=ped_device_get_next(dev))) {
			// FIXME workaround for parted breakage
			QString p(dev->path);
			if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || !QFile::exists("/proc/ide/" + p.section('/', 2, 2) + "/cache")))
				continue;
			PedDisk *disk=ped_disk_new(dev);
			if(!disk) // Unpartitioned disk
				continue;
			PedPartition *part=NULL;
			while((part=ped_disk_next_partition(disk, part))) {
				if((part->fs_type && part->fs_type->name) && ((part->type==0 || (part->type & PED_PARTITION_LOGICAL)))) {
					QString path(dev->path + QString::number(part->num));
					QString filesystem(_partitions[path].filesystem);
					if(formatParts.contains(path) || filesystem=="swap" || filesystem.startsWith("linux-swap")) {
						bool convertExt4(false);
						if(filesystem.isEmpty() || filesystem=="ext3" || filesystem=="ext4") {
							filesystem="ext2";
							convertExt4=true;
						} else if(filesystem=="swap")
							filesystem="linux-swap";

						/*
						PedFileSystemType *fstype=ped_file_system_type_get(filesystem);
						PedFileSystem *fs=ped_file_system_create(&(part->geom), fstype, timer->timer());
						ped_file_system_close(fs);
						*/
						QString const partdev(dev->path + QString::number(part->num));
						if(convertExt4) {
/*							MSG("Converting EXT3");
							Ext3FS e3(partdev);
							e3.addJournal(0);
							e3.setCheckInterval(0);
							e3.setCheckMountcount(-1);
							e3.setDirIndex(); */
							QProcess::execute("/sbin/mke2fs -j -O extent,uninit_bg,dir_index,filetype,flex_bg,sparse_super " + partdev);
							QProcess::execute("/sbin/tune2fs -c -1 -i 0 -m 1 " + partdev);
						} else if(filesystem == "jfs") {
							QProcess::execute("/sbin/mkfs.jfs -q " + partdev);
						} else if(filesystem == "xfs") {
							QProcess::execute("/sbin/mkfs.xfs -f " + partdev);
						} else if(filesystem == "reiserfs") {
							QProcess::execute("/sbin/mkfs.reiserfs -q -f 3.6 " + partdev);
						} else if(filesystem == "btrfs") {
							QProcess::execute("/sbin/mkfs.btrfs " + partdev);
						} else if(filesystem != "ext2" && !filesystem.startsWith("linux-swap")) {
							QProcess::execute("/sbin/mkfs." + _rootfs + " " + _mkfsopts + " " + partdev);
						} else if(filesystem.contains("swap")) // parted's swap generator is buggy
							QProcess::execute("/sbin/mkswap " + partdev);
						if(!_postmkfs.isEmpty())
							QProcess::execute(_postmkfs + " " + partdev);
					}
				}
			}
			ped_disk_destroy(disk);
		}

		if(_grub->currentText() == tr("Partition boot sector"))
			_grubLocation = Partition;
		else if(_grub->currentText() == tr("Don't install boot loader"))
			_grubLocation = Nowhere;
		
		emit done();
	}
}

void ExpertPartitioner::show()
{
	qtParted();
	Partitioner::show();
}

void ExpertPartitioner::qtParted()
{
#ifdef Q_WS_QWS
	QProcess::execute("qtparted-nox");
#else
	QProcess::execute("qtparted");
#endif

	//MSG("Generating mount point selection UI");
	_mountPoints->clearContents();
	_mountPoints->setRowCount(0);
	_mountPoints->setColumnCount(3);
	_mountPoints->setHorizontalHeaderLabels(QStringList() << tr("Device") << tr("Mount point") << tr("Format"));

	// Rescan all disks
	_fs.clear();
	//MSG("Rescanning partition tables");
	ped_device_probe_all();
	PedDevice *dev=NULL;
	bool haveRoot=false;

	while((dev=ped_device_get_next(dev))) {
		//MSG(QString("Looking at ") + dev->path);
		// FIXME workaround for parted breakage
		QString p(dev->path);
		if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || !QFile::exists("/proc/ide/" + p.section('/', 2, 2) + "/cache")))
			continue;
		PedDisk *disk=ped_disk_new(dev);
		if(!disk) // Unpartitioned disk
			continue;
		PedPartition *part=NULL;
		while((part=ped_disk_next_partition(disk, part))) {
			if((part->fs_type && part->fs_type->name) && ((part->type==0 || (part->type & PED_PARTITION_LOGICAL)))) {
				QString fs(part->fs_type->name);
				// Change some filesystem names to what
				// mount() expects...
				if(fs=="fat16" || fs=="fat32") {
					fs="vfat";
				} else if(fs=="ext2" || fs=="ext3") { // Parted can't create ext4 -- this is probably what the user meant to do though...
					fs="ext4";
					Ext3FS e3(dev->path + QString::number(part->num));
					e3.addJournal(0);
					e3.setCheckInterval(0);
					e3.setCheckMountcount(-1);
					e3.setDirIndex();
				}
				_fs.insert(p + QString::number(part->num), fs);
				bool swap=(fs.startsWith("linux-swap"));
				_mountPoints->setRowCount(_mountPoints->rowCount()+1);
				QTableWidgetItem *dev=new QTableWidgetItem(p + QString::number(part->num));
				dev->setFlags(Qt::NoItemFlags);
				_mountPoints->setItem(_mountPoints->rowCount()-1, 0, dev);
				QTableWidgetItem *mountPoint=new QTableWidgetItem(QString::null);
				mountPoint->setFlags(swap ? Qt::NoItemFlags : Qt::ItemIsEditable|Qt::ItemIsEnabled);
				QTableWidgetItem *format=new QTableWidgetItem(QString::null);
				format->setFlags(Qt::ItemIsUserCheckable|Qt::ItemIsEnabled);
				if(swap) {
					mountPoint->setText(tr("swap"));
					format->setCheckState(Qt::Checked);
				}
				// geom.length is in 512 kB-blocks, DISTSIZE in MB
				else if(!haveRoot && fs != "vfat" && fs != "ntfs" && part->geom.length >= DISTSIZE * 2) {
					mountPoint->setText("/");
					haveRoot=true;
					format->setCheckState(Qt::Checked);
				}
				_mountPoints->setItem(_mountPoints->rowCount()-1, 1, mountPoint);
				_mountPoints->setItem(_mountPoints->rowCount()-1, 2, format);
			}
		}
		ped_disk_destroy(disk);
	}
	//MSG("Partition rescan done");
}
