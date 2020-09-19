#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.canbus* = true"));
    MainWindow x;
    a.setStyle("fusion");
    x.show();
    return a.exec();
}
