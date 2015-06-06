#ifndef _SYSTEMPARTITIONER_H_
#define _SYSTEMPARTITIONER_H_ 1
#include "partitioner.h"
class SystemPartitioner:public Partitioner
{
	Q_OBJECT
public:
	SystemPartitioner(QWidget *parent, Qt::WindowFlags f=0);
	void init();
protected slots:
	void resizeEvent(QResizeEvent *e);
};
#endif
