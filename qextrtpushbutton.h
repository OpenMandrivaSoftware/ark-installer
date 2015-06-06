#ifndef _QEXTRTPUSHBUTTON_H_
#define _QEXTRTPUSHBUTTON_H_ 1
#include <QLabel>
#include <QEvent>
class QextRtPushButton:public QLabel
{
	Q_OBJECT
public:
	QextRtPushButton(const QString &text, QWidget *parent);
protected slots:
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void keyReleaseEvent(QKeyEvent *e);
	void paintEvent(QPaintEvent *e);
signals:
	void clicked();
};
#endif
