#include "sceneactions.h"

#include "ui_sceneactions.h"

sceneActions::sceneActions(QWidget *parent) : QGroupBox(parent),
    actions_ui(new Ui::sceneActions)
{
    actions_ui->setupUi(this);

    sceneActionsView = new QGraphicsScene(this); //Nouvelle scène -> capteurs de couleurs
    actions_ui->GV_actions->setScene(sceneActionsView);

    QPixmap rob(":/Robot/Images/SailTheWorld/Robot.png"); //Chemin d'accès de l'image du robot
    QPixmap sfl(":/Robot/Images/SailTheWorld/AirPump.png");

    robot = sceneActionsView->addPixmap(rob);
    robot->setOffset(-robot->boundingRect().center().x(),-robot->boundingRect().center().y()); //Définit le centre de rotation du robot
    robot->setPos(115, 118);
    robot->setRotation(90);

    QPen blue(Qt::blue); //Création d'un pinceau couleur rouge
    blue.setStyle(Qt::SolidLine); //Ligne pleine
    blue.setWidth(2); //2 pixel de large

    infoActionTxt = sceneActionsView->addText("Avant >>>"); //Applique un texte
    infoActionTxt->setPos(240, -10); //Place la zone de texte en haut à droite du graphicsView

    for(int i=0; i<2; i++)
    {
        for(int j=0; j<3; j++)
        {
            i==0 ? colors[i][j] = sceneActionsView->addEllipse((i*180)+15, 25+(j*80), 30, 30, blue, Qt::transparent) :
                    colors[i][j] = sceneActionsView->addEllipse(i*180, 25+(j*80), 30, 30, blue, Qt::transparent);
            colors[i][j]->setFlags(QGraphicsItem::ItemIsSelectable);

            airPump[i][j] = sceneActionsView->addPixmap(sfl);
            airPump[i][j]->setOffset(-airPump[i][j]->boundingRect().center().x(),-airPump[i][j]->boundingRect().center().y());
            airPump[i][j]->setScale(0.2);
            if(i) airPump[i][j]->setRotation(180);
            i==0 ? airPump[i][j]->setPos((i*280)-30, 35+(j*80)) : airPump[i][j]->setPos((i*250)+10, 35+(j*80));
            airPump[i][j]->setFlags(QGraphicsPixmapItem::ItemIsSelectable);
        }
    }

    QStringList titres;
    titres << "Nom" << "CAN ID" << "Valeur" << "Description";
    actions_ui->TW_variables->setColumnCount(4);
    actions_ui->TW_variables->setHorizontalHeaderLabels(titres);

    //Exemples
    addVariables("Position X", "", "", "");
    addVariables("Position Y", "", "", "");
    addVariables("Angle", "", "", "");
    addVariables("Servomoteurs", "", "", "");
    addVariables("Ventouses", "", "", "");
    addVariables("Capteurs de couleurs", "", "", "");

    QTimer::singleShot(10, this, &sceneActions::SetView); //Timer de 10ms pour mettre la vue à l'échelle
}

sceneActions::~sceneActions()
{
    delete actions_ui;
}

void sceneActions::SetView()
{
    actions_ui->GV_actions->fitInView(sceneActionsView->sceneRect(), Qt::KeepAspectRatio); //Le graphicsView est un rectangle qui garde un aspect proportionnel
    qDebug() << sceneActionsView->sceneRect(); //Affiche dans le débogeur la position et la taille du rectangle
}

void sceneActions::addVariables(QString nom, QString canID, QString valeur, QString infos)
{
    actions_ui->TW_variables->insertRow(actions_ui->TW_variables->rowCount());
    actions_ui->TW_variables->setItem(actions_ui->TW_variables->rowCount()-1, NOM, new QTableWidgetItem(nom));
    actions_ui->TW_variables->setItem(actions_ui->TW_variables->rowCount()-1, CAN_ID, new QTableWidgetItem(canID));
    actions_ui->TW_variables->setItem(actions_ui->TW_variables->rowCount()-1, VALEUR, new QTableWidgetItem(valeur));
    actions_ui->TW_variables->setItem(actions_ui->TW_variables->rowCount()-1, INFOS, new QTableWidgetItem(infos));
}
void sceneActions::setVariables(int composent, QString nom, QString canID, QString valeur, QString infos)
{
    actions_ui->TW_variables->setItem(composent, NOM, new QTableWidgetItem(nom));
    actions_ui->TW_variables->setItem(composent, CAN_ID, new QTableWidgetItem(canID));
    actions_ui->TW_variables->setItem(composent, VALEUR, new QTableWidgetItem(valeur));
    actions_ui->TW_variables->setItem(composent, INFOS, new QTableWidgetItem(infos));
}
