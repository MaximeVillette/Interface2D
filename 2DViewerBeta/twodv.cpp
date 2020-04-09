//Librairies du projet

#include "mainwindow.h"
#include "sendframebox.h"
#include "connectdialog.h"
#include "twodv.h"

//Librairie du design de la fenêtre
#include "ui_twodv.h"

//Librairies externes
#include <QtCore>
#include <QtGui>
#include <QPixmap>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QDebug>
#include <QCanBus>
#include <QCanBusDevice>
#include <QFileDialog>


//-----Déclaration des variables-----//

//Position sur le terrain virtuel
static double PosX=0, PosY=0, Rotate=0;

//Position vue par le bus CAN;
static double px, py, theta;

//Nombre de données à enregistrer
static int NbVariable=0;

//Consignes du tableau de commandes
static int16_t Distance=100, Rotation=15;
static double Vitesse=1.0;

//Mode de contrôle du robot (sur le logiciel)
static bool Mode=1;

//Tableau des réglages (barre d'outils)
static QString Reglages[13]={"0"};

//Autres variables
static int i=0;


//-----Définition de la "class" TwoDV-----//

TwoDV::TwoDV(MainWindow *bus, QWidget *parent): QMainWindow(parent), ui(new Ui::TwoDV), busCan(bus)
{
    ui->setupUi(this); //L'user Interface démarre

    //Création de la scene où l'on aura la table et le robot
    scene = new QGraphicsScene(this);
    ui->GV_control->setScene(scene);

    //Intégration des images pour la scene
    QPixmap tapis(":/Table/Images/SailTheWorld/TableSailTheWorld.png");
    image = scene->addPixmap(tapis);
    image->setOffset(-202,-481);
    QPixmap robot(":/Robot/Images/SailTheWorld/Robot.png");
    robot1 = scene->addPixmap(robot);
    robot1->setOffset(-robot1->boundingRect().center().x(),-robot1->boundingRect().center().y());
    robot1->setFlag(QGraphicsItem::ItemIsMovable); //Rendre le robot amovible avec la souris
    ui->TW_Commandes->currentIndex();
    robot1->setPos(0,0); //Le robot est positionné
    robot1->setRotation(0);
    QString TextPosition = tr("%0 X ; %1 Y ; %2 R").arg(-220).arg(-160).arg(0);
    ui->LposXYR1->setText(TextPosition);
    //Création des bordures virtuelles
    QPen redline(Qt::red);
    redline.setStyle(Qt::SolidLine);
    redline.setWidth(25);
    redline.setBrush(Qt::red);
    lineUp = scene->addLine(ORIGINE_X,ORIGINE_Y,LONGUEUR_TABLE,ORIGINE_Y,redline);
    lineDown = scene->addLine(ORIGINE_X,LARGEUR_TABLE,LONGUEUR_TABLE,LARGEUR_TABLE,redline);
    lineRight = scene->addLine(LONGUEUR_TABLE,ORIGINE_Y,LONGUEUR_TABLE,LARGEUR_TABLE,redline);
    lineLeft = scene->addLine(ORIGINE_X,ORIGINE_Y,ORIGINE_X,LARGEUR_TABLE,redline);
    lineMap1 = scene->addLine(OBSTACLE_1_X,OBSTACLE_1_Y,OBSTACLE_1_X,LARGEUR_TABLE,redline);
    lineMap2 = scene->addLine(OBSTACLE_2_X,OBSTACLE_2_Y,OBSTACLE_2_X,LARGEUR_TABLE,redline);
    lineMap3 = scene->addLine(OBSTACLE_3_X,OBSTACLE_3_Y,OBSTACLE_3_X,LARGEUR_TABLE,redline);
    lineUp->hide();
    lineDown->hide();
    lineRight->hide();
    lineLeft->hide();
    lineMap1->hide();
    lineMap2->hide();
    lineMap3->hide();
    QTimer::singleShot(10, this, SLOT(SetView()));
    ui->SW_general->setCurrentIndex(0);
    ui->L_probleme_REG->hide();
    if(ui->CB_chargement_REG->isChecked())
    {
        on_PB_ouvrir_REG_clicked();
    }
}

//Si la taille de la fenêtre est modifiée
void TwoDV::resizeEvent(QResizeEvent* event)
{
   QMainWindow::resizeEvent(event); //Détection de l'évènement
   SetView(); //Actualise la vue (taille des images dans le "graphics view")
}

//Fonction d'actualisation du "graphics view"
void TwoDV::SetView()
{
    //Le "graphics view" est un rectangle qui garde un aspect proportionnel
    ui->GV_control->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    //Affiche dans le débogeur la position et la taille du rectangle
    qDebug() << scene->sceneRect();
}

//Fin de l'User Interface
TwoDV::~TwoDV()
{
    delete ui; //Supprime l'ui
}

//Si il y a un évènement clavier
void TwoDV::keyPressEvent(QKeyEvent *keyevent) //Lecture des touches du clavier
{

//-----Commandes de déplacements-----//

    ReadPosition(); //Lit la position du robot
    //Transformation du type double (DSB) en int16_t pour le bus CAN
    Distance=static_cast<int16_t>(ui->DSBDistance->value());
    Rotation=static_cast<int16_t>(ui->DSBRotation->value());
    if(Mode==0) //Si le mode de contrôle est à0, alors (mode temps réel)
    {
        if(keyevent->key() == SelectTouche(ui->LE_avant_REG->text()))
           FrameControl(0x24,Distance); //Aller vers l'avant
        if(keyevent->key() == SelectTouche(ui->LE_arriere_REG->text()))
           FrameControl(0x24,Distance*-1); //Aller vers l'arrière
        if(keyevent->key() == SelectTouche(ui->LE_droite_REG->text()))
           FrameControl(0x23,Rotation*-10); //Aller vers la droite
        if(keyevent->key() == SelectTouche(ui->LE_gauche_REG->text()))
           FrameControl(0x23,Rotation*10); //Aller vers la gauche
    }
    else //Sinon, le mode est à 1 (mode de développement)
    {
        /* Notes :
         *  La fonction "TableLimit" : Vérifie la position du robot par rapport
         *  aux limites de la table
         *  Les fonctions "SelectionQuadrant SensP/SensN" : Calcul les coordonées
         *  d'arriver du robot en fonction de son angle
         * */
        Vitesse=ui->DSBVitesse->value(); //Récupère la consigne de vitesse (virtuel)
        if(keyevent->key() == SelectTouche(ui->LE_avant_REG->text())) //Allez vers l'avant
        {
            SelectionQuadrantSensP(Rotate);
            TableLimit(PosX,PosY);
        }
        if(keyevent->key() == SelectTouche(ui->LE_arriere_REG->text())) //Allez vers l'arrière
        {
            SelectionQuadrantSensN(Rotate);
            TableLimit(PosX,PosY);
        }
        if(keyevent->key() == SelectTouche(ui->LE_droite_REG->text())) //Allez vers la droite
        {
            Rotate+=Vitesse; //Incrémentation de l'angle
            //Si l'angle est supérieur à 360°, il faut repartir à 0°
            if(Rotate>=359) Rotate=0;
        }
        if(keyevent->key() == SelectTouche(ui->LE_gauche_REG->text())) //Aller vers la gauche
        {
            Rotate-=Vitesse; //Décrémentation de l'angle
            //Si l'angle est inférieur à 0°, il faut repartir à 359°
            if(Rotate<0) Rotate=359;
        }
        /* Notes :
         * Le robot ne peut pas être à 360° et à 0° en même temps, il faut donc
         * choisir l'un des 2
         * Ici on prend 0° comme référence
        */
    }

//-----Commandes de placements-----//

    //Si il y a un appui simultané de "Ctrl" "I"
    if((keyevent->key()== Qt::Key_I)&&(keyevent->modifiers()==Qt::ControlModifier)) //Si les touches Ctrl et I sont enfoncées
    {
        //Position par défaut demandée
        PosX=ui->DSBx1->value();
        PosY=ui->DSBy1->value();
        Rotate=ui->DSBr1->value();
    }
    //Si il y a un appui simultané de "Ctrl" "P"
    if((keyevent->key()== Qt::Key_P)&&(keyevent->modifiers()==Qt::ControlModifier)) //Si les touches Ctrl et I sont enfoncées
    {
        //Nouvelle position par défaut demandée
        ui->DSBx1->setValue(PosX);
        ui->DSBy1->setValue(PosY);
        ui->DSBr1->setValue(Rotate);
    }


//-----Actions générales-----//

    //Le robot est positionné
    robot1->setPos(PosX,PosY);
    robot1->setRotation(Rotate);
    //Affiche les coordonnées
    ui->LabelPosX->setNum(PosX);
    ui->LabelPosY->setNum(PosY);
    ui->label->setNum(Rotate);
    //Affiche sur un label la position par défault
    ui->LposXYR1->setText(GetText(2));
}

//Fonction pour vérifier que le robot est toujours sur la table
void TwoDV::TableLimit(double Px,double Py)
{
    //Coté gauche
    (Px<=ORIGINE_X-(LONGUEUR_ROBOT1/2)) ? lineLeft->show() : lineLeft->hide();

    //Coté droit
    (Px>=ORIGINE_X+LONGUEUR_TABLE+(LONGUEUR_ROBOT1/2)) ? lineRight->show() : lineRight->hide();

    //En bas
    (Py<=ORIGINE_Y+(LARGEUR_ROBOT1/2)) ? lineDown->show() : lineDown->hide();

    //En haut
    (Py>=ORIGINE_Y+LARGEUR_TABLE+(LARGEUR_ROBOT1/2)) ? lineUp->show() : lineUp->hide();

    //Obstacle 1
    (((Px>=OBSTACLE_1_X-(LONGUEUR_ROBOT1/2))&&(Px<=OBSTACLE_1_X+(LONGUEUR_ROBOT1/2)))&&(Py>=OBSTACLE_1_Y-(LARGEUR_ROBOT1/2))) ?
        lineMap1->show() : lineMap1->hide();

    //Obstacle 2
    (((Px>=OBSTACLE_2_X-(LONGUEUR_ROBOT1/2))&&(Px<=OBSTACLE_2_X+(LONGUEUR_ROBOT1/2)))&&(Py>=OBSTACLE_2_Y-(LARGEUR_ROBOT1/2))) ?
        lineMap2->show() : lineMap2->hide();

    //Obstacle 3
    (((Px>=OBSTACLE_3_X-(LONGUEUR_ROBOT1/2))&&(Px<=OBSTACLE_3_X+(LONGUEUR_ROBOT1/2)))&&(Py>=OBSTACLE_3_Y-(LARGEUR_ROBOT1/2))) ?
        lineMap3->show() : lineMap3->hide();
}

//Fonction qui lit la position du robot
void TwoDV::ReadPosition()
{
    PosX=robot1->pos().rx();
    PosY=robot1->pos().ry();
    Rotate=robot1->rotation();
}

// Les fonctions SelectionQuadrant SensN / Sens P

/*La différence entre le sens positif et le sens négatif est la façon de calculer l'ancienne position
* Exemple si le robot doit aller en avant : PosY+=(0.0111*Rotate)-1;
*                                           PosX+=(-0.0111*Rotate)+2;
*
* Exemple si le robot doit aller en arrière : PosY-=(0.0111*Rotate)-1;
*                                             PosX-=(-0.0111*Rotate)+2;
*
*  C'est donc la seule différence entre les deux fonctions, les calculs de trajectoire restent les mêmes
*
* La façon d'utiliser les signes + ou - devant le = dépend du quadrant
*/

//Calcul de la trajectoire dans le sens NEGATIF
void TwoDV::SelectionQuadrantSensN(double teta)
{
    Vitesse=ui->DSBVitesse->value(); //Récupère la vitesse (virtuel)
    if((teta>=0)&&(teta<=90)) //Si l'angle est dans le quadrant 1
    {
        PosY-=(-0.0111*Vitesse*Rotate)+(1*Vitesse);
        PosX+=(0.0111*Vitesse*Rotate);
    }
    if((teta>90)&&(teta<=180)) //Si l'angle est dans le quadrant 2
    {
        PosY+=(0.0111*Vitesse*Rotate)-(1*Vitesse);
        PosX+=(-0.0111*Vitesse*Rotate)+(2*Vitesse);
    }
    if((teta>180)&&(teta<=270)) //Si l'angle est dans ce quadrant 3
    {
        PosY+=(-0.0111*Vitesse*Rotate)+(3*Vitesse);
        PosX-=(0.0111*Vitesse*Rotate)-(2*Vitesse);
    }
    if((teta>270)&&(teta<360)) //Si l'angle est dans ce quadrant 4
    {
        PosY-=(0.0111*Vitesse*Rotate)-(3*Vitesse);
        PosX-=(-0.0111*Vitesse*Rotate)+(4*Vitesse);
    }
}

//Calcul de la trajectoire dans le sens POSITIF
void TwoDV::SelectionQuadrantSensP(double teta)
{
    Vitesse=ui->DSBVitesse->value(); //Récupère la vitesse (virtuel)
    if((teta>=0)&&(teta<=90)) //Si l'angle est dans le quadrant 1
    {
        PosY+=(-0.0111*Vitesse*Rotate)+(1*Vitesse);
        PosX-=(0.0111*Vitesse*Rotate);
    }
    if((teta>90)&&(teta<=180)) //Si l'angle est dans le quadrant 2
    {
        PosY-=(0.0111*Vitesse*Rotate)-(1*Vitesse);
        PosX-=(-0.0111*Vitesse*Rotate)+(2*Vitesse);
    }
    if((teta>180)&&(teta<=270)) //Si l'angle est dans ce quadrant 3
    {
        PosY-=(-0.0111*Vitesse*Rotate)+(3*Vitesse);
        PosX+=(0.0111*Vitesse*Rotate)-(2*Vitesse);
    }
    if((teta>270)&&(teta<360)) //Si l'angle est dans ce quadrant 4
    {
        PosY+=(0.0111*Vitesse*Rotate)-(3*Vitesse);
        PosX+=(-0.0111*Vitesse*Rotate)+(4*Vitesse);
    }
}

//Envoi d'une consigne avec l'ID et les données
void TwoDV::FrameControl(uint ID, int16_t DATA)
{
    QByteArray data(8, 0); //Donnée sur 8 octets
    //Transformation de la donnée de int16_t à char
    data[0] = static_cast<char>(DATA & 0xFF);
    data[1] = static_cast<char>((DATA >> 8) & 0xFF);
    QCanBusFrame frame(ID, data); //Définit une trame avec identifiant et donnée
    busCan->sendFrame(frame); //Envoi la trame
}

//Décomposer la trame pour placer le robot
void TwoDV::setCanPosRobot(const QCanBusFrame &fram)
{
    //Utilisation du poids fort/faible de la donnée (payload)

    uint16_t x_faible = static_cast<uint8_t>(fram.payload()[0]);
    uint16_t x_fort = static_cast<uint8_t>(fram.payload()[1]) << 8u;
    px = static_cast<int16_t>(x_faible | x_fort); //Retourne la position X

    uint16_t y_faible = static_cast<uint8_t>(fram.payload()[2]);
    uint16_t y_fort = static_cast<uint8_t>(fram.payload()[3]) << 8u;
    py = static_cast<int16_t>(y_faible | y_fort); //Retourne la position Y

    uint16_t th_faible = static_cast<uint8_t>(fram.payload()[4]);
    uint16_t th_fort = static_cast<uint8_t>(fram.payload()[5]) << 8u;
    theta = static_cast<int16_t>(th_faible | th_fort);
    theta = theta * 0.1; //Retourne l'angle theta

    //Positionne le robot (250 et 1750 sont des offsets pour régler le placement)
    robot1->setPos(px+250, 1750-py);
    robot1->setRotation(-theta);
    //Affiche la position dans le débogeur
    qDebug() << GetText(1);
}

//Mettre en avant ou ouvrir la fenêtre InterCAN
void TwoDV::on_AC_ouvrirperipherique_triggered()
{
    busCan->show(); //Montre toi
    busCan->activateWindow(); //Active la fenêtre et la met au premier plan
}

//Enregistrer des données de déplacements
void TwoDV::on_AC_enregistrer_triggered()
{
    //Ouvre le dossier de l'ordinateur
    QString FileInfos = QFileDialog::getSaveFileName(this,("Enregistrement des données"),tr(""),tr("Données(*.txt)"));
    if(FileInfos.isEmpty()) return; //Si le fichier est vide, il n'y a rien à enregistrer
    else
    {
        QFile NfilePosition(FileInfos); //Range le fichier QString dans un QFile
        //Si le QFile ne s'ouvre pas
        if(!NfilePosition.open(QFile::WriteOnly|QFile::Text))
        {
            //Message d'alerte
            QMessageBox::warning(this,"Attention","Impossible d'enregistrer les données");
            return; //Sort de la fonction
        }
        //Fait sortir le fichier au format ".txt"
        QTextStream out(&NfilePosition);
        //encore des choses à mettre
        out << GetText(0); //Enregistre ce texte
     }
}

//Fonction qui créer un fichier texte en fonction du besoin
QString TwoDV::GetText(int index)
{
    QString Text;
    int i;
    if(index==0) //Texte pour enregistrer la position en sortie du bus CAN
    {
        NbVariable=ui->SBDonnees->value();
        for(i=0;i<NbVariable;i++)
            Text = (QString("X : %1, Y : %2, Theta : %3 \n").arg(px).arg(py).arg(theta));
    }
    else if(index==1) //Texte de la position en sortie du bus CAN
        Text = (QString("%0 X ; %1 Y ; %2 R").arg(px).arg(py).arg(theta));
    else if(index==2) //Texte de la position par défaut
        Text = (QString("%0 X ; %1 Y ; %2 R").arg(ui->DSBx1->value()).arg(ui->DSBy1->value()).arg(ui->DSBr1->value()));
    else if(index==3) //Texte pour enregitrer les réglages
    {
        Text = (QString("%0\n%1\n%2\n%3\n%4\n%5\n%6\n%7\n%8\n%9\n%10\n%11\n%12").arg(Reglages[0]).arg(Reglages[1])
                .arg(Reglages[2]).arg(Reglages[3]).arg(Reglages[4]).arg(Reglages[5]).arg(Reglages[6])
                .arg(Reglages[7]).arg(Reglages[8]).arg(Reglages[9]).arg(Reglages[10]).arg(Reglages[11])
                .arg(Reglages[12]));
    }
    return Text; //Retourne le texte
}

//Ouvrir des données de déplacements
void TwoDV::on_AC_ouvrir_triggered()
{
    QString FileDonnees = QFileDialog::getOpenFileName(this,"Chargement des données");
    QFile NfileDon(FileDonnees);
    //Si le fichier n'est pas lisible
    if(!NfileDon.open(QFile::ReadOnly|QFile::Text))
    {
        QMessageBox::warning(this,"Attention","Impossible de charger les données");
        return;
    }
    QTextStream in(&NfileDon); //Fait rentrer le fichier au format ".txt"
    QString Resum = in.readAll(); //Lit tout le fichier
    //décomposition
    NfileDon.close(); //Ferme le fichier
}

//Mettre le mode temps réel
void TwoDV::on_AC_temspreel_triggered() { Mode = 0; }

//Mettre le mode développement
void TwoDV::on_AC_developpement_triggered() { Mode = 1; }

//Fermer InterCAN
void TwoDV::on_AC_fermer_triggered() { busCan->close(); }

//Fermer le logiciel
void TwoDV::on_AC_quitter_triggered() { TwoDV::close(); busCan->close(); }

//Ouvrir la page de réglage TwoDV
void TwoDV::on_AC_reglagesTwoDV_triggered()
{
    ui->SW_general->setCurrentIndex(1); //Met la page 1
    ui->GB_reglagesTwoDV_REG->show(); //Montre toi
}

//Ouvrir la page de réglage InterCAN
void TwoDV::on_AC_reglagesCAN_triggered()
{
    ui->SW_general->setCurrentIndex(1); //Met la page 1
    ui->GB_reglagesTwoDV_REG->hide(); //Cache toi
}

//-----En travaux-----//

//Enregistre les réglages
void TwoDV::on_PB_ok_REG_clicked()
{
    ui->SW_general->setCurrentIndex(0);
    Reglages[0] = ui->CB_chargement_REG->isChecked() ? "1" : "0";
    Reglages[1] = QString("%0").arg(ui->DSB_x1_REG->value());
    Reglages[2] = QString("%0").arg(ui->DSB_y1_REG->value());
    Reglages[3] = QString("%0").arg(ui->DSB_r1_REG->value());
    Reglages[4] = QString("%0").arg(ui->DSB_distance_REG->value());
    Reglages[5] = QString("%0").arg(ui->DSB_rotation_REG->value());
    Reglages[6] = QString("%0").arg(ui->DSB_vitesse_REG->value());
    Reglages[7] = QString("%0").arg(ui->SB_donnees_REG->value());
    Reglages[8] = ui->CB_mode1_REG->isChecked() ? "1" : "0";
    Reglages[9] = ui->LE_avant_REG->text();
    Reglages[10] = ui->LE_arriere_REG->text();
    Reglages[11] = ui->LE_droite_REG->text();
    Reglages[12] = ui->LE_gauche_REG->text();

    QString FileReglages = QFileDialog::getSaveFileName(this,("Enregistrement des réglages"),tr(""),tr("Réglages(*.txt)"));
        if(FileReglages.isEmpty()) return;
        else
        {
            QFile Nfile(FileReglages);
            if(!Nfile.open(QFile::WriteOnly|QFile::Text))
            {
                QMessageBox::warning(this,"Attention","Impossible d'enregistrer les réglages");
                return;
            }
            QTextStream out(&Nfile);
            out << GetText(3);
        }
}

//Convertir un caractère en hexa
int16_t TwoDV::SelectTouche(QString text)
{
    int16_t hexa=0;
    if((text=="A")||(text=="a"))
    {
        hexa=0x41;
        ui->L_probleme_REG->hide();
    }
    else if((text=="B")||(text=="b"))
    {
        hexa=0x42;
        ui->L_probleme_REG->hide();
    }
    else if((text=="C")||(text=="c"))
    {
        hexa=0x43;
        ui->L_probleme_REG->hide();
    }
    else if((text=="D")||(text=="d"))
    {
        hexa=0x44;
        ui->L_probleme_REG->hide();
    }
    else if((text=="E")||(text=="e"))
    {
        hexa=0x45;
        ui->L_probleme_REG->hide();
    }
    else if((text=="F")||(text=="f"))
    {
        hexa=0x46;
        ui->L_probleme_REG->hide();
    }
    else if((text=="G")||(text=="g"))
    {
        hexa=0x47;
        ui->L_probleme_REG->hide();
    }
    else if((text=="H")||(text=="h"))
    {
        hexa=0x48;
        ui->L_probleme_REG->hide();
    }
    else if((text=="I")||(text=="i"))
    {
        hexa=0x49;
        ui->L_probleme_REG->hide();
    }
    else if((text=="J")||(text=="j"))
    {
        hexa=0x4a;
        ui->L_probleme_REG->hide();
    }
    else if((text=="K")||(text=="k"))
    {
        hexa=0x4b;
        ui->L_probleme_REG->hide();
    }
    else if((text=="L")||(text=="l"))
    {
        hexa=0x4c;
        ui->L_probleme_REG->hide();
    }
    else if((text=="M")||(text=="m"))
    {
        hexa=0x4d;
        ui->L_probleme_REG->hide();
    }
    else if((text=="N")||(text=="n"))
    {
        hexa=0x4e;
        ui->L_probleme_REG->hide();
    }
    else if((text=="O")||(text=="o"))
    {
        hexa=0x4f;
        ui->L_probleme_REG->hide();
    }
    else if((text=="P")||(text=="p"))
    {
        hexa=0x50;
        ui->L_probleme_REG->hide();
    }
    else if((text=="Q")||(text=="q"))
    {
        hexa=0x51;
        ui->L_probleme_REG->hide();
    }
    else if((text=="R")||(text=="r"))
    {
        hexa=0x52;
        ui->L_probleme_REG->hide();
    }
    else if((text=="S")||(text=="s"))
    {
        hexa=0x53;
        ui->L_probleme_REG->hide();
    }
    else if((text=="T")||(text=="t"))
    {
        hexa=0x54;
        ui->L_probleme_REG->hide();
    }
    else if((text=="U")||(text=="u"))
    {
        hexa=0x55;
        ui->L_probleme_REG->hide();
    }
    else if((text=="V")||(text=="v"))
    {
        hexa=0x56;
        ui->L_probleme_REG->hide();
    }
    else if((text=="W")||(text=="w"))
    {
        hexa=0x57;
        ui->L_probleme_REG->hide();
    }
    else if((text=="X")||(text=="x"))
    {
        hexa=0x58;
        ui->L_probleme_REG->hide();
    }
    else if((text=="Y")||(text=="y"))
    {
        hexa=0x59;
        ui->L_probleme_REG->hide();
    }
    else if((text=="Z")||(text=="z"))
    {
        hexa=0x5a;
        ui->L_probleme_REG->hide();
    }
    else if(text=="0")
    {
        hexa=0x30;
        ui->L_probleme_REG->hide();
    }
    else if(text=="1")
    {
        hexa=0x31;
        ui->L_probleme_REG->hide();
    }
    else if(text=="2")
    {
        hexa=0x32;
        ui->L_probleme_REG->hide();
    }
    else if(text=="3")
    {
        hexa=0x33;
        ui->L_probleme_REG->hide();
    }
    else if(text=="4")
    {
        hexa=0x34;
        ui->L_probleme_REG->hide();
    }
    else if(text=="5")
    {
        hexa=0x35;
        ui->L_probleme_REG->hide();
    }
    else if(text=="6")
    {
        hexa=0x36;
        ui->L_probleme_REG->hide();
    }
    else if(text=="7")
    {
        hexa=0x37;
        ui->L_probleme_REG->hide();
    }
    else if(text=="8")
    {
        hexa=0x38;
        ui->L_probleme_REG->hide();
    }
    else if(text=="9")
    {
        hexa=0x39;
        ui->L_probleme_REG->hide();
    }
    else
    {
        hexa=0;
        ui->L_probleme_REG->show();
    }
    return hexa;
}

//Ouvrir un réglage
void TwoDV::on_PB_ouvrir_REG_clicked()
{
    QString FileReglages = QFileDialog::getOpenFileName(this,"Chargement des données");
    QFile Nfile(FileReglages);
    if(!Nfile.open(QFile::ReadOnly|QFile::Text))
    {
        QMessageBox::warning(this,"Attention","Impossible de charger les données");
        return;
    }
    QTextStream in(&Nfile);
    FileReglages = in.readAll();
    in >> FileReglages;
    for(i=0;i<FileReglages.size();i++)
    {
        Reglages[i]=FileReglages.at(i);
        qDebug() << Reglages[i];
    }
    ui->CB_chargement_REG->setChecked(Reglages[0].toInt());
    ui->DSB_x1_REG->setValue(Reglages[1].toDouble());
    ui->DSB_y1_REG->setValue(Reglages[2].toDouble());
    ui->DSB_r1_REG->setValue(Reglages[3].toDouble());
    ui->DSB_distance_REG->setValue(Reglages[4].toDouble());
    ui->DSB_rotation_REG->setValue(Reglages[5].toDouble());
    ui->DSB_vitesse_REG->setValue(Reglages[6].toDouble());
    ui->SB_donnees_REG->setValue(Reglages[7].toInt());
    ui->CB_mode1_REG->setChecked(Reglages[8].toInt());
    ui->LE_avant_REG->setText(Reglages[9]);
    ui->LE_arriere_REG->setText(Reglages[10]);
    ui->LE_droite_REG->setText(Reglages[11]);
    ui->LE_gauche_REG->setText(Reglages[12]);
    Nfile.close();
}


void TwoDV::on_CB_mode1_REG_stateChanged()
{
    ui->CB_mode1_REG->isChecked() ? ui->CB_mode2_REG->setChecked(0) : ui->CB_mode2_REG->setChecked(1);
    Reglages[0]="0";
}

void TwoDV::on_CB_mode2_REG_stateChanged()
{
    ui->CB_mode2_REG->isChecked() ? ui->CB_mode1_REG->setChecked(0) : ui->CB_mode1_REG->setChecked(1);
    Reglages[0]="1";
}
