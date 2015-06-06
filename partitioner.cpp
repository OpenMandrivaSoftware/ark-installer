#include "partitioner.moc"
#include "modules.h"
#include <QApplication>
extern "C" {
#include <parted/parted.h>
static PedExceptionOption _parted_exception_handler(PedException* ex) {
	// Parted exceptions are way too complicated for end users
	// (e.g. the typical LBA warning for drives partitioned with Windoze)
	// Simply ignore them, unless they're important.
	if(ex->type==PED_EXCEPTION_FATAL)
		return PED_EXCEPTION_UNHANDLED;
	else if(ex->options & PED_EXCEPTION_FIX)
		return PED_EXCEPTION_FIX;
	else
		return PED_EXCEPTION_IGNORE;
}
}

Partitioner::Partitioner(QWidget *parent, Qt::WindowFlags f):QFrame(parent, f),help(0L),_totalSize(0),_dosSize(0),_swap(QString::null),_rootfs("jfs"),_mkfsopts(),_postmkfs(),_grubLocation(MBR)
{
	setAutoFillBackground(true);
	FILE *kcmd=fopen("/proc/cmdline", "r");
	if(kcmd) {
		while(!feof(kcmd) && !ferror(kcmd)) {
			char buf[1024];
			fgets(buf, 1024, kcmd);
			QString s=buf;
			if(s.contains("fs=")) {
				_rootfs=s.mid(s.indexOf("fs=")+3).section(' ', 0, 0).trimmed();
			}
		}
		fclose(kcmd);
	}
	if(_rootfs == "xfs")
		_mkfsopts = "-f";
	else if(_rootfs == "reiserfs")
		_mkfsopts = "-q -f 3.6";
	else if(_rootfs == "jfs")
		_mkfsopts = "-q";
	else if(_rootfs == "ext4") {
		_mkfsopts = "-j -O extent,uninit_bg,dir_index,filetype,flex_bg,sparse_super";
		_postmkfs = "/sbin/tune2fs -i 0 -c 0 -m 1";
	}
	
	// Make sure the fs module is loaded
	Modules::instance()->loadWithDeps(_rootfs);

	setFrameShape((QFrame::Shape)(QFrame::Box|QFrame::Sunken));
	setLineWidth(3);
	setMidLineWidth(0);

	ped_exception_set_handler(_parted_exception_handler);

	timer=new PartedTimer();
	connect(timer, SIGNAL(info(PedTimer*)), this, SLOT(updateProgress(PedTimer*)));

	progress=new QProgressBar(this);
	progress->setRange(0, 100);
	progress->hide();

	_partitions.clear();
}

Partitioner::~Partitioner()
{
}

void Partitioner::updateProgress(PedTimer *t)
{
	progress->setValue(t->frac*100);
	progress->show();
	qApp->processEvents();
}

void Partitioner::setHelp(QString const &h)
{
	if(help)
		help->setText(h);
	else
		help=new QLabel(h, this);
	progress->setValue(0);
	help->show();
}

void Partitioner::abort()
{
	qApp->exit(1);
}
