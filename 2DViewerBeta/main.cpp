#include "twodv.h"
#include "mainwindow.h"

#include <QtCore>
#include <QtGui>
#include <QApplication>
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.canbus* = true"));
    MainWindow x;
    x.show();
    return a.exec();
}
