# For debugging post-partitioning parts, use -DNONDESTRUCTIVE to bypass all partitioning code
# (packages will be installed into /mnt/dest on the running system)

LIB=$(shell [ `uname -m` = "x86_64" ] && echo lib64 || echo lib)

ifneq ($(X11),)
QTDIR=/usr/$(LIB)/qt4
QT_LIB=-Wl,-rpath -Wl,$(QTDIR)/lib -lQtCore -lQtGui
QTFLAGS=-O2 -Os -fomit-frame-pointer -fvisibility=hidden -I$(QTDIR)/include -I$(QTDIR)/include/QtCore -I$(QTDIR)/include/QtGui -I. -L$(QTDIR)/lib -DQT_NO_COMPAT
else
QTDIR=/usr/$(LIB)/qt4-embedded
QT_LIB=-Wl,-rpath -Wl,$(QTDIR)/lib -lQtCore -Wl,-rpath,$(QTDIR)/lib -lQtGui
QTFLAGS=-O2 -Os -fomit-frame-pointer -fvisibility=hidden -I$(QTDIR)/include -I$(QTDIR)/include/QtCore -I$(QTDIR)/include/QtGui -I. -L$(QTDIR)/lib -DQT_NO_COMPAT -DQWS
endif
CXX=g++ -Wall

# -DNONDESTRUCTIVE

OBJ=main.o installer.o packages.o Files.o background.o installmode.o qextrtpushbutton.o partitioner.o systempartitioner.o windowspartitioner.o expertpartitioner.o expresspartitioner.o ext3.o meminfo.o partedtimer.o pkginstaller.o tetrixboard.o tetrixpiece.o postinstall.o qextconfigfile.o l10n.o mkdir.o modules.o
TESTOBJ=test.o postinstall.o qextconfigfile.o

check: installer
#	sync ; QTDIR=$(QTDIR) ./installer

checktst: test
	./test

xconfig: xconfig.o qextconfigfile.o
	$(CXX) $(CXXFLAGS) $(QTFLAGS) -o $@ xconfig.o qextconfigfile.o -lQtCore -lpci

all: installer

run: all
	QTDIR=$(QTDIR) ./installer -qws

clean:
	@-rm -f installer *.o *.moc core timezones.h *.qm

l10n.o: l10n.moc l10n.cpp timezones.h

postinstall.o: postinstall.moc postinstall.cpp

tetrixboard.o: tetrixboard.moc tetrixboard.cpp

qextrtpushbutton.o: qextrtpushbutton.moc qextrtpushbutton.cpp

installer.o: installer.moc installer.cpp

background.o: background.moc background.cpp

installmode.o: installmode.moc installmode.cpp

partitioner.o: partitioner.moc partitioner.cpp

packages.o: packages.moc packages.cpp

systempartitioner.o: systempartitioner.moc systempartitioner.cpp

windowspartitioner.o: windowspartitioner.moc windowspartitioner.cpp

expertpartitioner.o: expertpartitioner.moc expertpartitioner.cpp

expresspartitioner.o: expresspartitioner.moc expresspartitioner.cpp

partedtimer.o: partedtimer.moc partedtimer.cpp

pkginstaller.o: pkginstaller.moc pkginstaller.cpp

mkdir.o: mkdir.cpp mkdir.h

installer: $(OBJ)
	$(CXX) $(CXXFLAGS) $(QTFLAGS) -o $@ $(OBJ) $(QT_LIB) -lparted -lext2fs /usr/$(LIB)/libpci.a -lz
	strip -R .comment --strip-unneeded installer

test: $(TESTOBJ)
	$(CXX) $(CXXFLAGS) $(QTFLAGS) -o $@ $(TESTOBJ) $(QT_LIB) -lpci
	./test

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(QTFLAGS) -o $@ -c $<

%.moc: %.h
	$(QTDIR)/bin/moc -o $@ $<

messages: installer_de.ts

installer_de.ts:
	$(QTDIR)/bin/lupdate installer.pro

installer_de.qm:
	$(QTDIR)/bin/lrelease installer.pro

timezones.h:
	echo '#ifndef __TIMEZONES_H__' >timezones.h
	echo '#define __TIMEZONES_H__ 1' >>timezones.h
	echo 'static char const * const timezones[] = {' >>timezones.h
	find /usr/share/zoneinfo/posix/ -type f | sort | sed -e 's@/usr/share/zoneinfo/posix/@@;s@^@    \"@;s@$$@\",@' >>timezones.h
	echo '    NULL' >>timezones.h
	echo '};' >>timezones.h
	echo '#endif' >>timezones.h

uitest: uitest.o expertpartitioner.o partitioner.o meminfo.o partedtimer.o ext3.o qextrtpushbutton.o partedtimer.o
	$(CXX) $(CXXFLAGS) $(QTFLAGS) -o $@ uitest.o expertpartitioner.o partitioner.o meminfo.o ext3.o qextrtpushbutton.o partedtimer.o $(QT_LIB) -lparted -lext2fs /usr/$(LIB)/libpci.a
