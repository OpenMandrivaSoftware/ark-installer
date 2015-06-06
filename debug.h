#ifndef _DEBUG_H_
#define _DEBUG_H_ 1
//#define DEBUG_VERBOSE 1

#ifdef DEBUG_VERBOSE
#include <qmessagebox.h>
#define MSG(x) QMessageBox::information(0, "DEBUG", x)
#else
#define MSG(x)
#endif
#endif
