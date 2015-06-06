#ifndef __INSTALLER_H__
#define __INSTALLER_H__ 1
#include "background.h"
#include "installmode.h"
#include "pkginstaller.h"
#include "l10n.h"
#include <qframe.h>
#include <qtranslator.h>
class Installer:public Background {
	friend class L10N;
	Q_OBJECT
public:
	Installer();
protected slots:
	/** @brief L10N settings done... */
	void l10nDone(langtype lang, kbdtype kbd, QString tz);
	/** @brief InstallMode finished... */
	void modeSelected(InstallMode::Mode m);
	/** @brief partitioning finished... */
	void partitioningDone();
	/** @brief Package installation finished... */
	void pkgInstallDone(bool ok);
	/** @brief Post-install configuration finished... */
	void configDone();
	void systemSelected();
	void windowsSelected();
	void expressSelected();
private:
	int		w, h;

	InstallMode		*mode;
	L10N			*l10n;
	PkgInstaller		*pkgInst;
	Partitioner		*part;
	QTranslator		*trans;
	langtype		_language;
	kbdtype			_keyboard;
	QString			_timezone;
};
#endif
