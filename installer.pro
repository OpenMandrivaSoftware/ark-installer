# We don't actually use qmake, this is just a fake qmake file
# to allow lupdate to work.
HEADERS	= background.h dist.h expresspartitioner.h ext3.h Files.h gtetrix.h \
	  installer.h installmode.h l10n.h meminfo.h packages.h partedtimer.h \
	  partitioner.h pkginstaller.h postinstall.h qextconfigfile.h \
	  qextrtpushbutton.h qtetrixb.h systempartitioner.h \
	  timezones.h tpiece.h windowspartitioner.h

SOURCES = background.cpp expresspartitioner.cpp ext3.cpp Files.cpp \
	  gtetrix.cpp installer.cpp installmode.cpp l10n.cpp \
	  main.cpp meminfo.cpp packages.cpp partedtimer.cpp \
	  partitioner.cpp pkginstaller.cpp postinstall.cpp \
	  qextconfigfile.cpp qextrtpushbutton.cpp qtetrixb.cpp \
	  systempartitioner.cpp tpiece.cpp windowspartitioner.cpp

TRANSLATIONS = installer_de.ts installer_it.ts installer_nl.ts installer_fr.ts \
	installer_ee.ts installer_el.ts installer_ct.ts installer_es.ts installer_pl.ts installer_et.ts \
	installer_th.ts installer_ru.ts installer_am.ts
