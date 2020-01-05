#ifndef WIDGETCREATOR_H
#define WIDGETCREATOR_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QGraphicsItemAnimation>
#include <QMouseEvent>

QT_BEGIN_NAMESPACE
namespace Ui2 { class WCreator; }
QT_END_NAMESPACE

class WCreator : public QMainWindow
{
    Q_OBJECT

public:
    WCreator(QWidget *parent = nullptr);
    ~WCreator();

private slots:

private:
    Ui::WCreator *ui2;

};

#endif // WIDGETCREATOR_H
