#include "pkginstaller.moc"
#include "packages.h"
#include "Files.h"
#include <QFile>
#include "dist.h"
#include "mkdir.h"
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <qmessagebox.h>
#include <sys/utsname.h>
#include <sys/swap.h>
#include <iostream>
#include <errno.h>
#define max(x,y) x > y ? x : y
using namespace std;

PkgInstaller::PkgInstaller(QWidget *parent):QFrame(parent)
{
	setAutoFillBackground(true);
	setFrameShape((QFrame::Shape)(QFrame::Box|QFrame::Sunken));
	setLineWidth(3);
	setMidLineWidth(0);
	setFocusPolicy(Qt::StrongFocus);
	info=new QLabel(tr("Applications are being copied to your hard disk. Since this will take several minutes,\npossibly hours on a slow machine, you can use this time to play Tetrix below."), this);
	info->setTextFormat(Qt::PlainText);
	info->setScaledContents(true);
	currentPkg=new QLabel(tr("Determining installation order:"), this);
	progress=new QProgressBar(this);
	progress->setRange(0, 100);
	total=new QLabel(tr("Total progress:"), this);
	totalprogress=new QProgressBar(this);
	totalprogress->setRange(0, 100);
	_handler=new Packages(this);

	// Add a little game...
	game=new TetrixBoard(this);
	newGame=new QPushButton(tr("&New Game"), this);
	connect(newGame, SIGNAL(clicked()), game, SLOT(start()));
	pauseGame=new QPushButton(tr("&Pause Game"), this);
	connect(pauseGame, SIGNAL(clicked()), game, SLOT(pause()));
	scoreLbl=new QLabel(tr("Score"), this);
	score=new QLCDNumber(5, this);
	connect(game, SIGNAL(updateScoreSignal(int)), score, SLOT(display(int)));
	levelLbl=new QLabel(tr("Level"), this);
	level=new QLCDNumber(5, this);
	connect(game, SIGNAL(updateLevelSignal(int)), level, SLOT(display(int)));
	linesLbl=new QLabel(tr("Lines"), this);
	lines=new QLCDNumber(5, this);
	connect(game, SIGNAL(updateRemovedSignal(int)), lines, SLOT(display(int)));
	tetrixinfo=new QLabel(tr("Moving the falling<br>blocks:<br><font color=\"#0000ff\">sideways:</font> left/right arrow<br><font color=\"#0000ff\">turn:</font> up/down arrow<br><font color=\"#0000ff\">drop:</font>space<br><font color=\"#0000ff\">faster/slower:</font>0-9<br>Completed rows disappear.<br>The game ends when the\nupper border is reached."), this);
	resizeEvent(0);
}

void PkgInstaller::resizeEvent(QResizeEvent *e)
{
	if(e)
		QFrame::resizeEvent(e);
	info->setGeometry(10, 10, width()-20, info->sizeHint().height());
	int y=info->y()+info->height()+10;
	int w=max(currentPkg->sizeHint().width(), total->sizeHint().width());
	int pb_height=max(currentPkg->sizeHint().height(), progress->sizeHint().height());
	currentPkg->setGeometry(10, y, w, pb_height);
	progress->setGeometry(20+w, y, width()-30-w, pb_height); y+=pb_height+10;
	total->setGeometry(10, y, w, pb_height);
	totalprogress->setGeometry(20+w, y, width()-30-w, pb_height); y+=pb_height+10;
	game->setGeometry(160, y, width()-190, height()-y-20);
#define AddControl(c) c->setGeometry(10, y, 140, c->sizeHint().height()); y+=c->height()+5
	AddControl(newGame);
	AddControl(pauseGame);
	AddControl(scoreLbl);
	AddControl(score);
	AddControl(levelLbl);
	AddControl(level);
	AddControl(linesLbl);
	AddControl(lines);
	AddControl(tetrixinfo);
}

void PkgInstaller::packagesDone(langtype _language, kbdtype _keyboard, QString _timezone)
{
	info->setText(tr("Configuring your system...\nThe progress indicator may hang for a while."));
	progress->hide();
	currentPkg->resize(width()-20, currentPkg->height());
	configure=new PostInstall(_partitions->rootPartition(), _partitions->bootPartition(), _partitions->grubLocation(), _language, _keyboard, _timezone, this);
	connect(configure, SIGNAL(progress(QString, int)), this, SLOT(updateProgressConfig(QString, int)));
	connect(configure, SIGNAL(cfgDone()), this, SLOT(ready()));
	configure->start();
}

void PkgInstaller::updateProgress(Packages::Progress *p)
{
	currentPkg->setText(p->currentPkg);
	progress->setValue(p->progress);
	totalprogress->setValue(p->totalprogress);
}

void PkgInstaller::updateProgressConfig(QString task, int percentage)
{
	currentPkg->setText(task);
	totalprogress->setValue(percentage);
}

void PkgInstaller::ready()
{
	umount("/mnt/dest/sys");
	umount("/mnt/dest/proc");
	umount("/mnt/dest/dev/pts");
	umount("/mnt/dest/dev/shm");
	umount("/mnt/dest/tmp");  /* just in case */
	umount("/mnt/dest/usr");  /* just in case */
	umount("/mnt/dest/var");  /* just in case */
	umount("/mnt/dest/boot"); /* just in case */
	umount("/mnt/dest/home"); /* just in case */
	umount("/mnt/dest");
	umount("/mnt/source");
	info->setText(tr("DONE! %1 %2 is installed.\nFeel free to turn off the computer.").arg(DIST).arg(VERSION));
	currentPkg->hide();
	progress->setValue(100);
	emit postDone();
}

void PkgInstaller::keyPressEvent(QKeyEvent *e)
{
	if(e) {
		game->keyPressEvent(e);
		QFrame::keyPressEvent(e);
	}
}

void PkgInstaller::start(QStringList packages, Partitioner *partitions)
{
	setFocus();
	_partitions=partitions;
#if NONDESTRUCTIVE
	mkdir_p("/mnt/dest");
#else
	// First of all, mount partitions...
	PartitionMap __partitions=_partitions->partitions();
	bool needToRemount;
	QStringList mounted;
	QString fstab;

	// Get the mount order right -- e.g. / must be mounted before /home
	// /home must be mounted before /home/bero, ...
	umask(0);
	QString installsource(getenv("INSTALLSOURCE"));
	do {
		needToRemount=false;
		for(QMap<QString,FileSystem>::ConstIterator it=__partitions.begin(); it!=__partitions.end(); it++) {
			if(it.value().mountpoint.isEmpty() || it.value().mountpoint == "swap") // Partition isn't supposed to be mounted
				continue;
			bool ok(true);
			QString mp=it.value().mountpoint;
			QStringList dirs(mp.split("/"));

			dirs.prepend("");
			QString dir;
			for(QStringList::ConstIterator dit=dirs.begin(); dit!=dirs.end(); ++dit) {
				dir += "/" + *dit;
				if(dir.startsWith("//"))
					dir=dir.mid(1);
				if(__partitions.hasMountpoint(dir) && !mounted.contains(dir) && mp!=dir) {
					needToRemount=true;
					ok=false;
				}
			}
			if(!ok || mounted.contains(mp))
				continue;

			mkdir_p("/mnt/dest" + mp);
			if(mount(QFile::encodeName(it.key()), QFile::encodeName("/mnt/dest" + mp), QFile::encodeName(it.value().filesystem), 0, NULL))
				QMessageBox::information(0, "Warning", "Could not mount " + it.key() + " as " + mp + ":" + strerror(errno));
			QString finalDev(it.key());
			if(installsource.startsWith("/dev/sd") && finalDev.startsWith("/dev/sd")) {
				// We're installing from a memory stick, so all
				// /dev/sd* devices after that will have a
				// different name when booting from anything
				// else. Let's try to compensate...
				QChar srcId=installsource.at(7);
				QChar devId=finalDev.at(7);
				if(devId>srcId)
					finalDev.replace(7, QChar::fromAscii(devId.toAscii()-1));
			}
			fstab += finalDev + "\t" + mp + "\t" + it.value().filesystem + "\tnoatime\t1\t1\n";
			mounted.append(mp);
		}
	} while(needToRemount);
#endif

	// Prepare for rpm (won't work without /var/lib/rpm, /var/lock/rpm and /var/tmp...)
	mkdir("/mnt/dest/var", 0755);
	mkdir("/mnt/dest/var/tmp", 1777);
	mkdir("/mnt/dest/var/lib", 0755);
	mkdir("/mnt/dest/var/lib/rpm", 0755);
	mkdir("/mnt/dest/var/lib/rpm/tmp", 0755);
	mkdir("/mnt/dest/var/lib/rpm/log", 0755);
	mkdir("/mnt/dest/var/lock", 0755);
	mkdir("/mnt/dest/var/lock/rpm", 0755);

	// /var/lib/rpm must be writable in the root FS, for whatever reason
	mount("none", "/var/lib/rpm", "tmpfs", 0, NULL);
	mkdir("/var/lib/rpm/log", 0755);
	mkdir("/var/lib/rpm/tmp", 0755);

	// Get base filesystems up...
	mkdir("/mnt/dest/sys", 0555);
	mount("none", "/mnt/dest/sys", "sysfs", 0, NULL);
	mkdir("/mnt/dest/proc", 0555);
	mount("none", "/mnt/dest/proc", "proc", 0, NULL);
	mkdir("/mnt/dest/dev", 0755);
	mkdir("/mnt/dest/dev/pts", 0755);
	mount("none", "/mnt/dest/dev/pts", "devpts", 0, NULL);
	symlink("/proc/mounts", "/mnt/dest/etc/mtab");

#ifndef NONDESTRUCTIVE
	// Generate fstab
	mkdir("/mnt/dest/etc", 0755);
	FILE *f=fopen("/mnt/dest/etc/fstab", "w");

	fprintf(f, "%s", qPrintable(fstab));
	fprintf(f, "none	/dev/pts	devpts	gid=5,mode=620	0	0\n");
	fprintf(f, "none	/proc	proc	defaults	0	0\n");
	fprintf(f, "none	/dev/shm	tmpfs	defaults	0	0\n");
	// Scan for swap partition(s) -- there might be 0, 1 or several...
	ped_device_probe_all();
	PedDevice *dev=NULL;
	while((dev=ped_device_get_next(dev))) {
		QString p(dev->path);
		if(!p.startsWith("/dev/sd") && (p.contains("/dev/scd") || p.contains("/dev/sr") || !QFile::exists("/proc/ide/" + p.section('/', 2, 2) + "/cache")))
			continue;
		PedDisk *disk=ped_disk_new(dev);
		if(!disk) // Unpartitioned disk
			continue;
		PedPartition *part=NULL;
		while((part=ped_disk_next_partition(disk, part))) {
			if(part->fs_type && part->fs_type->name && QString(part->fs_type->name).startsWith("linux-swap")) {
				if(!swapon(dev->path, 0))
					QMessageBox::information(0, "swap error", "Couldn't activate swap on " + QString(dev->path) + ".\nContinuing without swap.");
				fprintf(f, "%s%u	swap	swap	defaults	0	0\n", dev->path, part->num);
			}
		}
	}
	fclose(f);

	// Initial rpm config
	f=fopen("/mnt/dest/var/lib/rpm/DB_CONFIG", "w");
	if(f) {
		fputs("set_data_dir .\n"
		      "set_create_dir .\n"
		      "set_lg_dir ./log\n"
		      "set_tmp_dir ./tmp\n\n"
		      "set_thread_count 64\n\n"
		      "set_mp_mmapsize 268435456\n\n"
		      "set_flags DB_LOG_AUTOREMOVE\n\n"
		      "set_lk_max_locks 16384\n"
		      "set_lk_max_lockers 16384\n"
		      "set_lk_max_objects 16384\n"
		      "mutex_set_max 163840\n", f);
		fclose(f);
	}

	f=fopen("/var/lib/rpm/DB_CONFIG", "w");
	if(f) {
		fputs("set_data_dir .\n"
		      "set_create_dir .\n"
		      "set_lg_dir ./log\n"
		      "set_tmp_dir ./tmp\n\n"
		      "set_thread_count 64\n\n"
		      "set_mp_mmapsize 268435456\n\n"
		      "set_flags DB_LOG_AUTOREMOVE\n\n"
		      "set_lk_max_locks 16384\n"
		      "set_lk_max_lockers 16384\n"
		      "set_lk_max_objects 16384\n"
		      "mutex_set_max 163840\n", f);
		fclose(f);
	}
#endif
	
	// Next, install packages...
	if(packages.count() <= 1) {
		// No package list provided -> this is an everything install
		QStringList pkgs=Files::glob("/RPMS/*.rpm", (Files::Types)(Files::File | Files::Link));
		// Add arch specific stuff...
		struct utsname s;
		uname(&s);
		QString arch=s.machine;
#if defined(__i386__) && !defined(__x86_64__)
		bool isAMD=false;
		bool isP3=false;
		bool isP4=false;
		bool isBrokenVIACentaur=false;
#endif

		QFile cpuinfo("/proc/cpuinfo");
		QString line;
		int CPUs=0;
		if(cpuinfo.open(QFile::ReadOnly)) {
			line=cpuinfo.readLine();
			while(!line.isNull()) {
				if(line.left(9) == "processor") {
					CPUs++;
#if defined(__i386__) && !defined(__x86_64__)
				} else if(line.left(9)=="vendor_id" && line.contains("AMD")) {
					isAMD=true;
				} else if(line.left(9)=="vendor_id" && line.contains("CentaurHauls")) {	
					isBrokenVIACentaur=true;
				} else if (line.left(5)=="flags") {
					if(line.contains("sse"))
						isP3=true;
					if(line.contains("sse2"))
						isP4=true;
					if(line.contains("cmov")) // If it's a VIA, it's a fixed one
						isBrokenVIACentaur=false;
#endif
				}
				line=cpuinfo.readLine();
			}
			cpuinfo.close();
		}
#if defined(__i386__) && !defined(__x86_64__)
//		cout << "arch is " << arch.local8Bit() << endl;
		if(arch == "i386" || arch == "i486") // Ouch! Let's hope the best...
			arch="i586";
		if(arch == "i786" || arch == "i886" || arch == "i986") // Ok, wild but educated guess
			arch = "i686";
		if(arch != "i586" && arch != "i686" && arch != "athlon") // Huh? Better safe than sorry...
			arch = "i586";
		if(arch == "i686" && isAMD && isP3) {
			// Unfortunately, uname() doesn't know the difference between an i686
			// and an athlon...
			// Let's assume that anything from AMD that is i686 or higher and has SSE is athlon compatible.
			arch = "athlon";
		}
		if(arch == "i686" && isBrokenVIACentaur) {
			arch = "i586";
		}
		if (arch == "i686" && isP3 && !isAMD) {
			if (isP4) {
				arch = "pentium4";
			} else {
				arch = "pentium3";
			}
		}
#endif
		// Allow override on the command line to ease installing on
		// one box to put the harddisk in another (e.g. boxes
		// without CD drives)
		QString cmdline;
		FILE *f=fopen("/proc/cmdline", "r");
		if(f) {
			char buf[1024];
			fgets(buf, 1024, f);
			fclose(f);
			cmdline=buf;
		}
		QStringList cmds=cmdline.simplified().split(' ');
		for(QStringList::ConstIterator it=cmds.begin(); it!=cmds.end(); ++it) {
			if((*it).startsWith("cpu=")) {
				QString forcedArch=(*it).section('=', 1, 1).trimmed();
#if defined(__i386__) && !defined(__x86_64__)
				if(forcedArch=="i586" || forcedArch=="i686" || forcedArch=="pentium3" || forcedArch=="pentium4" || forcedArch=="athlon")
					arch=forcedArch;
				else
					QMessageBox::information(0, "Warning", "Invalid cpu= option given (" + forcedArch + "), assuming " + arch);
#endif
			} else if((*it) == "smp") {
				if(CPUs<2)
					CPUs=2;
			} else if((*it) == "nosmp") {
				if(CPUs>1)
					CPUs=1;
			}
		}
		
		pkgs += Files::glob("/RPMS/" + arch + "/*.rpm", (Files::Types)(Files::File | Files::Link));

		// We need an SMP _or_ UP kernel, not both
		bool haveKernel=false;
		for(QStringList::ConstIterator it=pkgs.begin(); it!=pkgs.end(); it++) {
			if((*it).contains("/kernel-2")) {
				if(CPUs<=1) {
					packages.append(*it);
					haveKernel=true;
				}
			} else if((*it).contains("/kernel-smp")) {
				if(CPUs>1) {
					packages.append(*it);
					haveKernel=true;
				}
			} else {
				packages.append(*it);
			}
		}
		if(!haveKernel) {
			// No matching kernel package... Fall back to a
			// non-SMP kernel on SMP systems and vice versa
			for(QStringList::ConstIterator it=pkgs.begin(); it!=pkgs.end(); it++) {
				if((*it).contains("/kernel-2") || (*it).contains("/kernel-smp")) {
					packages.append(*it);
					haveKernel=true;
					break;
				}
			}
		}
#if defined(__i386__) && !defined(__x86_64__)
		if(!haveKernel) {
			// Lastly, fall back to i586
			pkgs += Files::glob("/RPMS/i586/*.rpm", (Files::Types)(Files::File | Files::Link));
			for(QStringList::ConstIterator it=pkgs.begin(); it!=pkgs.end(); it++) {
				if((*it).contains("/kernel-2") || (*it).contains("/kernel-smp")) {
					packages.append(*it);
					haveKernel=true;
					break;
				}
			}
		}
#endif
	}
	if(QFile::exists("/tmp/raidtab"))
		system("/bin/cp /tmp/raidtab /mnt/dest/etc");
	connect(_handler, SIGNAL(progress(Packages::Progress *)), this, SLOT(updateProgress(Packages::Progress *)));
	connect(_handler, SIGNAL(pkgDone(bool)), this, SIGNAL(finished(bool)));
	_handler->install(packages, "/mnt/dest");
}
