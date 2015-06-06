#include "packages.moc"
#include <qmessagebox.h>
#include <iostream>
using namespace std;
bool Packages::install(QStringList const &packages, QString const &chroot)
{
	info.currentPkg=tr("Determining installation order");
//	cout << "determining installation order" << endl;

	QString cmdline;
	FILE *f=fopen("/proc/cmdline", "r");
	if(f) {
		char buf[1024];
		fgets(buf, 1024, f);
		fclose(f);
		cmdline=buf;
	}

	info.progress=0;
	info.totalprogress=0;
	totalpackages=packages.count();
	package=0;
	rpm=new QProcess(this);
	QStringList args;
	args << "/bin/rpm" << "-iv";
	if(cmdline.contains("nodeps"))
		args << "--nodeps" ;
	if(cmdline.contains("force"))
		args << "--force";
#ifdef TEXTONLY
	args << "--excludedocs"
	     << "--excludepath"
	     << "/usr/share/info"
	     << "--excludepath"
	     << "/usr/share/man"
	     << "--excludepath"
	     << "/usr/share/doc"
	     << "--excludepath"
	     << "/usr/docs"
#endif
	args << "--percent";
	if(!chroot.isEmpty())
		args << "-r" << chroot;
	foreach(QString const &pkg, packages)
		args << pkg;

	connect(rpm, SIGNAL(processExited()), this, SLOT(rpmDone()));
	connect(rpm, SIGNAL(readyReadStdout()), this, SLOT(dataStdout()));
	connect(rpm, SIGNAL(readyReadStderr()), this, SLOT(dataStderr()));
	
	emit progress(&info);
	
	QProcessEnvironment rpmEnv;
	rpmEnv.clear();
	rpmEnv.insert("PATH", "/bin:/usr/bin:/sbin:/usr/sbin:" + chroot + "/bin:" + chroot + "/usr/bin:" + chroot + "/sbin:" + chroot + "/usr/sbin");
	rpmEnv.insert("HOME", "/tmp");
	rpmEnv.insert("USER", "root");
	rpmEnv.insert("USERNAME", "root");
	rpm->setProcessEnvironment(rpmEnv);
	rpm->start("/bin/rpm", args);
	_io = new QTextStream(rpm);
	return true;
}

void Packages::rpmDone()
{
//	emit pkgDone(rpm->normalExit() && (rpm->exitStatus()==0));
//	unfortunately rpm's exit status is not reliable information, it returns
//	error states even on warnings...
//	Checking the output for "error:" seems to be more reliable. :/
//	QMessageBox::information(0, "rpm exited", "Rpm exited! Status: " + QString::number(_success) + "/" + QString::number(_baddisc));
	if(!_brokenPkgs.isEmpty()) {
		FILE *f=fopen("/mnt/dest/tmp/rpminstall.log", "w");
		if(f) {
			for(QStringList::ConstIterator it=_rpmOut.begin(); it!=_rpmOut.end(); it++)
				fprintf(f, "%s\n", qPrintable(*it));
			fclose(f);
		} else
			QMessageBox::information(0, "rpm stderr", _rpmOut.join("\n"));
		_rpmOut.clear();
	}
	emit pkgDone(_success);
}

void Packages::dataStdout()
{
	QString s;
	while(!(s=_io->readLine()).isNull()) {
		if(s.left(1)=="%") {
			info.progress=int(s.mid(3).simplified().toDouble());
			info.totalprogress=int(float(100)*((package+(info.progress/100))/totalpackages));
		} else {
			_rpmOut << s;
			info.currentPkg=s.simplified();
			info.progress=0;
			info.totalprogress=int(float(100)*(++package/totalpackages));
		}
		emit progress(&info);
	}
}

void Packages::dataStderr()
{
	QByteArray err=rpm->readAllStandardError();
	QTextStream io(err, QIODevice::ReadOnly);
	QString s;
	while(!(s=io.readLine()).isNull()) {
		_rpmOut << s;
//		cout << s.local8Bit() << endl;
	//	if(s.startsWith("error:"))
	//		_success=false;

		if(s.contains("conflicts with")) {
			_conflicts.append(s);
			_success=false;
		} else if(s.contains("is needed")) {
			_failedDeps.append(s);
			_success=false;
		} else if(s.contains("input/output error")) {
			_success=false; _baddisc=true;
		} else if(s.contains("transaction lock") || s.contains("LOOP") || s.contains("from tsort relations")) {
			// harmless rpm bug
			continue;
		} else if(s.contains("No space left on device")) {
			if(!_errors.contains(tr("You ran out of disk space. Please use bigger partitions.")))
				_errors.append(tr("You ran out of disk space. Please use bigger partitions."));
			_success=false;
		} else if(s.contains("unpacking of archive failed")) {
			_errors.append(tr("There was an error unpacking a file, probably due to an installation ordering issue."));
		} else {
//			QMessageBox::information(0, "rpm stderr", s);
			_brokenPkgs.append(s);
		}
	}
}

QString Packages::errors() const
{
	QString e;
	if(!_errors.isEmpty())
		e += _errors.join("\n") + "\n";
	if(!_failedDeps.isEmpty()) {
		e += tr("Failed dependencies:\n");
		e += _failedDeps.join("\n") + "\n";
	}
	if(!_conflicts.isEmpty()) {
		e += tr("Conflicting packages:\n");
		e += _conflicts.join("\n") + "\n";
	}

	if(!e.isEmpty())
		e += tr("Please report this to development@arklinux.org.");

	// Overwrite everything if we had an I/O error -- that can cause a perfectly OK
	// package to fail, causing dependency problems..
	if(_baddisc)
		e = tr("Parts of the installation CD could not be read.\n"
		       "This is typically either a bad CD burn or a bad download.\n"
		       "If you're using an older CD drive, you may want to try\n"
		       "burning at a lower speed.");
	return e;
}
