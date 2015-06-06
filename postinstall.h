#ifndef __POSTINSTALL_H__
#define __POSTINSTALL_H__ 1
#include <QProcess>
#include "l10n.h"
#include "partitioner.h"
class PostInstall:public QObject {
	Q_OBJECT
public:
	PostInstall(QString const &rootDir, QString const &bootDir, Partitioner::GrubLocation grub, langtype _language, kbdtype _keyboard, QString _timezone, QObject *parent=0);
	void	start();
protected slots:
	void	configureL10N();
	void	configurePcmcia();
	void	configureHardware();
	void	configureInitrd();
	void	configureFonts();
	void	configureGrub();
	void	configureLogin();
	void	configureNet();
	void	configurePassword();
	void	prelink();
	void	postInstall();
	void	pcmciaDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	hardwareDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	initrdDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	fontsDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	grubDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	loginDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	netDone_();
	void	prelinkDone_(int exitCode, QProcess::ExitStatus exitStatus);
	void	passwordDone_(int exitCode, QProcess::ExitStatus exitStatus);
signals:
	void	L10NDone(bool success);
	void	pcmciaDone(bool success);
	void	hardwareDone(bool success);
	void	initrdDone(bool success);
	void	fontsDone(bool success);
	void	grubDone(bool success);
	void	loginDone(bool success);
	void	netDone(bool success);
	void	prelinkDone(bool success);
	void	progress(QString, int);
	void	passwordDone(bool success);
	void	postInstallDone();
	void	cfgDone();
private:
	QStringList	XDriver(QString const &cardName);
	QString		root;
	QString		boot;
	QProcessEnvironment	env;
	double		currentTask;
	double		numTasks;
	QProcess	*grub;
	QProcess	*sndconfig;
	QProcess	*probe;
	QProcess	*mkinitrd;
	QProcess	*_coldplug;
	QProcess	*adduser;
	QProcess	*chkfontpath;
	QProcess	*passwd;
	langtype	_lang;
	kbdtype		_kbd;
	QString		_tz;
	Partitioner::GrubLocation	_grub;
};
#endif
