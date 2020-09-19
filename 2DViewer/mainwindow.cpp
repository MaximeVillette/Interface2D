#include "mainwindow.h"
#include "sendframebox.h"
#include "connectdialog.h"
#include "twodv.h"

#include "ui_mainwindow.h"
#include "ui_sendframebox.h"
#include "ui_connectdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), m_ui(new Ui::MainWindow),
    m_busStatusTimer(new QTimer(this))
{

    m_ui->setupUi(this);

    ifstream flux(paramCANPath);
    if(flux) for(int i=0; i<SIZE_TAB; i++) flux >> defSet[i];
    else QMessageBox::warning(this,"Erreur","Impossible d'ouvrir les réglages CAN !");

    m_connectDialog = new ConnectDialog; //Créer une nouvelle fenêtre de dialog (ici pour connecter un périphérique au busCAN)
    m_status = new QLabel; //Nouveau label ... autres
    m_ui->statusBar->addPermanentWidget(m_status); //Ajout d'une fenêtre permanente dans la bar d'état du parent (ConnectDialog) ... autres
    m_written = new QLabel; //Nouveau label ... autres
    m_ui->statusBar->addWidget(m_written); //Ajout d'une fenêtre dans la barre d'état du parent (ConnectDialog) ... autres

    initActionsConnections(); //Initialise les actions (voir ligne XX pour plus d'informations) ... lignes

    QTimer::singleShot(50, m_connectDialog, &ConnectDialog::show); //Timer d'un seul signal de 50ms qui montre le parent (ConnectDialog) ... autres

    twoDV = new TwoDV(this); //Nouvelle fenêtre
    twoDV->show(); //Montrer la fenêtre

    connect(m_busStatusTimer, &QTimer::timeout, this, &MainWindow::busStatus);
    if(defSet[5]=="1") QTimer::singleShot(500, this, SLOT(closeMain()));
}

MainWindow::~MainWindow() //Action sur MainWindow
{
    delete m_connectDialog;
    delete m_ui;
}

void MainWindow::initActionsConnections()
{
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->sendFrameBox->setEnabled(false);

    connect(m_ui->sendFrameBox, &SendFrameBox::sendFrame, this, &MainWindow::sendFrame);
    connect(m_ui->actionConnect, &QAction::triggered, [this]() {
        m_canDevice.release()->deleteLater();
        m_connectDialog->show();
    });
    connect(m_connectDialog, &QDialog::accepted, this, &MainWindow::connectDevice);
    connect(m_ui->actionDisconnect, &QAction::triggered, this, &MainWindow::disconnectDevice);
    connect(m_ui->actionResetController, &QAction::triggered, this, [this]() {
        m_canDevice->resetController();
    });
    connect(m_ui->actionQuit, &QAction::triggered, this, &QWidget::close);
    connect(m_ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(m_ui->actionClearLog, &QAction::triggered, m_ui->receivedMessagesEdit, &QTextEdit::clear);
    connect(m_ui->actionPluginDocumentation, &QAction::triggered, this, []() {
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
    case QCanBusDevice::UnknownError:
        m_status->setText(m_canDevice->errorString());
        break;
    default:
        break;
    }
}

void MainWindow::connectDevice()
{
    const ConnectDialog::Settings p = m_connectDialog->settings();

    QString errorString;
    m_canDevice.reset(QCanBus::instance()->createDevice(p.pluginName, p.deviceInterfaceName,
                                                        &errorString));
    if (!m_canDevice) {
        m_status->setText(tr("Error creating device '%1', reason: '%2'")
                          .arg(p.pluginName).arg(errorString));
        return;
    }
    m_numberFramesWritten = 0;

    connect(m_canDevice.get(), &QCanBusDevice::errorOccurred,this, &MainWindow::processErrors);
    connect(m_canDevice.get(), &QCanBusDevice::framesReceived,this, &MainWindow::processReceivedFrames);
    connect(m_canDevice.get(), &QCanBusDevice::framesWritten,this, &MainWindow::processFramesWritten);

    if (p.useConfigurationEnabled) {
        for (const ConnectDialog::ConfigurationItem &item : p.configurations)
            m_canDevice->setConfigurationParameter(item.first, item.second);
    }
    if (!m_canDevice->connectDevice()) {
        m_status->setText(tr("Connection error: %1").arg(m_canDevice->errorString()));
        m_canDevice.reset();
    }
    else {
        m_ui->actionConnect->setEnabled(false);
        m_ui->actionDisconnect->setEnabled(true);
        m_ui->sendFrameBox->setEnabled(true);
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
        if (m_canDevice->hasBusStatus()) m_busStatusTimer->start(2000);
        else m_ui->busStatus->setText(tr("Non disponible"));
    }
}

void MainWindow::busStatus()
{
    if (!m_canDevice || !m_canDevice->hasBusStatus()) {
        m_ui->busStatus->setText(tr("No CAN bus status available."));
        m_busStatusTimer->stop();
        return;
    }

    switch (m_canDevice->busStatus()) {
    case QCanBusDevice::CanBusStatus::Good:
        m_ui->busStatus->setText("CAN bus status: Good.");
        break;
    case QCanBusDevice::CanBusStatus::Warning:
        m_ui->busStatus->setText("CAN bus status: Warning.");
        break;
    case QCanBusDevice::CanBusStatus::Error:
        m_ui->busStatus->setText("CAN bus status: Error.");
        break;
    case QCanBusDevice::CanBusStatus::BusOff:
        m_ui->busStatus->setText("CAN bus status: Bus Off.");
        break;
    default:
        m_ui->busStatus->setText("CAN bus status: Unknown.");
        break;
    }
}

void MainWindow::disconnectDevice()
{
    if (!m_canDevice) return;
    m_busStatusTimer->stop();
    m_canDevice->disconnectDevice();
    m_ui->actionConnect->setEnabled(true);
    m_ui->actionDisconnect->setEnabled(false);
    m_ui->sendFrameBox->setEnabled(false);
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
        const char * const idFormat = frame.hasExtendedFrameFormat() ? "%08X" : "%03X";
        uint fid = static_cast<uint>(frame.frameId());
        QString frID = QString::asprintf(idFormat, fid);
        QString view;
        if (frame.frameType() == QCanBusFrame::ErrorFrame) view = m_canDevice->interpretErrorFrame(frame);
        else view = frame.toString();
        const QString time = QString::fromLatin1("%1.%2  ").arg(frame.timeStamp().seconds(), 10, 10, QLatin1Char(' '))
        .arg(frame.timeStamp().microSeconds() / 100, 4, 10, QLatin1Char('0'));
        const QString flags = frameFlags(frame);
        m_ui->receivedMessagesEdit->append(QString("%0           %1 %2").arg(time).arg(flags).arg(view));
        if(twoDV->activeMode())
        {
            if(frID == "028") twoDV->setCanPosRobot(frame);
            if(frID == "27A") twoDV->setCanVentRobot(frame);
            if(frID == "106") twoDV->setCanServoRobot(frame);
            if(frID == "4B0") twoDV->setCanColorsRobot(frame);
        }
    }
}

void MainWindow::sendFrame(const QCanBusFrame &frame) const
{
    if (!m_canDevice) return;
    m_canDevice->writeFrame(frame);
}

void MainWindow::on_actionShow2DV_triggered()
{
    twoDV->show();
    twoDV->activateWindow();
}

void MainWindow::closeMain() {MainWindow::close();}
