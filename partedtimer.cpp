/* Parted timer handling for Qt applications       *
 * (c) 2002 Bernhard Rosenkraenzer <bero@arklinux.org> */

// The private->public hack is very ugly, but necessary if we want
// to allow a C-(parted-)style callback to emit a Qt signal without
// making an emit method public...
// Let's hope noone notices. ;)
#define private public
#include "partedtimer.moc"
#undef private
#include <qapplication.h>

extern "C" {
	static void _qt_parted_timer_handler(PedTimer *__timer, void *context);
}

PartedTimer::PartedTimer():QObject() {
	_timer=ped_timer_new(_qt_parted_timer_handler, (void*)this);
}

PartedTimer::~PartedTimer() {
	ped_timer_destroy(_timer);
}

void PartedTimer::emitInfo(PedTimer *t) {
	emit info(t);
}

extern "C" {
	static void _qt_parted_timer_handler(PedTimer *__timer, void *context) {
		for(int i=0; i<1000; i++)
			qApp->processEvents();
		PartedTimer *pt=(PartedTimer*)context;
		pt->emitInfo(__timer);
		for(int i=0; i<1000; i++)
			qApp->processEvents();
	}
};
