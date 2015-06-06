/* A truly minimalistic init - it'll just run rc.sysinit and *
 * spawn 7 bash instances                                    *
 * (c) 2002 Bernhard Rosenkraenzer <bero@arklinux.org>           */
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/vt.h>
#include <pty.h>
#include <termios.h>
#include <string.h>

char *env[] = {
		"PATH=/bin:/sbin:",
		"HOME=/",
		"TERM=linux",
		"USER=root",
		NULL
};

#define OUT(fd, t) write(fd, t, strlen(t))

void initterm(int fd, int clear)
{
	if(clear) {
		struct winsize winsize;
		ioctl(fd, TIOCGWINSZ, &winsize);
		winsize.ws_row=24;
		winsize.ws_col=80;
		ioctl(fd, TIOCSWINSZ, &winsize);
		OUT(fd, "\033[?25l\033[?1c\033[1;24r\033[?25h\033[?8c\033[?25h"
			"\033[?0c\033[H\033[J\033[24;1H\033[?25h\033[?0c");
	} else
		OUT(fd, "\n");
	if(clear != 2) {
		OUT(fd, "\033[1;31mArk\033[0;39mLinux installation\n");
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
	dup(fd);
	dup(fd);
	ioctl(fd, VT_ACTIVATE, vt);
	ioctl(fd, VT_WAITACTIVE, vt);
	initterm(fd, clear);
}

int spawnbash(int vt, int init)
{
	pid_t bash;
	if(!(bash=fork())) {
		createtty(vt, init, 0);
		setsid();
		execle("/bin/sh", "sh", "--login", "--rcfile",
		       "/etc/profile", NULL, env);
	} else
		return bash;
}

int main(int argc, char **argv)
{
	int vt;
	pid_t bash;
	static int bashpid[6];
	mount("none", "/proc", "proc", 0, NULL);
	mount("none", "/dev/pts", "devpts", 0, NULL);
	createtty(1, 2, O_NOCTTY);
	if(!(bash=fork()))
		execle("/bin/sh", "sh", "-c", "/etc/rc.d/rc.sysinit", NULL, env);
	else
		waitpid(bash, NULL, 0);
	close(0);
	close(1);
	close(2);
	for(vt=6; vt>=1; vt--)
		bashpid[vt]=spawnbash(vt, (vt!=1));
	while(bash=wait(NULL))
		for(vt=1; vt<=6; vt++)
			if(bashpid[vt]==bash) {
				bashpid[vt]=spawnbash(vt, 1);
				break;
			}
	return 0;
}
