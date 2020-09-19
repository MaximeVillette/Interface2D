#include "twodv.h"
#include "mainwindow.h"
#include "sceneasservissement.h"
#include "sceneactions.h"

//Librairie du design de la fenêtre
#include "ui_twodv.h"

//-----Variables & Accès-----//

static string Reglages[SIZE_TAB_REG]={"0"}; //Tableau des réglages
string const paramPath("C:/TwoDV/defaultSett.txt"); //Chemin de stockage des paramètres

//-----Définition de la "class" TwoDV-----//

TwoDV::TwoDV(MainWindow *bus, QWidget *parent): QMainWindow(parent),
    ui(new Ui::TwoDV),
    busCan(bus),
    asservScene(new sceneAsservissement),
    actionsScene(new sceneActions)
{
    ui->setupUi(this);

    ui->SW_general->setCurrentIndex(0); //Affiche la première page au départ
    QTimer::singleShot(10, this, SLOT(setSettings())); //Ouvre et applique les derniers réglages

    //Actions barre d'outils :
    connect(ui->AC_temspReel, &QAction::triggered, [this]()
        {Mode = true, emit sendMode(Mode);}); //Activer le mode temps réel
    connect(ui->AC_developpement, &QAction::triggered, [this]()
        {Mode = false, emit sendMode(Mode);}); //Activer le mode développement

    connect(ui->AC_ouvrirPeripherique, &QAction::triggered, [this]()
    {
        busCan->show();
        busCan->activateWindow(); //Active la fenêtre et la met au premier plan
    }); //Ouvre l'interface de communication

    connect(ui->AC_reglages, &QAction::triggered, [this]()
        {ui->SW_general->setCurrentIndex(1);}); //Ouvre la page de réglages

    connect(ui->AC_fermer, &QAction::triggered, [this]()
        {busCan->close();}); //Ferme la communication CAN
    connect(ui->AC_quitter, &QAction::triggered, [this]()
    {
        TwoDV::close();
        busCan->close();
    }); //Ferme le logiciel

    connect(ui->CB_modeTR_REG, &QCheckBox::clicked, [this]()
    {
        ui->CB_modeTR_REG->isChecked() ? ui->CB_modeD_REG->setChecked(0) : ui->CB_modeD_REG->setChecked(1);
        Reglages[6]="0";
    }); //Active le mode temps réel
    connect(ui->CB_modeD_REG, &QCheckBox::clicked, [this]()
    {
        ui->CB_modeD_REG->isChecked() ? ui->CB_modeTR_REG->setChecked(0) : ui->CB_modeTR_REG->setChecked(1);
        Reglages[6]="1";
    }); //Active le mode développement

    //Retour page réglage
    connect(ui->PB_retour_REG, &QPushButton::clicked, [this]()
        {ui->SW_general->setCurrentIndex(0);});

    //Actions page principale
    connect(ui->CB_afficherGVasserv, &QCheckBox::clicked, [this]()
    {
        ui->CB_afficherGVasserv->isChecked() ? ui->GB_sceneAsserv->show() : ui->GB_sceneAsserv->hide();
        QTimer::singleShot(10, ui->GB_sceneAsserv, &sceneAsservissement::SetView);
        QTimer::singleShot(10, ui->GB_sceneActions, &sceneActions::SetView);
    }); //Afficher ou non l'asservissement
    connect(ui->CB_afficherGVactions, &QCheckBox::clicked, [this]()
    {
        ui->CB_afficherGVactions->isChecked() ? ui->GB_sceneActions->show() : ui->GB_sceneActions->hide();
        QTimer::singleShot(10, ui->GB_sceneAsserv, &sceneAsservissement::SetView);
        QTimer::singleShot(10, ui->GB_sceneActions, &sceneActions::SetView);
    }); //Afficher ou non les actions

    connect(ui->GB_commandes, &tabCommandes::sendRotation, ui->GB_sceneAsserv, &sceneAsservissement::getRotation);
    connect(ui->GB_commandes, &tabCommandes::sendDistance, ui->GB_sceneAsserv, &sceneAsservissement::getDistance);
    connect(ui->GB_commandes, &tabCommandes::sendVitesse, ui->GB_sceneAsserv, &sceneAsservissement::getVitesse);
    connect(ui->GB_commandes, &tabCommandes::sendNewPosX, ui->GB_sceneAsserv, &sceneAsservissement::getNewPosX);
    connect(ui->GB_commandes, &tabCommandes::sendNewPosY, ui->GB_sceneAsserv, &sceneAsservissement::getNewPosY);
    connect(ui->GB_commandes, &tabCommandes::sendNewAngle, ui->GB_sceneAsserv, &sceneAsservissement::getNewAngle);

    connect(ui->GB_sceneAsserv, &sceneAsservissement::sendInfosPosRobot, ui->GB_commandes, &tabCommandes::getInfosPosRobot);
    connect(ui->GB_sceneAsserv, &sceneAsservissement::sendNewDefaultPos, ui->GB_commandes, &tabCommandes::getNewDefaultPos);
    connect(ui->GB_commandes, &tabCommandes::sendFrameControl, this, &TwoDV::FrameControl);

    connect(this, &TwoDV::sendMode, ui->GB_commandes, &tabCommandes::getCommandAuthorize);
    connect(this, &TwoDV::sendMode, ui->GB_sceneAsserv, &sceneAsservissement::getMode);

    connect(this, &TwoDV::sendPosXReg, ui->GB_commandes, &tabCommandes::getPosXReg);
    connect(this, &TwoDV::sendPosYReg, ui->GB_commandes, &tabCommandes::getPosYReg);
    connect(this, &TwoDV::sendAngleReg, ui->GB_commandes, &tabCommandes::getAngleReg);
    connect(this, &TwoDV::sendDistanceReg, ui->GB_commandes, &tabCommandes::getDistanceReg);
    connect(this, &TwoDV::sendRotationReg, ui->GB_commandes, &tabCommandes::getRotationReg);
    connect(this, &TwoDV::sendVitesseReg, ui->GB_commandes, &tabCommandes::getVitesseReg);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(requestDataCAN()));
    timer->start(1000);
}

TwoDV::~TwoDV()
{
    delete ui;
}

void TwoDV::resizeEvent(QResizeEvent* event) //Adapte la taille de la fenêtre
{
   QMainWindow::resizeEvent(event); //Détecte de l'évènement
   QTimer::singleShot(10, ui->GB_sceneAsserv, &sceneAsservissement::SetView);
   QTimer::singleShot(10, ui->GB_sceneActions, &sceneActions::SetView);
}

void TwoDV::requestDataCAN()
{
    FrameControl(0x4B0, 0); //Capteurs couleurs
    FrameControl(0x27A, 0); //Ventouses
}

void TwoDV::FrameControl(uint ID, int16_t DATA) //Envoi d'une consigne avec l'ID et les données
{
    QByteArray data(8, 0); //Donnée sur 8 octets
    data[0] = static_cast<char>(DATA & 0xFF); //Converssion 'int16_t' vers 'char'
    data[1] = static_cast<char>((DATA >> 8) & 0xFF); //Converssion int16_t vers 'char' plus décalage de 8 bits
    QCanBusFrame frame(ID, data); //Définit une trame avec identifiant et donnée
    busCan->sendFrame(frame);
}

void TwoDV::setCanPosRobot(const QCanBusFrame &fram) //Décomposer la trame pour placer le robot
{
    double px, py, theta; //Position vue par le bus CAN
    const char * const idFormat = fram.hasExtendedFrameFormat() ? "%08X" : "%03X";
    uint fid = static_cast<uint>(fram.frameId());
    QString frID = QString::asprintf(idFormat, fid);

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
    theta *= 0.1; //Retourne l'angle theta

    actionsScene->setVariables(POSX, "Position X", frID, QString("%0").arg(px), "");
    actionsScene->setVariables(POSY, "Position Y", frID, QString("%0").arg(py), "");
    actionsScene->setVariables(ANG, "Angle", frID, QString("%0").arg(theta), "");
    asservScene->SetPosition(px+250, 1750-py, 90-theta); //Positionne le robot (250 et 1750 sont des offsets pour régler le placement) (mode temps réel uniquement)

}
void TwoDV::setCanServoRobot(const QCanBusFrame &fram) //Décomposer la trame pour les servomoteurs
{
    const char * const idFormat = fram.hasExtendedFrameFormat() ? "%08X" : "%03X";
    uint fid = static_cast<uint>(fram.frameId());
    QString frID = QString::asprintf(idFormat, fid);

    QString etat;
    if(fram.payload().toHex()=="9900") etat = "Bras sorties";
    else if(fram.payload().toHex()=="1601") etat = "Bras rentrées";
    else etat = "Aucun mouvements";
    actionsScene->setVariables(SERVO, "Servo 1", frID, fram.payload().toHex(), etat);
}
void TwoDV::setCanVentRobot(const QCanBusFrame &fram) //Décomposer la trame pour les ventouses
{
     qDebug() << "Ventouses :" << fram.payload().toHex();
     actionsScene->setVariables(VENT, "Vent 1", "frID", fram.payload().toHex(), "etat");
}
void TwoDV::setCanColorsRobot(const QCanBusFrame &fram) //Décomposer la trame pour les capteurs de couleurs
{
    qDebug() << "Capteurs coulerus :" << fram.payload().toHex();
    actionsScene->setVariables(COLSENS, "Color 1", "frID", fram.payload().toHex(), "etat");
}

bool TwoDV::activeMode()
{
    return Mode;
}

void TwoDV::on_PB_enregistrer_REG_clicked() //Enregistre les réglages
{
    ofstream flux(paramPath); //Créer un flux de sortie
    ui->SW_general->setCurrentIndex(0); //Remet la page initiale
    //Enregistre chaque réglage
    Reglages[0] = QString("%0").arg(ui->DSB_posX_REG->value()).toStdString();
    Reglages[1] = QString("%0").arg(ui->DSB_posY_REG->value()).toStdString();
    Reglages[2] = QString("%0").arg(ui->DSB_angle_REG->value()).toStdString();
    Reglages[3] = QString("%0").arg(ui->DSB_distance_REG->value()).toStdString();
    Reglages[4] = QString("%0").arg(ui->DSB_rotation_REG->value()).toStdString();
    Reglages[5] = QString("%0").arg(ui->DSB_vitesse_REG->value()).toStdString();
    Reglages[6] = ui->CB_modeTR_REG->isChecked() ? "1" : "0";
    if(flux) //Si le flux est bien présent
    {
        for(int i=0; i<SIZE_TAB_REG; i++) flux << Reglages[i] << endl; //Enregistre chaque réglage
        setSettings();//Applique les réglages
    }
    else QMessageBox::warning(this,"Erreur","Impossible d'enregistrer les réglages !");
}

void TwoDV::setSettings() //Ouvre les réglages
{
    ifstream flux(paramPath); //Créer un flux d'entrée
    if(flux) //Si le flux est bien présent
    {
        for(int i=0; i<SIZE_TAB_REG; i++) flux >> Reglages[i]; //Charge chaque réglage
        //Applique chaque réglage
        ui->DSB_posX_REG->setValue(QString::fromStdString(Reglages[0]).toDouble());
        ui->DSB_posY_REG->setValue(QString::fromStdString(Reglages[1]).toDouble());
        ui->DSB_angle_REG->setValue(QString::fromStdString(Reglages[2]).toDouble());
        ui->DSB_distance_REG->setValue(QString::fromStdString(Reglages[3]).toDouble());
        ui->DSB_rotation_REG->setValue(QString::fromStdString(Reglages[4]).toDouble());
        ui->DSB_vitesse_REG->setValue(QString::fromStdString(Reglages[5]).toDouble());
        if(Reglages[6] == "1") //Différencie le mode temps réeel et développement
        {
            ui->CB_modeTR_REG->setChecked(1);
            ui->CB_modeD_REG->setChecked(0);
            Mode = true;
        }
        else
        {
            ui->CB_modeTR_REG->setChecked(0);
            ui->CB_modeD_REG->setChecked(1);
            Mode = false;
        }
        emit sendMode(Mode);
        emit sendPosXReg(QString::fromStdString(Reglages[0]).toDouble());
        emit sendPosYReg(QString::fromStdString(Reglages[1]).toDouble());
        emit sendAngleReg(QString::fromStdString(Reglages[2]).toDouble());
        emit sendDistanceReg(QString::fromStdString(Reglages[3]).toDouble());
        emit sendRotationReg(QString::fromStdString(Reglages[4]).toDouble());
        emit sendVitesseReg(QString::fromStdString(Reglages[5]).toDouble());
        asservScene->SetPosition(QString::fromStdString(Reglages[0]).toDouble(),
                QString::fromStdString(Reglages[1]).toDouble(), QString::fromStdString(Reglages[2]).toDouble()); //Applique la position de départ
    }
    else QMessageBox::warning(this,"Erreur","Impossible d'ouvrir les réglages!");
}
