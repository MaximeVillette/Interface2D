#include "mainwindow.h"
#include "sendframebox.h"
#include "connectdialog.h"
#include "twodv.h"

#include "ui_mainwindow.h"
#include "ui_sendframebox.h"
#include "ui_connectdialog.h"

#include <QCanBus>
#include <QCanBusFrame>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QTimer>

// Les "..." signifie qu'il manque des informations
// lignes
// autres

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::MainWindow), m_L_statusCANTimer(new QTimer(this))
{
    //Les fonctions ci-après sont exécutée au démarrage du logiciel

    m_ui->setupUi(this); //User Interface "m_ui" commmence ici
    m_connectDialog = new ConnectDialog; //Créer une nouvelle fenêtre de dialog (ici pour connecter un périphérique au busCAN)
    m_status = new QLabel; //Nouveau label ... autres
    m_ui->Barre_de_status->addPermanentWidget(m_status); //Ajout d'une fenêtre permanente dans la bar d'état du parent (ConnectDialog) ... autres
    m_written = new QLabel; //Nouveau label ... autres
    m_ui->Barre_de_status->addWidget(m_written); //Ajout d'une fenêtre dans la barre d'état du parent (ConnectDialog) ... autres
    initActionsConnections(); //Initialise les actions (voir ligne XX pour plus d'informations) ... lignes
    QTimer::singleShot(50, m_connectDialog, &ConnectDialog::show); //Timer d'un seul signal de 50ms qui montre le parent (ConnectDialog) ... autres
    twoDV = new TwoDV(this); //Nouvelle fenêtre
    twoDV->show(); //Montrer la fenêtre
}

MainWindow::~MainWindow() //Action sur MainWindow
{
    //Supprime les fonctions
    delete m_connectDialog;
    delete m_ui;
}

void MainWindow::initActionsConnections()
{
    m_ui->AC_deconnecter->setEnabled(false);
    m_ui->GB_messagesecrits->setEnabled(false);
    connect(m_ui->GB_messagesecrits, &SendFrameBox::sendFrame, this, &MainWindow::sendFrame);
    connect(m_ui->AC_connecter, &QAction::triggered, [this]()
    {
        m_canDevice.release()->deleteLater();
        m_connectDialog->show();
    });
    connect(m_connectDialog, &QWidget::activateWindow, this, &MainWindow::connectDevice);
    connect(m_ui->AC_deconnecter, &QAction::triggered, this, &MainWindow::disconnectDevice);
    connect(m_ui->AC_resetcontroller, &QAction::triggered, this, [this]()
    {
        m_canDevice->resetController();
    });
    connect(m_ui->AC_quitter, &QAction::triggered, this, &QWidget::close);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(m_ui->AC_nettoyer, &QAction::triggered, m_ui->TE_messagesrecus, &QTextEdit::clear);
    connect(m_ui->actionPluginDocumentation, &QAction::triggered, this, []()
    {
        QDesktopServices::openUrl(QUrl("http://doc.qt.io/qt-5/qtcanbus-backends.html#can-bus-plugins"));
    });
}

void MainWindow::processErrors(QCanBusDevice::CanBusError error) const
{
    switch (error)
    {
    case QCanBusDevice::ReadError:
    case QCanBusDevice::WriteError:
    case QCanBusDevice::ConnectionError:
    case QCanBusDevice::ConfigurationError:
    case QCanBusDevice::UnknownError:m_status->setText(m_canDevice->errorString());
        break;
    default:
        break;
    }
}

void MainWindow::connectDevice()
{
    const ConnectDialog::Settings p = m_connectDialog->settings();
    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice(p.pluginName, p.deviceInterfaceName,&errorString));
    if (!m_canDevice)
    {
        m_status->setText(tr("Error creating device '%1', reason: '%2'").arg(p.pluginName).arg(errorString));
        return;
    }
    m_numberFramesWritten = 0;
    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,this, &MainWindow::processErrors);
    connect(m_canDevice.get(), &QCanBusDevice::framesReceived,this, &MainWindow::processReceivedFrames);
    connect(m_canDevice.get(), &QCanBusDevice::framesWritten,this, &MainWindow::processFramesWritten);
    if (p.useConfigurationEnabled)
    {
        for (const ConnectDialog::ConfigurationItem &item : p.configurations)
            m_canDevice->setConfigurationParameter(item.first, item.second);
    }
    if (!m_canDevice->connectDevice())
    {
        m_status->setText(tr("Connection error: %1").arg(m_canDevice->errorString()));
        m_canDevice.reset();
    }
    else
    {
        m_ui->AC_connecter->setEnabled(false);
        m_ui->AC_deconnecter->setEnabled(true);
        m_ui->GB_messagesecrits->setEnabled(true);
        const QVariant bitRate = m_canDevice->configurationParameter(QCanBusDevice::BitRateKey);
        if (bitRate.isValid())
        {
            const bool isCanFd =m_canDevice->configurationParameter(QCanBusDevice::CanFdKey).toBool();
            const QVariant dataBitRate =m_canDevice->configurationParameter(QCanBusDevice::DataBitRateKey);
            if (isCanFd && dataBitRate.isValid())
            {
                m_status->setText(tr("Plugin: %1, connected to %2 at %3 / %4 kBit/s").arg(p.pluginName).arg(p.deviceInterfaceName)
                .arg(bitRate.toInt() / 1000).arg(dataBitRate.toInt() / 1000));
            }
            else
            {
                m_status->setText(tr("Plugin: %1, connected to %2 at %3 kBit/s").arg(p.pluginName).arg(p.deviceInterfaceName)
                .arg(bitRate.toInt() / 1000));
            }
        }
        else
        {
            m_status->setText(tr("Plugin: %1, connected to %2").arg(p.pluginName).arg(p.deviceInterfaceName));
        }
    }
    connect(m_L_statusCANTimer, &QTimer::timeout, this, [this]()
    {
        switch (m_canDevice->busStatus())
        {
        case QCanBusDevice::CanBusStatus::Good:m_ui->L_statusCAN->setText("Bien");
            break;
        case QCanBusDevice::CanBusStatus::Warning:m_ui->L_statusCAN->setText("Attention");
            break;
        case QCanBusDevice::CanBusStatus::Error:m_ui->L_statusCAN->setText("Erreur");
            break;
        case QCanBusDevice::CanBusStatus::BusOff:m_ui->L_statusCAN->setText("Eteint");
            break;
        default:m_ui->L_statusCAN->setText("Inconnu");
            break;
        }
    });
    if (m_canDevice->hasBusStatus()) m_L_statusCANTimer->start(2000);
    else m_ui->L_statusCAN->setText(tr("Non disponible"));
}

void MainWindow::disconnectDevice()
{
    if (!m_canDevice) return;
    m_L_statusCANTimer->stop();
    m_canDevice->disconnectDevice();
    m_ui->AC_connecter->setEnabled(true);
    m_ui->AC_deconnecter->setEnabled(false);
    m_ui->GB_messagesecrits->setEnabled(false);
    m_status->setText(tr("Déconnecté"));
}

void MainWindow::processFramesWritten(qint64 count)
{
    m_numberFramesWritten += count;
    m_written->setText(tr("%1 trames écrites").arg(m_numberFramesWritten));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    m_connectDialog->close();
    event->accept();
}

static QString frameFlags(const QCanBusFrame &frame)
{
    QString result = QLatin1String(" --- ");
    if (frame.hasBitrateSwitch()) result[1] = QLatin1Char('B');
    if (frame.hasErrorStateIndicator()) result[2] = QLatin1Char('E');
    if (frame.hasLocalEcho()) result[3] = QLatin1Char('L');
    return result;
}

void MainWindow::processReceivedFrames()
{
    if (!m_canDevice) return;
    while (m_canDevice->framesAvailable())
    {
        const QCanBusFrame frame = m_canDevice->readFrame();
        QString view;
        if (frame.frameType() == QCanBusFrame::ErrorFrame) view = m_canDevice->interpretErrorFrame(frame);
        else view = frame.toString();
        const QString time = QString::fromLatin1("%1.%2  ").arg(frame.timeStamp().seconds(), 10, 10, QLatin1Char(' '))
        .arg(frame.timeStamp().microSeconds() / 100, 4, 10, QLatin1Char('0'));
        const QString flags = frameFlags(frame);
        m_ui->TE_messagesrecus->append(time + flags + view);
        if (frame.frameId() == 0x26)
        {
            twoDV->setCanPosRobot(frame);
        }
    }
}

void MainWindow::sendFrame(const QCanBusFrame &frame) const
{
    if (!m_canDevice) return;
    m_canDevice->writeFrame(frame);
}

void MainWindow::on_AC_afficherTwoDV_triggered()
{
    twoDV->show();
    twoDV->activateWindow();
}
