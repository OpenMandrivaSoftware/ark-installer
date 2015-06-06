/* End User Linux installer - v 0.1                                *
 * (c) 2001-2005 Bernhard Rosenkraenzer <bero@arklinux.org>    *
 * Hereby released under the terms of the GNU GPL v2.              *
 *                                                                 *
 * Yes, I know using Q*Layout is nicer code - however, I want this *
 * to compile with minimized versions of Qt/Embedded.              */
#include "dist.h"
#include "background.moc"
#include <qfont.h>
#include <qapplication.h>
#include <qdesktopwidget.h>
#include <qpainter.h>
#include <float.h>
#include <qpixmap.h>
using namespace std;
Background::Background(QWidget *parent, Qt::WFlags f):QWidget(parent, f)
{
	if(!parent)
		parent=static_cast<QWidget*>(qApp->desktop());
#ifdef Q_WS_X11
	w=1024; h=768;
#else
	w=parent->width();
	h=parent->height();
	move(0, 0);
#endif
	setFixedSize(w, h);
	if(w<1024) { // Yuck. We need to scale down Tux. :/
		QPixmap tux_p("/images/tux.png");
		tux_p=tux_p.scaledToWidth(tux_p.width()/2, Qt::SmoothTransformation);
		tux=new QImage(tux_p.toImage());
		QPixmap help_p("/images/help.png");
		help_p=help_p.scaledToWidth(float(help_p.width())*float(w)/float(1024), Qt::SmoothTransformation);
		help=new QImage(help_p.toImage());
	} else {
		tux=new QImage(QString("/images/tux.png"));
		help=new QImage(QString("/images/help.png"));
	}
}

void Background::paintEvent(QPaintEvent *e)
{
	QPainter paint(this);
	if(QPaintDevice::depth()>=8) {
		float step=height();
		step/=255;
		for(float i=0; i<height(); i+=step) {
			paint.setBrush(QColor(0, 0, i/step));
			paint.setPen(QColor(0, 0, i/step));
			paint.drawRect(0, i, width(), step+1);
		}
	} else {
		// Yuck. VGA16FB.
		paint.setBrush(Qt::blue);
		paint.setPen(Qt::blue);
		paint.drawRect(0, 0, width(), height());
	}
	paint.drawImage(w-tux->width(), h-tux->height(), *tux);
	paint.setBrush(Qt::white);
	paint.setPen(Qt::white);
#ifdef Q_WS_QWS
	QFont f("lucidux_sans", 36, QFont::Black);
#else
	QFont f("Liberation Sans", 28, QFont::Black);
#endif
	paint.setFont(f);
	paint.drawText(5, 37, DIST" "VERSION" "EDITION);
#ifdef XP
	paint.setBrush(Qt::red); paint.setPen(Qt::red);
#ifdef Q_WS_QWS
	f=QFont("lucidux_sans", 24, QFont::Black);
#else
	f=QFont("Liberation Sans", 22, QFont::Black);
#endif
	paint.setFont(f);
	paint.drawText(5, 64, tr("eXPerimental"));
	paint.setBrush(Qt::white); paint.setPen(Qt::white);
#endif
	if(help_x >= 0 && help_y >= 0) {
		// Help for language selection screen
		paint.drawImage(help_x, help_y, *help);
		paint.setBrush(Qt::white); paint.setPen(Qt::white);
		paint.drawText(help_x+help->width()/2.9, help_y+help->height()/4.5, tr("1. Move mouse"));
		paint.drawText(help_x+help->width()/2.9, help_y+help->height()/1.7, tr("2. Press left button"));
	}
#ifdef Q_WS_QWS
	f=QFont("SansSerif", 16);
#else
	f=QFont("Liberation Sans", 8);
#endif
	paint.setFont(f);
	paint.drawText(5, h-17, QString::fromUtf8("© 2002-2011 Bernhard \"Bero\" Rosenkränzer <bero@arklinux.org>"));
}
