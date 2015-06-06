#define NO_SHELLS 1

#include <QApplication>
#include <QStyleFactory>
#include <QFile>
#include <iostream>
#include <sys/klog.h>
#include <sys/stat.h>
#include "installer.h"
#include "modules.h"

// DSDS includes for bash support
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/vt.h>
#include <pty.h>
#include <termios.h>
#include <string.h>

// PCI scanning/module loading support
extern "C" {
#include <pci/pci.h>
#include <sys/wait.h>
#include <stdarg.h>
}

using namespace std;

static char const * const env[] = {"PATH=/bin:/sbin:","HOME=/","TERM=linux","USER=root",NULL};
static QString cmdline = QString::null;

#define OUT(fd, t) write(fd, t, strlen(t))

QString unquote(QString const &s);
void createtty(int vt, int clear, int flags=0);

// Remove quotation from a string
QString unquote(QString const &s) {
	QString result=s.trimmed();
	if(result.startsWith("\""))
		result=result.mid(1);
	if(result.endsWith("\""))
		result=result.left(result.length()-1);
	return result;
}

// Check PCI bus and insmod everything for hardware we have
int load_modules() {
	struct pci_access *pci;
	struct pci_filter filter;
	struct pci_dev *p;
	
	pci=pci_alloc();
	pci_filter_init(pci, &filter);
	pci->numeric_ids=1;
	pci_init(pci);
	pci_scan_bus(pci);

	byte config[128];
	unsigned short pci_vendor[1024], pci_dev[1024], pci_class[1024]; // I refuse to believe anyone has more than 1024 PCI devices.
	int numpci=0;
	
	for(p=pci->devices; p && numpci < 1024; p=p->next) {
		int size=64;
		if(!pci_read_block(p, 0, config, size)) {
			cerr << "Unable to read config data from PCI" << endl;
			continue;
		}
		if(size < 128 && ((config[PCI_HEADER_TYPE] & 0x7f) == PCI_HEADER_TYPE_CARDBUS)) {
			// Cardbus bridges have a 128 byte info block
			pci_read_block(p, size, config+size, 128-size);
			size=128;
		}
		pci_setup_cache(p, config, size);
		pci_fill_info(p, PCI_FILL_IDENT | PCI_FILL_IRQ | PCI_FILL_BASES | PCI_FILL_ROM_BASE | PCI_FILL_SIZES);
		pci_vendor[numpci]=p->vendor_id;
		pci_dev[numpci]=p->device_id;
		pci_class[numpci++]=config[PCI_CLASS_DEVICE] | (config[PCI_CLASS_DEVICE+1] << 8);
	}

	QFile pcitable("/usr/share/hwdata/pcitable");
	if(!pcitable.open(QFile::ReadOnly)) {
		perror("open pcitable");
		return 1;
	}
	QTextStream pt(&pcitable);
	while(!pt.atEnd()) {
		int vendor, device;
		QString module;
		QString line=pt.readLine();
		if(line.isEmpty() || line.startsWith("#"))
			continue;
		QStringList current=line.simplified().split(' ');
		vendor=current[0].mid(2).toInt(0, 16);
		device=current[1].mid(2).toInt(0, 16);
		if(current[2].startsWith("\""))
			module=unquote(current[2]);
		else {
			// Skip subvendor/subdevice for now
			module=unquote(current[4]);
		}
		
		if(module.isEmpty()) continue;

		// Check if we have the device...
		int pdev;
		Modules *modLoader=Modules::instance();
		for(pdev=0; pdev<numpci; pdev++) {
			if(pci_vendor[pdev]==vendor && pci_dev[pdev]==device) {
#ifdef DEBUG
				printf("PCI device %04x:%04x: %s\n", vendor, device, module);
#endif
				// We have the device -- try to load the module.
				if(module.startsWith("Card:", Qt::CaseInsensitive)) // No need to care about X11 yet.
					continue;
				else if(!module.compare("unknown", Qt::CaseInsensitive) || !module.compare("ignore", Qt::CaseInsensitive)) // Can't load that...
					continue;
				if(pci_class[pdev]==PCI_CLASS_STORAGE_SCSI || pci_class[pdev]==PCI_CLASS_STORAGE_IDE) { // Make sure base SCSI support is loaded -- relevant for IDE devices as well, because libata/SATA disks use the IDE class and the SCSI layer
					modLoader->loadWithDeps("scsi_mod");
					modLoader->loadWithDeps("cdrom");
					modLoader->loadWithDeps("sr_mod");
					modLoader->loadWithDeps("sd_mod");
					modLoader->loadWithDeps("mptbase");
					modLoader->loadWithDeps("ide-gd_mod");
				}
				modLoader->loadWithDeps(module);
			}
		}
	}
	pcitable.close();
	return 0;
}

// Check if the framebuffer driver is there
int check_fb()
{
	int fd=open("/dev/fb0", 0);
	if(fd<0) {
		return 0;
	}
	close(fd);
	return 1;
}

#ifndef MINIMAL
// Load extra framebuffer modules in case vesafb doesn't work...
// Unfortunately some crappy chipsets (i810 etc.) don't support
// standards.
bool load_fb()
{
	if(check_fb())
		return true;

	Modules *modLoader=Modules::instance();

	// Framebuffer drivers need to know the resolution...
	bool vga16=false;
	bool useVesa=true;
	bool forceVesa=false;
	int fbx=1024, fby=768;
	QString fb="1024x768-16@60";
	QString vesa="0x117";

	if(cmdline.contains("lowres")) {
		fbx=800;
		fby=600;
		fb="800x600-16@60";
		vesa="0x114";
	} else if(cmdline.contains("vga16"))
		vga16=true;
	else if(cmdline.contains("novesa"))
		useVesa=false;
	else if(cmdline.contains("vesa"))
		forceVesa=true;
	if(cmdline.contains("mode="))
		fb=cmdline.mid(cmdline.indexOf("mode=")+5).section(' ', 0, 0).trimmed();

	if(cmdline.contains("nofb"))
		return false;

	if(!vga16) {
		if(!forceVesa) {
			if(!cmdline.contains("noi915")) {
				puts("loading i915 framebuffer module");
				modLoader->loadWithDeps("i915", "modeset=1");
				if(check_fb())
					return true;
			}

			if(!cmdline.contains("noradeon ") && !cmdline.endsWith("noradeon")) { // Shouldn't be confused with noradeonfb
				puts("loading radeon framebuffer module");
				modLoader->loadWithDeps("radeon", "modeset=1");
				if(check_fb())
					return true;
			}

			if(!cmdline.contains("nonouveau")) {
				puts("loading nouveau framebuffer module");
				modLoader->loadWithDeps("nouveau", "modeset=1 ctxfw=0");
				if(check_fb())
					return true;
			}

			// Try SiSFB before VESA. SiS implements VESA, but apparently gets the
			// timings wrong --- causing "out of range" errors on many LCD
			// displays.
			if(!cmdline.contains("nosisfb")) {
				puts("loading sisfb framebuffer module");
				modLoader->loadWithDeps("sisfb", "vesa=" + vesa);
				if(check_fb())
					return true;
			}

			// VIA implements VESA, but as of 2005/12/07, vesafb-tng causes
			// the machine to hang on some machines with CLE266 chips.
			// FIXME: Check if this is still the case with uvesafb
			if(!cmdline.contains("noviafb")) {
				puts("loading viafb framebuffer module");
				modLoader->loadWithDeps("viafb", "mode=" + fb);
				if(check_fb())
					return true;
			}

			// ATI implements VESA, but as of 2006/08/06, it apparently
			// produces a wrong refresh rate on Compaq evo n400c graphics
			// chipsets.
			if(!cmdline.contains("noatyfb")) {
				puts("loading atyfb framebuffer module");
				modLoader->loadWithDeps("atyfb", "mode=" + fb);
				if(check_fb())
					return true;
			}

			// Radeons usually just work, but radeonfb seems to work well too
			if(!cmdline.contains("noradeonfb")) {
				puts("loading radeonfb framebuffer module");
				modLoader->loadWithDeps("radeonfb", "mode_option=" + fb);
				if(check_fb())
					return true;
			}
		}

		if(useVesa) {
			// Try uVesa next...
			puts("loading vesa VGA module");
			// We're assuming that crtc will break some notebooks
			// because it used to be that way with vesafb-tng
			modLoader->loadWithDeps("uvesafb", "nocrtc=1 mode_option=" + fb + " mtrr=3 pmipal=1");
			if(check_fb())
				return true;

			// on older kernels, we have vesafb-tng instead of uvesafb...
			puts("loading vesa VGA module");
			modLoader->loadWithDeps("vesafb-thread");
			// crtc breaks Acer Aspire 1360 notebooks (K8N800)
			modLoader->loadWithDeps("vesafb-tng", "nocrtc=1 mode=" + fb);
			if(check_fb())
				return true;
		}

		// Some other drivers (e.g. i810) require AGP and vgastate
		modLoader->loadWithDeps("agpgart");
		modLoader->loadWithDeps("vgastate");

		// intelfb needs i2c
		modLoader->loadWithDeps("i2c-core");
		modLoader->loadWithDeps("i2c-algo-bit");

		// Since (AFAWK) only i810fb requires a working AGP driver, and
		// it's an intel_agp onboard chipset, it's safe to assume this is
		// the right AGP module
		modLoader->loadWithDeps("intel-agp");

		// We know i810 is broken...
		puts("loading intelfb framebuffer module");
		modLoader->loadWithDeps("intelfb", QString("mode=") + fb + " accel=1 hwcursor=1");
		if(check_fb())
			return true;

		// While intelfb covers some chipsets i810fb covers as well, intelfb doesn't
		// support i810 and i815 chipsets
		puts("loading i810fb framebuffer module");
		modLoader->loadWithDeps("i810fb", "mode_option=" + fb + " hsync1=40 hsync2=60 vsync1=50 vsync2=70 accel=1 mtrr=1");
		if(check_fb())
			return true;

		// atyfb and radeon are omitted intentionally.
		// We've verified all major ATI cards (as of 2005) to be VESA compliant.
		// Try the rest...
		puts("loading Trident Cyberblade framebuffer modules");
		modLoader->loadWithDeps("cyblafb", "mode=" + fb);
		
		puts("loading matrox framebuffer modules");
		modLoader->loadWithDeps("matroxfb_misc");
		modLoader->loadWithDeps("matroxfb_accel");
		modLoader->loadWithDeps("g450_pll");
		modLoader->loadWithDeps("matroxfb_g450");
		modLoader->loadWithDeps("matroxfb_DAC1064");
		modLoader->loadWithDeps("matroxfb_Ti3026");
		modLoader->loadWithDeps("matroxfb_base", "vesa=" + vesa);
		modLoader->loadWithDeps("matroxfb_proc");
		if(check_fb())
			return true;
		/* AFAIK riva cards do VESA - this may not be necessary */
		puts("loading rivafb framebuffer module");
		modLoader->loadWithDeps("rivafb");
		if(check_fb())
			return true;
		puts("loading cyber2000fb framebuffer module");
		modLoader->loadWithDeps("cyber2000fb");
		if(check_fb())
			return true;
		puts("loading neofb framebuffer module");
		modLoader->loadWithDeps("neofb", "mode=" + fb + " internal=1 external=1");
		if(check_fb())
			return true;
		puts("loading pm2fb framebuffer module");
		modLoader->loadWithDeps("pm2fb", "mode=" + fb);
		if(check_fb())
			return true;
		puts("loading sstfb framebuffer module");
		modLoader->loadWithDeps("sstfb");
		if(check_fb())
			return true;
		puts("loading tdfxfb framebuffer module");
		modLoader->loadWithDeps("tdfxfb");
		if(check_fb())
			return true;
		puts("loading tridentfb framebuffer module");
		modLoader->loadWithDeps("tridentfb", "mode=" + QString::number(fbx) + "x" + QString::number(fby) + " bpp=16 noaccel=1");
		if(check_fb())
			return true;

		if(useVesa) {
			// Last resort: Try uvesafb without mode options...
			puts("loading vesa VGA module");
			// We're assuming that crtc will break some notebooks
			// because it used to be that way with vesafb-tng
			modLoader->loadWithDeps("uvesafb");
			if(check_fb())
				return true;
		}
	}

	// Yuck... very very last resort.
	puts("loading vga16fb framebuffer module");
	modLoader->loadWithDeps("vgastate");
	modLoader->loadWithDeps("vga16fb");

	if(check_fb())
		return true;
	return false;
}
#else
int load_fb()
{
	return check_fb();
}
#endif

void initterm(int fd, int clear)
{
	if(clear) {
// DSDS		struct winsize winsize;
// DSDS		ioctl(fd, TIOCGWINSZ, &winsize);
// DSDS		winsize.ws_row=24;
// DSDS		winsize.ws_col=80;
// DSDS		ioctl(fd, TIOCSWINSZ, &winsize);
		OUT(fd, "\033[?25l\033[?1c\033[1;24r\033[?25h\033[?8c\033[?25h"
		        "\033[?0c\033[H\033[J\033[24;1H\033[?25h\033[?0c");
	} else
		OUT(fd, "\n");

	if(clear != 2) {
		OUT(fd, "\033[1;31mArk\033[0;39m Linux Installer - Shell Access\n");
		OUT(fd, "\n");
		OUT(fd, "To return to the installer screen, press <Ctrl><Alt><F1>.\n");
		OUT(fd, "\n");
	}
}

void createtty(int vt, int clear, int flags)
{
	int fd;
	char v[20];
	sprintf(v, "/dev/tty%u", vt);
	close(0);
	close(1);
	close(2);
	fd=open(v, O_RDWR|flags);
	if(fd < 0)
		perror(v);
	dup(fd);
	dup(fd);
	ioctl(fd, VT_ACTIVATE, vt);
	ioctl(fd, VT_WAITACTIVE, vt);
	initterm(fd, clear);
}

int spawnbash(int vt, int init)
{
	pid_t bash;
	createtty(vt,init);
	setsid();
	if (!(bash=fork()))
	{
		execle("/bin/sh", "sh", "--login", NULL, env);
	} else {
		createtty(1,2);
		return bash;
	}
	return -1;
}

int main(int argc, char **argv)
{
	setenv("QWS_DISPLAY", "linuxfb", 1);

	int fd=open("/proc/cmdline", O_RDONLY|O_NOCTTY);
	if(fd>=0) {
		char buf[8192];
		memset(buf, 0, 8192);
		read(fd, buf, 8192);
		cmdline=buf;
		close(fd);
	}

#ifndef Q_WS_X11
	fd=open("/proc/sys/kernel/hotplug", O_RDWR|O_NOCTTY);
	if(fd>=0) {
		write(fd, "/sbin/hotplug", 13);
		close(fd);
	}

	if(!cmdline.contains("debugshell") && !load_fb()) {
		puts("\nSorry, your graphics card does not support framebuffer mode.\n"
		     "Please get a VESA compliant graphics card.\n"
		     "If you think you're getting this message in error, please\n"
		     "report it to development@arklinux.org.\n"
		     "\n"
		     "Will try to continue installing anyway, but this is likely to fail.\n");
		sleep(10);
	}

	int tty1, tty4;
	spawnbash(2,1);
	spawnbash(3,1);
	createtty(4,1);
	if ((tty4 = open("/dev/tty4", O_RDWR)) < 0)
		perror("open tty4");
	else {
		close(STDOUT_FILENO);
		close(STDERR_FILENO);
		if (dup2 (tty4, STDOUT_FILENO) < 0)
			perror("stdout->tty4");
		if (dup2 (tty4, STDERR_FILENO) < 0)
			perror("stderr->tty4");
	}

	// Load mouse and keyboard drivers -- not much we can do without them
	Modules *loader=Modules::instance();
	loader->loadWithDeps("usbhid");
	loader->loadWithDeps("usbkbd");
	loader->loadWithDeps("usbmouse");
	loader->loadWithDeps("psmouse");
	loader->loadWithDeps("serial_core");
	loader->loadWithDeps("8250");
	loader->loadWithDeps("sermouse");
	// There seems to be a problem with autoloading ext4 at the moment
	// (2.6.29-rc2) -- so we load it manually just in case
	loader->loadWithDeps("ext4");
	
	// Load modules for all present PCI cards. This is already done
	// in the initrd, but it may not have all needed modules (SATA drivers
	// etc). Loading a module twice doesn't kill.
	//
	load_modules();

	// Return to TTY 1...
	if ((tty1 = open("/dev/tty0", O_RDWR)) < 0) {
		perror("open tty0");
	} else {
		ioctl(tty1, VT_ACTIVATE, 1);
		ioctl(tty1, VT_WAITACTIVE, 1);
		close(tty1);
	}

	if(cmdline.contains("debugshell")) {
		cout << "Enjoy your debug shell..." << endl;
		spawnbash(1,1);
		while(true)
			sleep(-1);
	}

	// Maybe the shells need some time?!
	sleep(1);
	
	cerr << "Entering graphics mode -- if your system hangs here, this is probably" << endl
	     << "a problem with the graphics driver. If this happens, please notify" << endl
	     << "development@arklinux.org and let us know which graphics chipset" << endl
	     << "you're using." << endl;
#endif

#ifdef Q_WS_QWS
	// Qt/Embedded doesn't like printks to its console too much...
	klogctl(6, NULL, 0);
	QApplication app(argc, argv, QApplication::GuiServer);
#else
	QApplication app(argc, argv);
#endif
	// The default font doesn't handle ISO-8859-2 characters
	// well (e.g. Polish)
	// so we switch to a font that does
#ifdef Q_WS_QWS
	app.setFont(QFont("lucidux_sans", 10));
#else
	app.setFont(QFont("Liberation Sans", 9));
#endif
	app.setStyle(QStyleFactory::create("Plastique"));
	umask(0);
	Installer i;
	int ret=app.exec();
#ifndef NO_SHELLS
	if (close (tty4) < 0)
		perror("close tty4");
#endif
	return ret;
}
