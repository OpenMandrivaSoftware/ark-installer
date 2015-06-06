#ifndef _WINDOWSPARTITIONER_H_
#define _WINDOWSPARTITIONER_H_ 1
#include "partitioner.h"
#include "installmode.h"
#include <QEvent>
#include <QComboBox>
#include <QSlider>
class WindowsPartitioner:public Partitioner
{
	Q_OBJECT
public:
	WindowsPartitioner(QWidget *parent, Qt::WindowFlags f=0);
	void init();
	void resizeEvent(QResizeEvent *e);
protected slots:
	void driveSelected();
	void do_resize();
	/** @brief update the progress bar */
	void changed(int value);
signals:
	/** @brief restart in System Install mode */
	void restartSys();
	/** @brief restart in Express Install mode */
	void restartExp();
private:
	enum status { Starting, NoWin, NoSpace, OneWin, MultiWin, Resizing } _status;
	void			drv_resize();
	QextRtPushButton	*SystemInstall;
	QextRtPushButton	*ExpressInstall;
	QextRtPushButton	*Abort;
	QComboBox		*drives;
	QextRtPushButton	*ok;
	QLabel			*current;
	QSlider			*retain;
	char			*windrive[26];
	int			winpart[26];
	unsigned long		winsize[26];
	unsigned long		minsize[26];
	int			win_dev;
};
#endif
