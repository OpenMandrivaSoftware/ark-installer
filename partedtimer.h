#ifndef _QT_PARTEDTIMER_H
#define _QT_PARTEDTIMER_H 1
#include <qobject.h>
extern "C" {
#include <parted/parted.h>
}

/** @brief Parted timer handler for Qt applications
 */
class PartedTimer:public QObject {
	Q_OBJECT
public:
	PartedTimer();
	~PartedTimer();
	PedTimer *timer() { return _timer; }
private:
	void emitInfo(PedTimer *t);
signals:
	void info(PedTimer *t);
protected:
	PedTimer *_timer;
};
#endif
