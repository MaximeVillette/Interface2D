#ifndef TWODV_H
#define TWODV_H

//Librairies externes

#include <QMainWindow>
#include <QtCore>
#include <QtGui>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QCanBusDevice>
#include <QCanBus>

//Macros pour le robot et la table

#define LONGUEUR_TABLE 3000
#define LARGEUR_TABLE 2000
#define LONGUEUR_ROBOT1 250
#define LARGEUR_ROBOT1 250

#define ORIGINE_X 0
#define ORIGINE_Y 0

#define OBSTACLE_1_X 895
#define OBSTACLE_1_Y 1850

#define OBSTACLE_2_X 1495
#define OBSTACLE_2_Y 1700

#define OBSTACLE_3_X 2095
#define OBSTACLE_3_Y 1850

class MainWindow; //Définit la "class" en fenêtre principale

QT_BEGIN_NAMESPACE
namespace Ui { class TwoDV; }
QT_END_NAMESPACE

class TwoDV : public QMainWindow //Déclare la "class"
{
    Q_OBJECT

public:
    TwoDV(MainWindow *bus, QWidget *parent = nullptr); //Déclare la fonction TwoDV
    ~TwoDV(); //je sais pas trop

    void setCanPosRobot(const QCanBusFrame &fram); //Déclare la fonction ...

private slots:

    //Tous les prototypes de fonctions
    void TableLimit(double Px,double Py);
    void ReadPosition();
    void SelectionQuadrantSensP(double teta);
    void SelectionQuadrantSensN(double teta);
    void FrameControl(uint ID, int16_t DATA);
    void SetView();
    void on_AC_ouvrirperipherique_triggered();
    void on_AC_enregistrer_triggered();
    QString GetText(int index);
    void on_AC_ouvrir_triggered();
    void on_AC_temspreel_triggered();
    void on_AC_developpement_triggered();
    void on_AC_fermer_triggered();
    void on_AC_quitter_triggered();
    void on_AC_reglagesTwoDV_triggered();
    void on_AC_reglagesCAN_triggered();
    int16_t SelectTouche(QString text);
    void on_PB_ok_REG_clicked();
    void on_PB_ouvrir_REG_clicked();
    void on_CB_mode1_REG_stateChanged();
    void on_CB_mode2_REG_stateChanged();

private:
    Ui::TwoDV *ui; //Créer l'ui dans TwoDV
    MainWindow *busCan; //...

    //Création des différents items à intégrer dans la fenêtre 'graphicsView'
    QGraphicsScene *scene;
    QGraphicsPixmapItem *image;
    QGraphicsPixmapItem *robot1;
    QGraphicsLineItem *lineUp;
    QGraphicsLineItem *lineDown;
    QGraphicsLineItem *lineRight;
    QGraphicsLineItem *lineLeft;
    QGraphicsLineItem *lineMap1;
    QGraphicsLineItem *lineMap2;
    QGraphicsLineItem *lineMap3;
    std::unique_ptr<QCanBusDevice> can_device;

    //Création de l'interface can;
	QList<QCanBusDeviceInfo> interfaces;

    //Création du prototype de fonction : évènement de touche enfoncée
    void keyPressEvent(QKeyEvent *keyevent);

protected:
    virtual void resizeEvent(QResizeEvent *event);
};
#endif // TWODV_H
