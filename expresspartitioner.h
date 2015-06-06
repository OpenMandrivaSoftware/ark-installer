#ifndef _EXPRESSPARTITIONER_H_
#define _EXPRESSPARTITIONER_H_ 1
#include "partitioner.h"
#include "qextrtpushbutton.h"
#include <QLabel>

class ExpressPartitioner:public Partitioner
{
	Q_OBJECT
public:
	ExpressPartitioner(QWidget *parent, Qt::WindowFlags f=0);
	void	init();
	void	resizeEvent(QResizeEvent *e);
signals:
	void	restartSys();
	void	restartWin();
private:
	QextRtPushButton	*SystemInstall;
	QextRtPushButton	*WindowsInstall;
	QextRtPushButton	*Abort;
};
#endif
