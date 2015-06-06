// Yes, I know you get nicer code and fewer i18n problems by using layouts.
// This is supposed to work with a minimalized version of Qt/Embedded,
// though.
#include "installmode.moc"
#include "debug.h"
#include "dist.h"
#include <QFile>
#include <iostream>

extern "C" {
#include <parted/parted.h>
}

using namespace std;
InstallMode::InstallMode(QWidget *parent, Qt::WFlags f):QFrame(parent, f)
{
	setAutoFillBackground(true);
	bool haveAnything=false, haveUnpartitioned=false, haveFat=false;
	ped_device_probe_all();
	PedDevice *dev=NULL;
	while((dev=ped_device_get_next(dev))) {
		// FIXME workaround for parted breakage
		QString p(dev->path);
		MSG("Looking at " + p);
		if(!p.contains("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || access(QFile::encodeName("/proc/ide/" + p.section('/', 2, 2) + "/cache"), R_OK)))
			continue;
		PedDisk *disk=ped_disk_new(dev);
		PedPartition *part=NULL;
		while((part=ped_disk_next_partition(disk, part))) {
			MSG("Found a partition");
			if(part->type & PED_PARTITION_FREESPACE)
				haveUnpartitioned=true;
			else {
				haveAnything=true;
				if(part->fs_type && part->fs_type->name) {
					QString fs(part->fs_type->name);
					if(fs=="fat" || fs=="vfat" || fs=="fat16" || fs=="fat32")
						haveFat=true;
				}
			}
		}
	}
	MSG("Done scanning devices");
	ped_device_free_all();
	MSG("Freed memory");
	
	t=ERROR;
        setFrameShape((QFrame::Shape)(QFrame::Box|QFrame::Sunken));
	setLineWidth(3);
        setMidLineWidth(0);
	info=new QLabel(tr("<h1><b>Welcome to %1 %2!</b></h1><br><big>Please select your installation type.</big>").arg(DIST).arg(VERSION), this);
	SystemInstall=new QextRtPushButton(tr("<big><b>S&ystem install</b></big><br>This will use all disk space on <font color=\"red\">ALL</font> attached hard disks, overwriting any other operating system and data you might have installed."), this);
	ExpressInstall=new QextRtPushButton(tr("<big><b>E&xpress install</b></big><br>This will use all \"unpartitioned space\" (space not assigned to any other operating system/data) in your computer."), this);
	ParallelInstall=new QextRtPushButton(tr("<big><b>&Parallel install</b></big><br>An existing Windows or DOS drive is reduced in size to make room for Linux, leaving the data intact."), this);
	ExpertInstall=new QextRtPushButton(tr("<big><b>&Expert mode</b></big><br>You can modify and create partitions yourself. Use this only if you know what you're doing."), this);
	if(!haveUnpartitioned && haveAnything)
		ExpressInstall->setDisabled(true);
	if(!haveFat)
		ParallelInstall->setDisabled(true);
	connect(SystemInstall, SIGNAL(clicked()), this, SLOT(systemClicked()));
	connect(ExpressInstall, SIGNAL(clicked()), this, SLOT(expressClicked()));
	connect(ParallelInstall, SIGNAL(clicked()), this, SLOT(windowsClicked()));
	connect(ExpertInstall, SIGNAL(clicked()), this, SLOT(expertClicked()));
	ExpressInstall->setFocus();
	resizeEvent(0); // Set the geometry correctly.
}

void InstallMode::systemClicked()
{
	t=System;
	emit done(t);
}

void InstallMode::expressClicked()
{
	t=Express;
	emit done(t);
}

void InstallMode::windowsClicked()
{
	t=Windows;
	emit done(t);
}

void InstallMode::expertClicked()
{
	t=Expert;
	emit done(t);
}

void InstallMode::resizeEvent(QResizeEvent *e)
{
	if(e)
		QFrame::resizeEvent(e);
	int bh=max(max(max(SystemInstall->sizeHint().height(), ExpressInstall->sizeHint().height()), ParallelInstall->sizeHint().height()), ExpertInstall->sizeHint().height());
	int spacing=(height()-4*bh-info->sizeHint().height())/5;
	if(spacing<0) {
		bh /= 1.45;
		spacing=(height()-4*bh-info->sizeHint().height())/5;
	}
	int y=spacing;
	info->setGeometry(10, y, width()-20, info->sizeHint().height()); y += info->height();
	SystemInstall->setGeometry(10, y, width()-20, bh); y += bh+spacing;
	ExpressInstall->setGeometry(10, y, width()-20, bh); y += bh+spacing;
	ParallelInstall->setGeometry(10, y, width()-20, bh); y += bh+spacing;
	ExpertInstall->setGeometry(10, y, width()-20, bh); y += bh+spacing;
}
