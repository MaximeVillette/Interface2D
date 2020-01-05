#ifndef TWODV_H
#define TWODV_H

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QGraphicsItemAnimation>
#include <QMouseEvent>
#include <QKeyEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class TwoDV; }
QT_END_NAMESPACE

class TwoDV : public QMainWindow
{
    Q_OBJECT

public:
    TwoDV(QWidget *parent = nullptr);
    ~TwoDV();

private slots:

    void PositionRobot(double X1,double Y1);
    void TableLimit(double Px,double Py);
    void ReadPosition();
    void SelectionQuadrantSensP(int teta);
    void SelectionQuadrantSensM(int teta);

private:
    Ui::TwoDV *ui;

    //Création des différents items à intégrer dans la fenêtre 'graphicsView'

    QGraphicsScene *scene;
    QGraphicsPixmapItem *image;
    QGraphicsPixmapItem *robot1;

    //Création du prototype de fonction : évènement de touche enfoncée

    void keyPressEvent(QKeyEvent *keyevent);
};
#endif // TWODV_H
