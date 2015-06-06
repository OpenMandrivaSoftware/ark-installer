#ifndef _INITIALINFO_H_
#define _INITIALINFO_H_ 1
#include <QLabel>
#include <QPushButton>
#include <QEvent>
#include "qextrtpushbutton.h"
class InstallMode:public QFrame
{
	Q_OBJECT
public:
	InstallMode(QWidget *parent=0, Qt::WFlags f=0);
	enum Mode { System, Express, Windows, Expert, ERROR };
	Mode type() { return t; }
signals:
	void done(InstallMode::Mode m);
protected slots:
	void systemClicked();
	void expressClicked();
	void windowsClicked();
	void expertClicked();
	void resizeEvent(QResizeEvent *e);
private:
	QLabel			*info;
	QextRtPushButton	*SystemInstall;
	QextRtPushButton	*ExpressInstall;
	QextRtPushButton	*ParallelInstall;
	QextRtPushButton	*ExpertInstall;
	Mode			t;
};
#endif
