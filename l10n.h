#ifndef __L10N_H__
#define __L10N_H__ 1
#include <QFrame>
#include <QLabel>
#include <QComboBox>
#include <QEvent>
#include <QPushButton>
#include <QTranslator>

struct langtype {
	const char *name;
	const char *englishname;
	const char *flag;
	const char *code;
	const char *charset;
	const char *font;
	const char *defaultkbd;
	const char *defaulttz;
};

struct kbdtype {
	const char *name;
	const char *englishname;
	const char *consoletype;
	const char *x11layout;
	const char *x11variant;
	const char *x11options;
};

class Installer;

class L10N:public QFrame
{
	Q_OBJECT
public:
	L10N(Installer *parent=0, Qt::WindowFlags f=0);
	~L10N();
	void	resizeEvent(QResizeEvent *e);
protected slots:
	void	okClicked();
	void	kbdChanged(int kbd) { _kbdChanged=true; }
	void	tzChanged(int tz) { _tzChanged=true; }
	void	languageChanged(int lang);
signals:
	void	done(langtype language, kbdtype keyboard, QString timezone);
private:
	QTranslator	*interimTranslator;
	QLabel		*info;
	QLabel		*langLbl;
	QComboBox	*lang;
	QLabel		*info2;
	QLabel		*kbdLbl;
	QComboBox	*kbd;
	QLabel		*tzLbl;
	QComboBox	*tz;
	QLabel		*help;
	QPushButton	*ok;
	Installer	*_parent;
	bool		_kbdChanged;
	bool		_tzChanged;
	bool		languageSelected;
};
#endif
