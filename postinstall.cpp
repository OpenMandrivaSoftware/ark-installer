#include "postinstall.moc"
#include "qextconfigfile.h"
#include "Files.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <qfile.h>
#include <libgen.h>
#define PROGRESS int(float(100)*(currentTask++/(numTasks-1)))
extern "C" {
#include <pci/pci.h>
#include <parted/parted.h>
#include <unistd.h>
}
#include <fstream>
#include <iostream>
#ifndef VER
#define VER "2007.1"
#endif
using namespace std;

void pam_grant(QString const &file, QStringList const &users, bool firstline=true, bool securetty=false);
void pam_grant(QString const &file, QStringList const &users, bool firstline, bool securetty)
{
	if(QFile::exists(file)) {
		QStringList pamConfig;
		QFile in(file);
		if (in.open(QFile::ReadOnly))
		{
			QString line;
			bool added=false;
			while(!in.atEnd()) {
				line=in.readLine().trimmed();
				if(firstline && !added && !line.isEmpty() && line.left(1)!="#") {
					added=true;
					QString stty=QString::null;
					if(securetty)
						stty=" securetty";
					pamConfig.append("auth	sufficient	pam_userlist.so users=" + users.join(",") + stty);
				}
				if(!firstline && line.contains("pam_console"))
					firstline=true;
				pamConfig.append(line);
			}
			in.close();
		}
		if (in.open(QFile::WriteOnly)) {
			in.write(pamConfig.join("\n").toUtf8());
			in.close();
		}
	} else {
		QFile f(file);
		if(f.open(QFile::WriteOnly)) {
			QTextStream ts(&f);
			ts << "#%%PAM-1.0" << endl
			   << "auth	sufficient	pam_userlist.so users=arklinux" << (securetty ? " securetty" : "") << endl
			   << "auth	sufficient	pam_rootok.so" << endl
			   << "auth	include		system_auth" << endl
			   << "session	optional	pam_xauth.so" << endl
			   << "account	required	pam_permit.so" << endl;
			f.close();
		}
	}
}

void console_grant(QStringList const &users);
void console_grant(QStringList const &users)
{
	if(QFile::exists("/mnt/dest/etc/security/console.autologin")) {
		QStringList consoleConfig;
		QFile in("/mnt/dest/etc/security/console.autologin");
		if (in.open(QFile::ReadOnly))
		{
			QString line;
			bool added=false;
			while(!in.atEnd()) {
				line=in.readLine().trimmed();
				if(!added && !line.isEmpty() && !line.startsWith("#")) {
					added=true;
					consoleConfig.append(users.join("\n"));
				}
				consoleConfig.append(line);
			}
			in.close();
		}
		FILE *out=fopen("/mnt/dest/etc/security/console.autologin", "w");
		fputs(qPrintable(consoleConfig.join("\n")), out);
		fchmod(fileno(out), 0644);
		fclose(out);
	} else {
		FILE *f=fopen("/mnt/dest/etc/security/console.autologin", "w");
		if(f) {
			fputs("arklinux\n", f);
			fchmod(fileno(f), 0644);
			fclose(f);
		}
	}
}

PostInstall::PostInstall(QString const &rootDir, QString const &bootDir, Partitioner::GrubLocation grub, langtype _language, kbdtype _keyboard, QString _timezone, QObject *parent):QObject(parent),root(rootDir),boot(bootDir),_grub(grub)
{
	_lang=_language;
	_kbd=_keyboard;
	_tz=_timezone;
	numTasks=9;
	currentTask=0;
	env.clear();
	env.insert("PATH", "/bin:/usr/bin:/sbin:/usr/sbin");
	env.insert("HOME", "/tmp");
	env.insert("USER", "root");
	env.insert("USERNAME", "root");
#if !defined(SERVER) || 1 // root pw configuration is broken ATM
	connect(this, SIGNAL(L10NDone(bool)), this, SLOT(configureLogin()));
	connect(this, SIGNAL(loginDone(bool)), this, SLOT(configureHardware()));
#else
	connect(this, SIGNAL(L10NDone(bool)), this, SLOT(configurePassword()));
	connect(this, SIGNAL(passwordDone(bool)), this, SLOT(configureHardware()));
#endif
	connect(this, SIGNAL(hardwareDone(bool)), this, SLOT(configureNet()));
	connect(this, SIGNAL(netDone(bool)), this, SLOT(configureInitrd()));
	// initrd must be done after hardware (for SCSI/SATA detection) but before grub (grub needs to know where the initrd is)
	if(grub != Partitioner::Nowhere) {
		connect(this, SIGNAL(initrdDone(bool)), this, SLOT(configureGrub()));
		connect(this, SIGNAL(grubDone(bool)), this, SLOT(configurePcmcia()));
	} else {
		connect(this, SIGNAL(initrdDone(bool)), this, SLOT(configurePcmcia()));
		numTasks--;
	}
	connect(this, SIGNAL(pcmciaDone(bool)), this, SLOT(configureFonts()));
	connect(this, SIGNAL(fontsDone(bool)), this, SLOT(prelink()));
	connect(this, SIGNAL(prelinkDone(bool)), this, SLOT(postInstall()));
	connect(this, SIGNAL(postInstallDone()), this, SIGNAL(cfgDone()));
}

void PostInstall::start() {
	// This could be in the constructor, but we want to connect
	// our signals first...
	configureL10N();
}

void PostInstall::configureL10N() {
	emit progress(tr("Configuring Locales"), PROGRESS);
	qApp->processEvents();
	// Configure localization...
	QString xkboptions=_kbd.x11options;
	if(!xkboptions.isEmpty())
		xkboptions.append(",");
	xkboptions.append("compose:paus,terminate:ctrl_alt_bksp");
	QFile xorg("/mnt/dest/etc/X11/xorg.conf.d/keyboard.conf");
	if(xorg.open(QFile::WriteOnly)) {
		QTextStream ts(&xorg);
		ts << "Section \"InputClass\"" << endl
		   << "	MatchIsKeyboard \"yes\"" << endl
		   << "	Driver \"evdev\"" << endl
		   << "	Option \"CoreKeyboard\"" << endl
		   << "	Option \"XkbLayout\" \"" << _kbd.x11layout << "\"" << endl
		   << "	Option \"XkbModel\" \"evdev\"" << endl
		   << "	Option \"XkbVariant\" \"" << _kbd.x11variant << "\"" << endl
		   << "	Option \"XkbRules\" \"xorg\"" << endl
		   << "	Option \"XkbOptions\" \"" << xkboptions << "\"" << endl
		   << "EndSection";
		xorg.close();
	}
	QFile kxkb("/mnt/dest/usr/share/config/kxkbrc");
	if(kxkb.open(QFile::WriteOnly)) {
		QTextStream ts(&kxkb);
		ts << "[Layout]" << endl
		   << "LayoutList=" << _kbd.x11layout;
		if(!QString(_kbd.x11variant).isEmpty())
			ts << "(" << _kbd.x11variant << ")";
		ts << endl
		   << "Model=evdev" << endl
		   << "Options=" << xkboptions << endl
		   << "ShowFlag=true" << endl
		   << "ShowLayoutIndicator=true" << endl
		   << "ShowSingle=false" << endl;
		kxkb.close();
	}
	QFile consoleKbd("/mnt/dest/etc/sysconfig/keyboard");
	if(consoleKbd.open(QFile::WriteOnly)) {
		QTextStream ts(&consoleKbd);
		ts << "KEYBOARDTYPE=\"pc\"" << endl
		   << "KEYTABLE=\"" << _kbd.consoletype << "\"" << endl;
		consoleKbd.close();
	}
	QFile i18n("/mnt/dest/etc/sysconfig/i18n");
	if(i18n.open(QFile::WriteOnly)) {
		QTextStream ts(&i18n);
		ts << "LANG=\"" << _lang.code << "\"" << endl
		   << "CHARSET=\"" << _lang.charset << "\"" << endl
		   << "SYSFONT=\"" << _lang.font << "\"" << endl;
		i18n.close();
	}
	QProcess::execute("/mnt/dest/usr/sbin/chroot /mnt/dest /usr/sbin/rebuild-locale-archive");
	bool const success=(symlink(QFile::encodeName("/usr/share/zoneinfo/posix/" + _tz), "/mnt/dest/etc/localtime") == 0);
	emit L10NDone(success);
}

void PostInstall::configureGrub() {
	emit progress(tr("Configuring Boot Loader"), PROGRESS);
	// Check if we need to pass anything else to the kernel
	QString extra_param;
	FILE *kcmd=fopen("/proc/cmdline", "r");
	if(kcmd) {
		while(!feof(kcmd) && !ferror(kcmd)) {
			char buf[1024];
			fgets(buf, 1024, kcmd);
			QString s=buf;
			if(s.contains("nousb"))
				extra_param += "nousb ";
			if(s.contains("nofirewire"))
				extra_param += "nofirewire ";
			if(s.contains("acpi=off"))
				extra_param += "acpi=off ";
			if(s.contains("pci=noacpi"))
				extra_param += "pci=noacpi ";
			if(s.contains(" noacpi") || s.startsWith("noacpi"))
				extra_param += "noacpi ";
			if(s.contains("noapic"))
				extra_param += "noapic ";
			if(s.contains("nolapic"))
				extra_param += "nolapic ";
		}
		fclose(kcmd);
	}
	extra_param=QString(" ") + extra_param.simplified();
#ifdef SERVER
	extra_param += " 3";
#endif

	// Generate a device map
	system("/mnt/dest/usr/sbin/chroot /mnt/dest /sbin/grub --device-map=/boot/grub/device.map --batch </dev/null");

	QString rootDisk=root.left(root.length()-1);
	int rootPart=root.right(1).toUInt()-1; // -1 because grub is 0 based, linux is 1-based

	QString bootDisk=boot.left(boot.length()-1);
	int bootPart=boot.right(1).toUInt()-1; // -1 because grub is 0 based, linux is 1-based

	// Write a grub configuration...
	char mbr[6] = { '(', 'h', 'd', '0', ')', 0 };
	QString installsource(getenv("INSTALLSOURCE"));
	if(installsource.startsWith("/dev/sd")) {
		// (hd0) will be the memory stick we're installing from.
		// Overwriting its boot sector is probably not a very
		// intelligent thing to do.
		mbr[3]++;
		// Similarily, if root is /dev/sdb*, that will be /dev/sda*
		// in a running system...
		if(root.startsWith("/dev/sd")) {
			QChar instdev=installsource.at(7);
			QChar rootdev=root.at(7);
			if(rootdev>instdev)
				root.replace(7, QChar::fromAscii(rootdev.toAscii()-1));
		}
	}

	FILE *f=fopen("/mnt/dest/tmp/install-grub.sh", "w");
	fprintf(f,
	      "#!/bin/sh\n"
	      "# Copy the GRUB loaders where they belong...\n"
	      "cp /usr/share/grub/*/* /boot/grub\n"
	      "# Find the GRUB label for our root partition...\n"
	      "KERNEL=`/bin/ls --sort=time /boot/vmlinuz-* |/usr/bin/head -n1`\n"
	      "INITRD=`/bin/ls --sort=time /boot/initrd-*.img |/usr/bin/head -n1`\n"
	      "KERNELVER=`echo ${KERNEL} |/bin/sed \"s,.*nuz-,,\"`\n");
	if(boot.isEmpty()) {
		fprintf(f,
			"BOOT=/boot\n"
			"ROOT=\"`cat /boot/grub/device.map |grep %s |sed -e 's,).*,,'`,%u)\"\n", qPrintable(rootDisk), rootPart
		);
	} else {
		fprintf(f,
			"BOOT=\n"
			"KERNEL=\"/`basename $KERNEL`\"\n"
			"INITRD=\"/`basename $INITRD`\"\n"
			"ROOT=\"`cat /boot/grub/device.map |grep %s |sed -e 's,).*,,'`,%u)\"\n", qPrintable(bootDisk), bootPart
		);
	}
	fprintf(f,
	      "echo \"root $ROOT\" >/tmp/grub-install\n"
	      "echo \"setup --force-lba %s\" >>/tmp/grub-install\n"
	      "/bin/cat >/boot/grub/grub.conf <<EOF\n"
	      "default=0\n"
	      "timeout=2\n"
	      "splashimage=${BOOT}/grub/splash.xpm.gz\n"
#ifdef XP
	      "title Ark Linux " VER " " XP "\n"
#else
	      "title Ark Linux " VER "\n"
#endif
	      "	root $ROOT\n"
	      "	kernel ${KERNEL} ro root=%s%s quiet\n"
	      "	initrd ${INITRD}\n", _grub == Partitioner::MBR ? mbr : "$ROOT", qPrintable(root), qPrintable(extra_param));
	// Check for other OSes we may want to list...
	int diskid=0;
	PedDevice *dev=NULL;
	// ped_device_probe_all();
	// ^^^^^^^^^^^^^^^^^^^^ is already done by *Partitioner, no need
	// to call it again here.
	while((dev=ped_device_get_next(dev))) {
		// FIXME workaround for parted breakage
		QString p(dev->path);
		if(p.contains("/dev/scd") || p.contains("/dev/sr") || !QFile::exists("/proc/ide/" + p.section('/', 2, 2) + "/cache"))
			continue;
		PedDisk *disk=ped_disk_new(dev);
		PedPartition *part=NULL;
		while((part=ped_disk_next_partition(disk, part))) {
			if(!part || !part->fs_type || !part->fs_type->name)
				continue;
			QString fs=QString(part->fs_type->name).toLower();
			if((part->type==0 || (part->type & PED_PARTITION_LOGICAL))) {
				// For now, we detect only Windoze installations...
				if(fs.left(3)!="fat" && fs.left(4)!="ntfs")
					continue;
				
				QString osname=QString::null;
				if(fs.left(3)=="fat") {
					// This is a Windoze partition... Let's see if it's bootable.
					struct stat s;
					mount(QFile::encodeName(dev->path + QString::number(part->num)), "/mnt/dest/mnt", "vfat", MS_RDONLY, NULL);
					if(QFile::exists("/mnt/dest/mnt/ntldr")) // Some sort of NT on FAT
						osname=tr("Windows NT/2000/XP");
					else if(!stat("/mnt/dest/mnt/msdos.sys", &s)) {
						if(s.st_size > 32767) // This seems to be a real DOS
							osname=tr("DOS");
						else { // Windoze 95/98/ME DOS
							osname=tr("Windows 95/98/ME");
						}
					} else if(QFile::exists("/mnt/dest/mnt/ibmbio.com"))
						osname=tr("DOS");
					else if(QFile::exists("/mnt/dest/mnt/os2"))
						osname=tr("OS/2");
					umount("/mnt/dest/mnt");
				} else if(fs.left(4)=="ntfs") {
					// Yuck. We have no way to detect if an NTFS partition is
					// bootable... Let's simply assume it is.
					osname=tr("Windows NT/2000/XP");
				}
				if(!osname.isEmpty()) {
					fprintf(f, "title %s\n"
					           "	rootnoverify (hd%u,%u)\n"
						   "	makeactive\n"
						   "	chainloader +1\n", qPrintable(osname), diskid, part->num-1);
				}
			}
		}
		ped_disk_destroy(disk);
		diskid++;
	}
	ped_device_free_all();
	// Install the thing.
	fputs("EOF\n"
	      "exec /sbin/grub --batch </tmp/grub-install\n", f);
	fclose(f);
	chmod("/mnt/dest/tmp/install-grub.sh", 0755);
	grub=new QProcess(this);
	grub->setProcessEnvironment(env);
	QStringList args;
	args << "/mnt/dest"
	     << "/tmp/install-grub.sh";
	connect(grub, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(grubDone_(int, QProcess::ExitStatus)));
	if(!grub->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "Boot Loader configuration failed.");
		emit grubDone(false);
	}
}

void PostInstall::configurePcmcia() {
	emit progress(tr("Configuring PCMCIA"), PROGRESS);
	probe=new QProcess(this);
	QStringList args;
	args << "/mnt/dest"
	     << "/bin/sh"
	     << "-c"
	     << "if /sbin/lspcmcia |/bin/grep -q; then /sbin/chkconfig --level 35 pcmciamod on; fi";
	probe->setProcessEnvironment(env);
	connect(probe, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(pcmciaDone_(int, QProcess::ExitStatus)));
	if(!probe->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "PCMCIA configuration failed.");
		emit pcmciaDone(false);
	}
}

void PostInstall::configureHardware() {
	emit progress(tr("Detecting Hardware"), PROGRESS);

	_coldplug=new QProcess(this);
	QStringList args;
	args << "/mnt/dest"
	     << "/bin/coldplug";
	connect(_coldplug, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(hardwareDone_(int, QProcess::ExitStatus)));
	if(!_coldplug->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "Hardware detection failed.");
		emit hardwareDone(false);
	}
}

void PostInstall::configureInitrd() {
	emit progress(tr("Configuring Initial Ramdisk"), PROGRESS);
	// Write a grub configuration and create an initramfs image.
	// Force USB support if necessary, mkinitrd can't possibly
	// autodetect this when the target filesystem isn't mounted
	// at /
	QString usbarg("");
	if(root.startsWith("/dev/sd"))
		// Fixme do this only if no scsi controller was found
		usbarg="--with-usb";

	FILE *f=fopen("/mnt/dest/tmp/install-initrd.sh", "w");
	fprintf(f,
	      "#!/bin/sh\n"
	      "# Set variables and run mkinitrd...\n"
	      "KERNEL=`/bin/ls --sort=time /boot/vmlinuz-* |/usr/bin/head -n1`\n"
	      "KERNELVER=`echo ${KERNEL} |/bin/sed \"s,.*nuz-,,\"`\n"
	      "exec /sbin/mkinitrd %s -f /boot/initrd-${KERNELVER}.img ${KERNELVER}\n", qPrintable(usbarg));
	fclose(f);
	chmod("/mnt/dest/tmp/install-initrd.sh", 0755);
	mkinitrd=new QProcess(this);
	QStringList args;
	args << "/mnt/dest"
	     << "/tmp/install-initrd.sh";
	connect(mkinitrd, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(initrdDone_(int, QProcess::ExitStatus)));
	if(!mkinitrd->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "Initial Ramdisk configuration failed.");
		emit initrdDone(false);
	}
}

void PostInstall::configureFonts() {
	emit progress(tr("Configuring Fonts"), PROGRESS);
	chkfontpath=new QProcess(this);
	chkfontpath->setProcessEnvironment(env);
	QStringList const args = QStringList()
		<< "/mnt/dest"
		<< "/tmp/chkfontpath.todo";
	connect(chkfontpath, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(fontsDone_(int, QProcess::ExitStatus)));
	if(!chkfontpath->startDetached("/mnt/dest/usr/sbin/chroot"))
		emit fontsDone(false);
}

void PostInstall::configureLogin() {
	emit progress(tr("Adding Default User"), PROGRESS);
	adduser=new QProcess(this);
	adduser->setProcessEnvironment(env);
	QStringList const args = QStringList()
		<< "/mnt/dest"
		<< "/usr/sbin/useradd"
		<< "-u500"
		<< "-Gsrc,games,audio"
		<< "arklinux";
	connect(adduser, SIGNAL(processExited(int, QProcess::ExitStatus)), this, SLOT(loginDone_(int, QProcess::ExitStatus)));
	if(!adduser->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "User addition failed.");
		loginDone_(1, QProcess::CrashExit);
	}
}

void PostInstall::configurePassword() {
	QString rootpw=QInputDialog::getText(0, tr("Root password"), tr("Please select a root password"), QLineEdit::Password, QString::null);

	// Generate random salt
	char salt[9];
	for(int i=0; i<8; i++) {
		long r=random()%52;
		if(r<26)
			salt[i]='a' + r;
		else
			salt[i]='A' + r - 26;
	}
	salt[8]=0;
	
	passwd=new QProcess(this);
	passwd->setProcessEnvironment(env);
	QStringList const args = QStringList()
		<< "/mnt/dest"
		<< "/usr/bin/perl"
		<< "-pi"
		<< "-e"
		<< "$cpw=crypt(\"" + rootpw.trimmed() + "\", \"\\$1\\$" + salt + "\"); s,^(root:)[^:]*(.*),$1$cpw$2,g;"
		<< "/etc/shadow";
	connect(passwd, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(passwordDone_(int, QProcess::ExitStatus)));
	if(!passwd->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "Root password failed.");
		passwordDone_(1, QProcess::CrashExit);
	}
}

void PostInstall::configureNet() {
	emit progress(tr("Configuring Local Networking"), PROGRESS);
	FILE *f=fopen("/mnt/dest/etc/hosts", "w");
	fputs("127.0.0.1	localhost.localdomain	localhost\n", f);
	fclose(f);
	f=fopen("/mnt/dest/etc/sysconfig/network", "w");
	fputs("NETWORKING=yes\n", f);
	fputs("HOSTNAME=localhost.localdomain\n", f);
	fclose(f);
	netDone_();
}

void PostInstall::prelink() {
	emit progress(tr("Optimizing Application Startup"), PROGRESS);
	prelinkDone_(1, QProcess::CrashExit);
	return; // Disabled for now, as it takes too long
		
	QProcess *prelink=new QProcess(this);
	QStringList args = QStringList() << "/mnt/dest" << "/usr/sbin/prelink" << "--all";
	connect(prelink, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(prelinkDone_(int, QProcess::ExitStatus)));
	if(!prelink->startDetached("/mnt/dest/usr/sbin/chroot", args)) {
		QMessageBox::information(0, "ERROR", "Application startup optimization failed.");
		prelinkDone_(1, QProcess::CrashExit);
	}
}

void PostInstall::postInstall() {
	QDir d("/postinstall");
	if(d.exists()) {
		system("cp -a /postinstall /mnt/dest");
		emit progress(tr("Running local customizations"), PROGRESS);
		qApp->processEvents();
		QStringList postScripts=d.entryList(QStringList() << "*.sh");
		for(QStringList::ConstIterator it=postScripts.begin(); it!=postScripts.end(); it++) {
			QProcess::execute(QString("/mnt/dest/usr/sbin/chroot /mnt/dest ") + d.absoluteFilePath(*it));
		}
		system("rm -rf /mnt/dest/postinstall");
	}
	emit postInstallDone();
}

void PostInstall::grubDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit grubDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::pcmciaDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit pcmciaDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::hardwareDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	// If a network card was found, make it try using DHCP by default...
	QStringList modprobeConfs = QStringList() << "/mnt/dest/etc/modprobe.conf";
	QDir d("/mnt/dest/etc/modprobe.d");
	QStringList const el=d.entryList(QDir::Files|QDir::Readable);
	for(QStringList::ConstIterator it=el.begin(); it!=el.end(); it++)
		modprobeConfs << d.absoluteFilePath(*it);
	for(QStringList::ConstIterator it=modprobeConfs.begin(); it!=modprobeConfs.end(); it++) {
		FILE *f=fopen(QFile::encodeName(*it), "r");
		if(f) {
			char buf[256];
			while(!feof(f) && !ferror(f) && fgets(buf, 256, f)) {
				QString line(buf);
				line=line.trimmed();
				if(line.startsWith("alias eth0")) {
					FILE *i=fopen("/mnt/dest/etc/sysconfig/network-scripts/ifcfg-eth0", "w");
					if(i) {
						fputs("ONBOOT=yes\n"
						      "BOOTPROTO=dhcp\n"
						      "CABLETIMEOUT=5\n"
						      "DEVICE=eth0\n"
						      "AUTOUPDATE=no\n"
						      "DEFROUTE=yes\n",i);
						fclose(i);
					}
					break;
				}
			}
			fclose(f);
		}
	}
	emit hardwareDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::initrdDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit initrdDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::fontsDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit fontsDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::loginDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	// User created... Now grant him all necessary privileges.
	QStringList consolehelper=Files::glob("/mnt/dest/etc/security/console.apps/*", (Files::Types)(Files::File | Files::Dir), false);
	for(QStringList::Iterator it=consolehelper.begin(); it!=consolehelper.end(); it++) {
		char *b=strdup(QFile::encodeName(*it));
		QString app=basename(b);
		free(b);
		if(app=="." || app==".." || app=="apt-cache" || app=="apt-cdrom" || app=="apt-config" || app=="apt-extracttemplates" || app=="apt-get" || app=="apt-sortpkgs" || app=="synaptic" || app=="ifdown" || app=="sudo" || app=="xserver")
			continue;
		pam_grant("/mnt/dest/etc/pam.d/" + app, QStringList() << "arklinux");
	}
	// Grant privileges on xserver, leaving pam_console intact
	pam_grant("/mnt/dest/etc/pam.d/xserver", QStringList() << "arklinux", false);
	// Grant privileges on su
	pam_grant("/mnt/dest/etc/pam.d/su", QStringList() << "arklinux", true, true);
	// Grant privileges on all of apt
	pam_grant("/mnt/dest/etc/pam.d/apt", QStringList() << "arklinux");
	// Grant privileges on kde
	pam_grant("/mnt/dest/etc/pam.d/kde", QStringList() << "arklinux");
	// Grant privileges on kscreensaver
	pam_grant("/mnt/dest/etc/pam.d/kscreensaver", QStringList() << "arklinux");
	// Grant privilege to log in to local text consoles
	console_grant(QStringList() << "arklinux");
	emit loginDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::netDone_()
{
	emit netDone(true);
}

void PostInstall::prelinkDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit prelinkDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}

void PostInstall::passwordDone_(int exitCode, QProcess::ExitStatus exitStatus)
{
	emit passwordDone(exitCode == 0 && exitStatus == QProcess::NormalExit);
}
