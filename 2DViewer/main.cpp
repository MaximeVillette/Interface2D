#include "twodv.h"
#include <QtCore>
#include <QtGui>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TwoDV w;
    w.show();
    return a.exec();
}
