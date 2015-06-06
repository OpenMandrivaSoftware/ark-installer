/* Ark Linux installer - v 0.1                                     *
 * (c) 2001-2007 Bernhard Rosenkraenzer <bero@arklinux.org>        *
 * Hereby released under the terms of the GNU GPL v3.              *
 *                                                                 *
 * This code was written without Q*Layout because it was supposed  *
 * to run on a minimized versions of Qt/Embedded back in 2001.     *
 * Feel free to use QLayouts now.                                  */
#include "dist.h"
#include "installer.moc"
#include <QApplication>
#include <QFont>
#include <QPainter>
#include <QMessageBox>
#include "systempartitioner.h"
#include "expresspartitioner.h"
#include "windowspartitioner.h"
#include "expertpartitioner.h"
#include "debug.h"
#include <iostream>
using namespace std;
Installer::Installer():Background(0, Qt::FramelessWindowHint), part(0)
{
	l10n=new L10N(this);
	if(width()>=800)
		l10n->setGeometry(width()/2-400, height()/2-200,400,400);
	else // vga16fb...
		l10n->setGeometry(120, 40, 400, 400);

	connect(l10n, SIGNAL(done(langtype,kbdtype,QString)), this, SLOT(l10nDone(langtype,kbdtype,QString)));
	showHelp(l10n->x() + l10n->width()-5, l10n->y()+23);
	show();
}

void Installer::l10nDone(langtype lang, kbdtype keyboard, QString timezone)
{
	_language=lang;
	_keyboard=keyboard;
	_timezone=timezone;
	trans=new QTranslator(0);
	QString langcode=lang.code;
	if(langcode.contains('_'))
		langcode=langcode.section('_', 0, 0);
	trans->load(QString("/i18n/installer_%1.qm").arg(langcode));
	qApp->installTranslator(trans);
	l10n->hide();
	delete l10n; l10n=0;
	showHelp();
	cerr << "Calling InstallMode constructor" << endl;
	mode=new InstallMode(this);
	cerr << "Returned from InstallMode constructor" << endl;
	mode->setGeometry(width()/2-200, height()/2-200, 400, 400);
	connect(mode, SIGNAL(done(InstallMode::Mode)), this, SLOT(modeSelected(InstallMode::Mode)));
	mode->show();
}

void Installer::modeSelected(InstallMode::Mode m)
{
	if(mode) {
		mode->hide();
		delete mode;
		mode=0;
	}
	if(part) {
		delete part;
		part=0;
	}
	switch(m) {
	case InstallMode::System:
		part=new SystemPartitioner(this);
//		cout << "system selected" << endl;
		break;
	case InstallMode::Express:
		part=new ExpressPartitioner(this);
		connect(part, SIGNAL(restartSys()), this, SLOT(systemSelected()));
		connect(part, SIGNAL(restartWin()), this, SLOT(windowsSelected()));
		break;
	case InstallMode::Windows:
		part=new WindowsPartitioner(this);
		connect(part, SIGNAL(restartSys()), this, SLOT(systemSelected()));
		connect(part, SIGNAL(restartExp()), this, SLOT(expressSelected()));
		break;
	case InstallMode::Expert:
		part=new ExpertPartitioner(this);
		break;
	default:
		MSG("Impossible condition triggered. Notify info@arklinux.org.");
	}
	connect(part, SIGNAL(done()), this, SLOT(partitioningDone()));
	part->setGeometry(width()/2-200, height()/2-200, 400, 400);
	part->show();
	part->init();
}

void Installer::partitioningDone()
{
	part->hide();

	pkgInst=new PkgInstaller(this);
	connect(pkgInst, SIGNAL(finished(bool)), this, SLOT(pkgInstallDone(bool)));
	connect(pkgInst, SIGNAL(postDone()), this, SLOT(configDone()));

	pkgInst->setGeometry(width()/2-200, height()/2-235, 400, 470);
	pkgInst->show();
	pkgInst->start(QStringList(), part);
}

void Installer::systemSelected()
{
	modeSelected(InstallMode::System);
}

void Installer::expressSelected()
{
	modeSelected(InstallMode::Express);
}

void Installer::windowsSelected()
{
	modeSelected(InstallMode::Windows);
}

void Installer::pkgInstallDone(bool ok) {
	if(!ok) {
		QMessageBox::information(this, tr("Installation failed!"), tr("Your %1 %2 installation has failed\nbecause of the following error(s):\n%3").arg(DIST).arg(VERSION).arg(pkgInst->errors()));
		qApp->quit();
	}
	pkgInst->packagesDone(_language, _keyboard, _timezone);
}

void Installer::configDone()
{
	bool paused=pkgInst->game_paused();
	if(!paused)
		pkgInst->pause_game(); // Let's not kill the user's Tetrix highscore... ;)
	if(QMessageBox::information(this, tr("Installation done!"), tr("%1 %2 is now installed on your system.\nWhen you're done playing Tetrix, simply press the reset button (or turn the computer off and back on)\nand remove the installation CD.\n\nTo continue playing, just select the OK button and your game will proceed.").arg(DIST).arg(VERSION), QString::null, tr("I'm done"))==1) {
		qApp->quit();
	}
	if(!paused)
		pkgInst->pause_game();
}
