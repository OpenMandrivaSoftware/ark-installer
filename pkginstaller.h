#ifndef __PKGINSTALLER_H__
#define __PKGINSTALLER_H__ 1
#include "partitioner.h"
#include "postinstall.h"
#include "packages.h"
#include "tetrixboard.h"
#include <QFrame>
#include <QProgressBar>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QLCDNumber>

class PkgInstaller:public QFrame {
	friend class Installer;
	Q_OBJECT
public:
	PkgInstaller(QWidget *parent=0);
	void start(QStringList packages, Partitioner *partitions);
	void pause_game() { game->pause(); }
	bool game_paused() const { return game->paused(); }
	QString errors() const { return _handler ? _handler->errors() : QString::null; }
signals:
	void finished(bool success);
	void postDone();
protected slots:
	void updateProgress(Packages::Progress *info);
	void updateProgressConfig(QString task, int percentage);
	void resizeEvent(QResizeEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void packagesDone(langtype _language, kbdtype _keyboard, QString timezone);
	void ready();
private:
	Packages	*_handler;
	Partitioner	*_partitions;
	QLabel		*info;
	QLabel		*currentPkg;
	QProgressBar	*progress;
	QLabel		*total;
	QProgressBar	*totalprogress;
	TetrixBoard	*game;
	QPushButton	*newGame;
	QPushButton	*pauseGame;
	QLabel		*scoreLbl;
	QLCDNumber	*score;
	QLabel		*levelLbl;
	QLCDNumber	*level;
	QLabel		*linesLbl;
	QLCDNumber	*lines;
	QLabel		*tetrixinfo;
	PostInstall	*configure;
};
#endif
