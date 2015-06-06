#ifndef _EXPERTPARTITIONER_H_
#define _EXPERTPARTITIONER_H_ 1
#include "partitioner.h"
#include "installmode.h"
#include <QEvent>
#include <QTableWidget>
#include <QComboBox>
#include <QVBoxLayout>

class ExpertPartitioner:public Partitioner
{
	Q_OBJECT
public:
	ExpertPartitioner(QWidget *parent, Qt::WindowFlags f=0);
	void init();
	void show();
protected slots:
	void qtParted();
	void proceed();
private:
	QVBoxLayout		_layout;
	QextRtPushButton	*_rerunQtParted;
	QLabel			*_mpLabel;
	QTableWidget		*_mountPoints;
	QWidget			*_grubLayout;
	QLabel			*_grubLbl;
	QComboBox		*_grub;
	QextRtPushButton	*_proceed;
	QMap<QString,QString>	_fs;
};
#endif
