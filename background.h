#ifndef __BACKGROUND_H__
#define __BACKGROUND_H__ 1
#include <qwidget.h>
#include <qimage.h>
/** @brief Paint the background for the installer */
class Background:public QWidget {
	Q_OBJECT
public:
	Background(QWidget *parent=0, Qt::WFlags f=0);
public slots:
	void	showHelp(int helpx=-1, int helpy=-1) { help_x=helpx; help_y=helpy; paintEvent(0); }
protected slots:
	/** @brief Overloaded for internal reasons */
	void paintEvent(QPaintEvent *e);
private:
	int		w, h;
	int		help_x, help_y;
	QImage		*tux;
	QImage		*help;
};
#endif
