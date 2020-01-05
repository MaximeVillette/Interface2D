#include "twodv.h"
#include "ui_twodv.h"
#include <QtCore>
#include <QtGui>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QKeyEvent>

TwoDV::TwoDV(QWidget *parent): QMainWindow(parent), ui(new Ui::TwoDV)
{
    ui->setupUi(this);

    //Création de la scene qui où l'on retrouvera la table et le robot

    scene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);

    //Intégration des images pour la scene

    QPixmap tapis(":/Fond/tapis/Table.PNG");
    image = scene->addPixmap(tapis);

    QPixmap robot(":/Fond/tapis/Robot.png");
    robot1 = scene->addPixmap(robot);

    robot1->setScale(0.118); //Réduction de la taille de l'image pour qu'elle soit proportionnelle à celle de la table
    robot1->setFlag(QGraphicsItem::ItemIsMovable); //Rendre le robot amovible avec la souris
    robot1->setTransformOriginPoint(305,305); //Point d'origine au centre de l'image pour faire tourner le robot
    robot1->setPos(-220,-160); //Position par défault dans le coin supérieur gauche
    robot1->setRotation(0); //Angle par défault du robot
}

TwoDV::~TwoDV()
{
    delete ui;
}

static double PosX=0; //Position X du robot
static double PosY=0; //Position Y du robot
static int Rotate=0; //Angle du robot

void TwoDV::keyPressEvent(QKeyEvent *keyevent) //Lecture des touches du clavier
{
    ReadPosition(); //Lit la position du robot
    if(keyevent->key() == Qt::Key_Z) //Aller vers l'avant si la touche Z est enfoncée
    {
        SelectionQuadrantSensP(Rotate); //Calcul des coordonées d'arriver pour une translation dans le sens positif
        TableLimit(PosX,PosY); //Vérification de la position du robot
        PositionRobot(PosX,PosY); //Le robot est positionné
    }
    if(keyevent->key() == Qt::Key_S) //Aller vers l'arrière si la touche S est enfoncée
    {
        SelectionQuadrantSensM(Rotate); //Calcul des coordonées d'arriver pour une translation dans le sens négatif
        TableLimit(PosX,PosY);
        PositionRobot(PosX,PosY);
    }
    if(keyevent->key() == Qt::Key_D) //Aller vers la droite si la touche D est enfoncée
    {
        Rotate++; //Incrémentation de l'angle
        if(Rotate>360)
        {
            Rotate=0; //Si l'angle est supérieur à 360°, il faut repartir à 0°
        }
        robot1->setRotation(Rotate); //Le robot tourne avec l'angle demandé
    }
    if(keyevent->key() == Qt::Key_Q) //Aller vers la gauche si la touche Q est enfoncée
    {
        Rotate--; //Décrémentation de l'angle
        if(Rotate<0)
        {
            Rotate=360; //Si l'angle est inférieur à 0°, il faut repartir à 360°
        }
        robot1->setRotation(Rotate);
    }
    if((keyevent->key()== Qt::Key_I)&&(keyevent->modifiers()==Qt::ControlModifier)) //Si les touches Ctrl et I sont enfoncées
    {
        PosX=-220;
        PosY=-160;
        PositionRobot(PosX,PosY); //Position initiale
        Rotate=0;
        robot1->setRotation(Rotate); //Angle initiale
    }
    ui->LabelPosX->setNum(PosX); //Affiche la coordonnée Y
    ui->LabelPosY->setNum(PosY); //Affiche la coordonnée X
    ui->label->setNum(Rotate); //Affiche l'angle
}

void TwoDV::PositionRobot(double X,double Y) //Fonction pour placer le robot en fonction de X et Y
{
    robot1->setX(X); //Donne la valeur de X
    robot1->setY(Y); //Donne la valeur de Y
}

void TwoDV::TableLimit(double Px,double Py) //Fonction pour vérifier que le robot est toujours sur la table
{
    //Cette fonction peut encore être améliorée pour changer les positions par défaults
    if(!((Px>-220.0)&&(Px<390.0))) //Si les coordonnées Xmin et Xmax ne sont pas bonnes
    {
        PosX=-220.0; //Donne -220 par défault
    }
    if(!((Py>-160.0)&&(Py<230.0))) //Si les coordonnées Ymin et Ymax ne sont pas bonnes
    {
        PosY=-160.0; //Donne -160 par défault
    }
}

void TwoDV::ReadPosition() //Fonction qui lit la position du robot
{
    PosX=robot1->pos().rx(); //Change PosX
    PosY=robot1->pos().ry(); //Change PosY
}

// Les calculs des fonctions ci-dessous, sont issus du logiciel Regressi

void TwoDV::SelectionQuadrantSensM(int teta) //Calcul de la trajectoire dans le sens négatif
{
    /*La différence entre le sens positif et le sens négatif est la façon de calculer l'ancienne position
      Exemple si le robot doit aller en avant : PosY+=(0.0111*Rotate)-1;
                                                PosX+=(-0.0111*Rotate)+2;
      Exemple si le robot doit aller en arrière : PosY-=(0.0111*Rotate)-1;
                                                  PosX-=(-0.0111*Rotate)+2;
      C'est donc la seule différence entre les deux fonctions, les calculs de trajectoire restent les mêmes

      La façon d'utiliser les signes + ou - devant le = dépend du quadrant (voir figure 17 du rapport technique)
    */
    if((teta>=0)&&(teta<=90)) //Si l'angle est dans le quadrant 1
    {
        PosY-=(-0.0111*Rotate)+1;
        PosX+=(0.0111*Rotate);
    }
    if((teta>90)&&(teta<=180)) //Si l'angle est dans le quadrant 2
    {
        PosY+=(0.0111*Rotate)-1;
        PosX+=(-0.0111*Rotate)+2;
    }
    if((teta>180)&&(teta<=270)) //Si l'angle est dans ce quadrant 3
    {
        PosY+=(-0.0111*Rotate)+3;
        PosX-=(0.0111*Rotate)-2;
    }
    if((teta>270)&&(teta<360)) //Si l'angle est dans ce quadrant 4
    {
        PosY-=(0.0111*Rotate)-3;
        PosX-=(-0.0111*Rotate)+4;
    }
}

void TwoDV::SelectionQuadrantSensP(int teta) //Calcul de la trajectoire dans le sens positif
{
    if((teta>=0)&&(teta<=90)) //Si l'angle est dans le quadrant 1
    {
        PosY+=(-0.0111*Rotate)+1;
        PosX-=(0.0111*Rotate);
    }
    if((teta>90)&&(teta<=180)) //Si l'angle est dans le quadrant 2
    {
        PosY-=(0.0111*Rotate)-1;
        PosX-=(-0.0111*Rotate)+2;
    }
    if((teta>180)&&(teta<=270)) //Si l'angle est dans ce quadrant 3
    {
        PosY-=(-0.0111*Rotate)+3;
        PosX+=(0.0111*Rotate)-2;
    }
    if((teta>270)&&(teta<360)) //Si l'angle est dans ce quadrant 4
    {
        PosY+=(0.0111*Rotate)-3;
        PosX+=(-0.0111*Rotate)+4;
    }
}

