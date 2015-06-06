#include "qextrtpushbutton.moc"
#include <QPainter>
#include <QShortcut>
#include <QStyle>
#include <QStyleOption>
#include <QMessageBox>
#include <QMouseEvent>
#include <ctype.h>

QextRtPushButton::QextRtPushButton(const QString &text, QWidget *parent):QLabel(text, parent)
{
	if(text.contains('&')) {
		QString realTxt=text;
		int loc=text.indexOf('&');
		QChar accel=text.at(loc+1);
		realTxt.insert(loc+2, "</u>");
		realTxt.replace(loc, 1, "<u>");
		setText(realTxt);
		QShortcut *a=new QShortcut("Alt+" + QString(accel), this);
		connect(a, SIGNAL(activated()), this, SIGNAL(clicked()));
	}
	setWordWrap(true);
	setFrameStyle((QFrame::Shape)(QFrame::WinPanel | QFrame::Raised));
	setFocusPolicy(Qt::StrongFocus);
	setLineWidth(1);
	setMidLineWidth(0);
}

void QextRtPushButton::mousePressEvent(QMouseEvent *e)
{
	if((frameStyle() & QFrame::Raised) && (e->button() == Qt::LeftButton)) {
		setFrameStyle((QFrame::Shape)(QFrame::WinPanel | QFrame::Sunken));
		repaint();
	} else
		QLabel::mousePressEvent(e);
}

void QextRtPushButton::mouseReleaseEvent(QMouseEvent *e)
{
	if((frameStyle() & QFrame::Sunken) && (e->button() == Qt::LeftButton)) {
		setFrameStyle((QFrame::Shape)(QFrame::WinPanel | QFrame::Raised));
		repaint();
		emit clicked();
	} else
		QLabel::mouseReleaseEvent(e);
}

void QextRtPushButton::keyPressEvent(QKeyEvent *e)
{
	if((frameStyle() & QFrame::Raised) && (e->text()==" " || e->text()=="\n" || e->text()=="\r")) {
		setFrameStyle((QFrame::Shape)(QFrame::WinPanel | QFrame::Sunken));
		repaint();
		e->accept();
	} else
		QLabel::keyPressEvent(e);
}

void QextRtPushButton::keyReleaseEvent(QKeyEvent *e)
{
	if((frameStyle() & QFrame::Sunken) && (e->text()==" " || e->text()=="\n" || e->text()=="\r")) {
		setFrameStyle((QFrame::Shape)(QFrame::WinPanel | QFrame::Raised));
		e->accept();
		repaint();
		emit clicked();
	} else
		QLabel::keyReleaseEvent(e);
}

void QextRtPushButton::paintEvent(QPaintEvent *e)
{
	QLabel::paintEvent(e);
	if(hasFocus()) {
		QStyleOptionFocusRect option;
		option.init(this);
		option.backgroundColor = palette().color(QPalette::Background);
		QPainter p(this);
		style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p, this);
	}
}
