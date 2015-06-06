#ifndef __PACKAGES_H__
#define __PACKAGES_H__ 1
#include <QProcess>
#include <QStringList>
#include <QTextStream>

class Packages:public QObject {
	Q_OBJECT
public:
	struct Progress {
		QString	currentPkg;
		int	progress;
		int	totalprogress;
	};
	Packages(QObject *parent=0):QObject(parent),_success(true),_baddisc(false) { }
	bool install(QStringList const &packages, QString const &chroot=QString::null);
	QString errors() const;
protected slots:
	void rpmDone();
	void dataStdout();
	void dataStderr();
signals:
	void progress(Packages::Progress *p);
	void pkgDone(bool success);
private:
	bool		_success;
	bool		_baddisc;
	Progress	info;
	double		package;
	double		totalpackages;
	QProcess	*rpm;
	QTextStream	*_io;
	QStringList	_errors;
	QStringList	_failedDeps;
	QStringList	_conflicts;
	QStringList	_brokenPkgs;
	QStringList	_rpmOut;
};
#endif
