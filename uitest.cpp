#include "expertpartitioner.h"
#include <qapplication.h>

int main(int argc, char **argv) {
	QApplication app(argc, argv);
	ExpertPartitioner ep(0);
	app.setMainWidget(&ep);
	ep.show();
	app.exec();
}
