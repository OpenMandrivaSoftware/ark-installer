#include "l10n.moc"
#define MAX(x,y) x > y ? x : y
#define MAX3(x,y,z) (x > y) ? (x > z ? x : z) : (y > z ? y : z)

#include "timezones.h"
#include <QApplication>
#include <QTranslator>
#include <QFile>
#include "installer.h"
#include "debug.h"

// { Language in its own name, Language English name, short name, locale, charset, text mode font, keyboard layout, default timezone }

static struct langtype const languages[] = {
	{ "Català", "Catalan", "ct", "ca_ES.UTF-8", "UTF-8", "LatArCyrHeb-16", "es", "Europe/Andorra" },
#if 1
	{ "中文", "Chinese", "cn", "zh_CN.UTF-8", "UTF-8", "gb2312", "us", "Asia/Shanghai" },
#endif
	{ "Deutsch", "German", "de", "de_DE.UTF-8", "UTF-8", "LatArCyrHeb-16", "de-latin1-nodeadkeys", "Europe/Berlin" },
	{ "Deutsch (Österreich)", "German (Austria)", "at", "de_AT.UTF-8", "UTF-8", "LatArCyrHeb-16", "de-latin1-nodeadkeys", "Europe/Vienna" },
	{ "Deutsch (Schweiz)", "German (Switzerland)", "ch", "de_CH.UTF-8", "UTF-8", "LatArCyrHeb-16", "sg-latin1", "Europe/Zurich" },
	{ "Dutch", "Dutch", "nl", "nl_NL.UTF-8", "UTF-8", "LatArCyrHeb-16", "nl2", "Europe/Amsterdam" },
	{ "Dutch (België)", "Dutch (Belgium)", "be", "nl_BE.UTF-8", "UTF-8", "LatArCyrHeb-16", "be-latin1", "Europe/Brussels" },
	{ "Eesti", "Estonian", "ee", "et_EE.UTF-8", "UTF-8", "LatArCyrHeb-16", "et-nodeadkeys", "Europe/Tallinn" },
	{ "Ellinika", "Greek", "el", "el_GR.UTF-8", "UTF-8", "LatArCyrHeb-16", "gr", "Europe/Athens" },
	{ "English (Australia)", "English (Australia)", "au", "en_AU.UTF-8", "UTF-8", "LatArCyrHeb-16", "us", "Australia/Canberra" },
	{ "English (Canada)", "English (Canada)", "ca", "en_CA.UTF-8", "UTF-8", "LatArCyrHeb-16", "us", "Canada/Pacific" },
	{ "English (UK)", "English (UK)", "gb", "en_GB.UTF-8", "UTF-8", "LatArCyrHeb-16", "uk", "Europe/London" },
	{ "English (US)", "English (US)", "us", "en_US.UTF-8", "UTF-8", "LatArCyrHeb-16", "us", "America/New_York" },
	{ "Español", "Spanish", "es", "es_ES.UTF-8", "UTF-8", "LatArCyrHeb-16", "es", "Europe/Madrid" },
	{ "Français", "French", "fr", "fr_FR.UTF-8", "UTF-8", "LatArCyrHeb-16", "fr-latin0", "Europe/Paris" },
	{ "Italiano", "Italian", "it", "it_IT.UTF-8", "UTF-8", "LatArCyrHeb-16", "it", "Europe/Rome" },
	{ "Italiano (Svizzera)", "Italian (Switzerland)", "it", "it_CH.UTF-8", "UTF-8", "LatArCyrHeb-16", "it", "Europe/Zurich" },
	{ "Polski", "Polish", "pl", "pl_PL.UTF-8", "UTF-8", "LatArCyrHeb-16", "pl", "Europe/Warsaw" },
	{ NULL, NULL, NULL, NULL, NULL }
};

static struct kbdtype const keyboards[] = {
	{ NULL,		"Belgian",	"be-latin1",	"be",	"", "" },
	{ NULL,		"Bulgarian",	"bg",		"bg",	"", "" },
	{ NULL,		"Brazil",	"br-abnt2",	"br",	"", "" },
	{ NULL,		"French Canadian",
					"cf",		"ca_enhanced","", "" },
	{ NULL,		"Czech",	"cz-lat2",	"cz",	"", "" },
	{ NULL,		"Danish",	"dk-latin1",	"dk",	"", "" },
	{ NULL,		"Dvorak",	"dvorak",	"dvorak", "", "" },
	{ "English (UK)",
			"English (UK)",	"uk",		"gb",	"", "" },
	{ "English (US)",
			"English (US)",	"us",		"us",	"", "" },
	{ NULL,		"Estonian",	"et-nodeadkeys",	"ee",	"", "" },
	{ NULL,		"Finnish",	"fi-latin1",	"fi",	"", "" },
	{ "Français",	"French",	"fr-latin0",	"fr",	"", "" },
	{ NULL,		"French (Switzerland)",
					"fr_CH",	"ch","fr_nodeadkeys", "" },
	{ "Deutsch",	"German",	"de-latin1-nodeadkeys",
							"de",	"nodeadkeys", "" },
	{ "Deutsch (Schweiz)",
			"German (Switzerland)",
					"sg-latin1",	"ch","de_scriptdeadkeys", "" },
	{ "Ellinika",	"Greek",	"gr",		"us(intl),el(extended)",	"", "grp:alt_shift_toggle,compose:menu" },
	{ NULL,		"Hungarian",	"hu",		"hu",	"", "" },
	{ NULL,		"Icelandic",	"is-latin1",	"is",	"", "" },
	{ "Italiano",	"Italian",	"it",		"it",	"", "" },
	{ NULL,		"Japanese",	"jp106",	"jp",	"", "" },
	{ "Nederlands",	"Dutch",	"nl2",		"nl",	"", "" },
	{ NULL,		"Norwegian",	"no-latin1",	"no",	"", "" },
	{ "Polski",	"Polish",	"pl",		"pl",	"", "" },
	{ NULL,		"Portuguese",	"pt-latin1",	"pt",	"", "" },
	{ NULL,		"Russian",	"ru",		"ru",	"", "" },
	{ "Español",	"Spanish",	"es",		"es",	"", "" },
	{ NULL,		"Swedish",	"se-latin1",	"se",	"", "" },
	{ NULL,		"Slovak",	"sk-qwerty",	"us_sk_qwerty", "", "" },
	{ NULL,		"Slovene",	"slovene",	"si",	"", "" },
	{ NULL,		"Turkish",	"trq",		"tr",	"", "" },
	{ NULL,		"Ukrainian",	"ua",		"ua",	"", "" },
	{ NULL,		NULL,		NULL,		NULL,	NULL, NULL }
};

static void setCurrentText(QComboBox *b, QString const &text) {
	for(int i=0; i<b->maxCount(); i++) {
		if(b->itemText(i) == text) {
			b->setCurrentIndex(i);
			return;
		}
	}
}

L10N::L10N(Installer *parent, Qt::WindowFlags f):QFrame(parent, f),interimTranslator(0),_parent(parent),_kbdChanged(false),_tzChanged(false),languageSelected(false)
{
	setAutoFillBackground(true);
	setFrameShape((QFrame::Shape)(QFrame::Box|QFrame::Sunken));
	setLineWidth(3);
	setMidLineWidth(0);
	info=new QLabel(this);
	langLbl=new QLabel(this);
	lang=new QComboBox(this);
	connect(lang, SIGNAL(activated(int)), this, SLOT(languageChanged(int)));
	langLbl->setBuddy(lang);
	lang->addItem("---- Select language ----");

	for(int i=0; languages[i].code; i++) {
		QPixmap p("/images/" + QString(languages[i].flag) + ".png");
		if(languages[i].name && strlen(languages[i].name))
			lang->addItem(p, QString::fromUtf8(languages[i].name));
		else
			lang->addItem(p, QString::fromUtf8(languages[i].englishname));
	}
	info2=new QLabel(this);
	kbdLbl=new QLabel(this);
	kbd=new QComboBox(this);
	for(int i=0; keyboards[i].consoletype; i++) {
		if(keyboards[i].name)
			kbd->addItem(QString::fromUtf8(keyboards[i].name));
		else
			kbd->addItem(QString::fromUtf8(keyboards[i].englishname));
	}
	connect(kbd, SIGNAL(activated(int)), this, SLOT(kbdChanged(int)));
	kbdLbl->setBuddy(kbd);
	tzLbl=new QLabel(this);
	tz=new QComboBox(this);
	connect(tz, SIGNAL(activated(int)), this, SLOT(tzChanged(int)));
	for(int i=0; timezones[i]; i++)
		tz->addItem(timezones[i]);
	tzLbl->setBuddy(tz);
	help=new QLabel(this);
	ok=new QPushButton(this);
	connect(ok, SIGNAL(clicked()), this, SLOT(okClicked()));
	ok->setEnabled(false);

	// Default to US settings, most people are familiar with the US keyboard layout
	// even if their keyboard is different
	lang->setCurrentIndex(0);
	kbd->setCurrentIndex(8);
	setCurrentText(tz, "Etc/Universal");

	resizeEvent(0);
}

L10N::~L10N()
{
	if(interimTranslator) {
		qApp->removeTranslator(interimTranslator);
		delete interimTranslator;
	}
}

void L10N::okClicked()
{
	emit done(languages[lang->currentIndex()], keyboards[kbd->currentIndex()], tz->currentText());
}

void L10N::languageChanged(int language)
{
	if(!languageSelected && language==0) // invalid input
		return;
	if(!languageSelected) { // First time -- get rid of the help text
		languageSelected=true;
		lang->removeItem(0);
		language--;
		lang->setCurrentIndex(language);
		ok->setEnabled(true);
	}
	QString currentLang=languages[language].code;
	if(!QFile::exists(QString("/i18n/installer_%1.qm").arg(currentLang)) && currentLang.contains('@'))
		currentLang=currentLang.section('@', 0, 0);
	if(!QFile::exists(QString("/i18n/installer_%1.qm").arg(currentLang)) && currentLang.contains('.'))
		currentLang=currentLang.section('.', 0, 0);
	if(!QFile::exists(QString("/i18n/installer_%1.qm").arg(currentLang)) && currentLang.contains('_'))
		currentLang=currentLang.section('_', 0, 0);
	if(interimTranslator) {
		qApp->removeTranslator(interimTranslator);
		delete interimTranslator;
	}
	interimTranslator=new QTranslator(0);
	interimTranslator->load(QString("/i18n/installer_%1.qm").arg(currentLang));
	qApp->installTranslator(interimTranslator);
	// If the user hasn't picked a different keyboard and timezone earlier,
	// set the defaults for the language...
	if(!_kbdChanged && languages[language].defaultkbd) {
		for(int i=0; keyboards[i].consoletype; i++) {
			if(keyboards[i].consoletype && !strcmp(keyboards[i].consoletype, languages[language].defaultkbd)) {
				kbd->setCurrentIndex(i);
				break;
			}
		}
	}
	if(!_tzChanged && languages[language].defaulttz && strlen(languages[language].defaulttz)) {
		setCurrentText(tz, languages[language].defaulttz);
	}
	static_cast<Installer*>(parent())->paintEvent(0); // Update language even on the background
	resizeEvent(0);
}

void L10N::resizeEvent(QResizeEvent *e)
{
	if(e)
		QFrame::resizeEvent(e);
	info->setText(tr("Please choose your language:"));
	langLbl->setText(tr("&Language:"));
	info2->setWordWrap(true);
	info2->setText(tr("Please choose the keyboard type and timezone of your location:"));
	kbdLbl->setText(tr("&Keyboard:"));
	tzLbl->setText(tr("&Timezone:"));
	help->setWordWrap(true);
	help->setText(tr("When the 3 settings above are ok, please select the \"Continue\" button in the lower right corner.<br><br>There are 3 ways to select a button:<br><ul><li>\"clicking\": move the mouse until the cursor (arrow) is on the button you'd like to choose, and press the left mouse button.</li><li>\"hotkeys\": You will notice an underlined letter in most buttons. You can activate a button by holding down the \"Alt\" key and pressing the key matching the letter.</li><li>The focus - shown as a dotted line around a button - shows which button is currently active. Press the \"Tab\" key to move the focus to a different button. Press the \"Space\" or \"Enter\" keys to choose the button that has the focus.</li></ul>"));
	ok->setText(tr("&Continue"));
	int lw=MAX3(langLbl->sizeHint().width(),
	            kbdLbl->sizeHint().width(),
		    tzLbl->sizeHint().width());
	int y=10;
	info->setGeometry(10, y, width()-20, info->sizeHint().height());
	y += info->height()+10;
	langLbl->setGeometry(10, y, lw, langLbl->sizeHint().height());
	lang->setGeometry(20+lw, y, width()-30-lw, lang->sizeHint().height());
	y += MAX(langLbl->height(), lang->height())+10;
	info2->setGeometry(10, y, width()-20, info2->sizeHint().height());
	y += info2->height()+10;
	kbdLbl->setGeometry(10, y, lw, kbdLbl->sizeHint().height());
	kbd->setGeometry(20+lw, y, width()-30-lw, kbd->sizeHint().height());
	y += MAX(kbdLbl->height(), kbd->height())+10;
	tzLbl->setGeometry(10, y, lw, tzLbl->sizeHint().height());
	tz->setGeometry(20+lw, y, width()-30-lw, tz->sizeHint().height());
	y += MAX(tzLbl->height(), tz->height())+10;
	help->setGeometry(10, y, width()-20, help->sizeHint().height());

	ok->setGeometry(width()-ok->sizeHint().width()-10, height()-ok->sizeHint().height()-10, ok->sizeHint().width(), ok->sizeHint().height());
}
