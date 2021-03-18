#include "fastdat.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	newFastData w;
	w.show();
	return a.exec();
}
