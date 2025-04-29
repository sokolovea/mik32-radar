#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QGraphicsTextItem>
#include <QMessageBox>

void drawSector(QGraphicsScene *scene, int angle, int distance, double stepDistance)
{
    if (distance < 0) distance = MAX_DISTANCE_CM;

    double radius = 105 * 4;
    double startAngle = angle - 0.5;
    double endAngle = angle + 0.5;

    double maxDistance = stepDistance * 4;

    double coeff = distance / maxDistance > 1.0 ? 1.0 : distance / maxDistance;

    QPainterPath pathRed;
    pathRed.moveTo(0, 0);
    pathRed.arcTo(-radius, -radius, 2 * radius, 2 * radius, -startAngle, -(endAngle - startAngle));
    pathRed.closeSubpath();

    QPainterPath pathGreen;
    pathGreen.moveTo(0, 0);
    pathGreen.arcTo(-radius * coeff, -radius * coeff, 2 * radius * coeff, 2 * radius * coeff, -startAngle, -(endAngle - startAngle));
    pathGreen.closeSubpath();


    QGraphicsPathItem *sectorRed = new QGraphicsPathItem(pathRed);
    sectorRed->setBrush(QBrush(Qt::red));
    sectorRed->setPen(Qt::NoPen);
    sectorRed->setOpacity(1.0);

    scene->addItem(sectorRed);

    QGraphicsPathItem *sectorGreen = new QGraphicsPathItem(pathGreen);
    sectorGreen->setBrush(QBrush(Qt::green));
    sectorGreen->setPen(Qt::NoPen);
    sectorGreen->setOpacity(1.0);

    scene->addItem(sectorGreen);

    // Уменьшение прозрачности и удаление
    QTimer *fadeTimer = new QTimer();
    double *opacity = new double(1.0);
    QObject::connect(fadeTimer, &QTimer::timeout, [sectorRed, sectorGreen, fadeTimer, opacity]() {
        *opacity -= 0.1;
        if (*opacity <= 0.0) {
            fadeTimer->stop();
            delete fadeTimer;
            delete sectorRed;
            delete sectorGreen;
            delete opacity;
        } else {
            sectorRed->setOpacity(*opacity);
            sectorGreen->setOpacity(*opacity);
        }
    });
    fadeTimer->start(50);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    stepDistancesArray = std::vector<QGraphicsTextItem*>();
    timerUpdateComPortList = new QTimer();
    connect(timerUpdateComPortList, &QTimer::timeout, this, &MainWindow::updateComPortsList);
    timerUpdateComPortList->start(500);


    scene = new QGraphicsScene(this);
    ui->graphicsViewRadar->setScene(scene);
    ui->graphicsViewRadar->setRenderHint(QPainter::Antialiasing);
    scene->setSceneRect(-200, -500, 400, 400);

    QPen gridPen(Qt::gray);
    gridPen.setWidth(10);

    for (int r = 5; r <= 500; r += 105) {
        scene->addEllipse(-r, -r, r * 2, r * 2, gridPen);
    }

    for (int angle = 0; angle <= 180; angle += 30) {
        double radians = qDegreesToRadians(static_cast<double>(angle));
        double x = 460 * std::cos(radians);
        double y = -455 * std::sin(radians);
        QGraphicsTextItem *text = scene->addText(QString::number(angle) + "°");
        text->setFont(QFont("Mono", 20));
        text->setDefaultTextColor(Qt::black);
        text->setPos(x - text->boundingRect().width() / 2, y - text->boundingRect().height() / 2);
    }
    updateStepDistance(ui->spinBoxStepDistance->value());
}

void MainWindow::updateStepDistance(int newStep) {
    qDeleteAll(stepDistancesArray);
    stepDistancesArray.clear();


    for (int i = 0; i <= 4; i += 1) {
        double x = 20 + 75 * i;
        double y = -20 + -x;
        QGraphicsTextItem *text = scene->addText(QString::number(i * newStep) + " см");
        stepDistancesArray.push_back(text);
        text->setFont(QFont("Mono", 18));
        text->setRotation(45);
        text->setDefaultTextColor(Qt::black);
        text->setPos(x, y);
        text->setZValue(1.0);
    }
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::updateComPortsList()
{
    if (!isConnected) {
        const auto serialPortInfos = QSerialPortInfo::availablePorts();
        QList<QString> serialPortInfosQList;

        for (const auto& each: serialPortInfos) {
            serialPortInfosQList.append(each.portName());
        }

        QList<QString> items;
        for (int i = 0; i < ui->comboBoxComPort->count(); ++i) {
            items.append(ui->comboBoxComPort->itemText(i));
        }
        if (items != serialPortInfosQList) {
            ui->comboBoxComPort->clear();
            for (const QSerialPortInfo &portInfo : serialPortInfos) {
                if (portInfo.hasProductIdentifier()) {
                    ui->comboBoxComPort->addItem(portInfo.portName());
                }
            }
        }

        if (ui->comboBoxComPort->count() == 0) {
            ui->pushButtonConnect->setDisabled(true);
            ui->actionConnect->setDisabled(true);
        } else {
            ui->pushButtonConnect->setEnabled(true);
            ui->actionConnect->setEnabled(true);
        }

    }
}


void MainWindow::on_pushButtonConnect_clicked()
{
    if (ui->comboBoxComPort->currentText().length() > 0) {
        serial.setPortName(ui->comboBoxComPort->currentText());
        timerUpdateComPortList->stop();

        if(serial.open(QIODevice::ReadWrite)) {
            serial.setBaudRate(QSerialPort::Baud115200);
            serial.setDataBits(QSerialPort::Data8);
            serial.setParity(QSerialPort::NoParity);
            serial.setStopBits(QSerialPort::OneStop);
            serial.setFlowControl(QSerialPort::NoFlowControl);
            ui->comboBoxComPort->setDisabled(true);
            ui->pushButtonConnect->setDisabled(true);
            ui->actionConnect->setDisabled(true);
        } else {
            qDebug() << "Error opening port:" << serial.errorString();
        }
        ui->horizontalSlider->setEnabled(true);
        ui->horizontalSlider->setValue(10);
        on_horizontalSlider_valueChanged(10);
        char byteToSend = 10;
        qint64 bytesWritten = serial.write(&byteToSend, 1);

        QObject::connect(&serial, &QSerialPort::readyRead, [&]()
        {
         static QByteArray buffer;
         buffer.append(serial.readAll());
         int endIndex = buffer.indexOf('\n');
         while(endIndex != -1) {
             QByteArray message = buffer.left(endIndex).trimmed();
             buffer = buffer.mid(endIndex + 1);
             qDebug() << "Received:" << message;

             QStringList parts = QString(message).split(' ');

             if (parts.size() == 2) {
                 bool ok1, ok2;
                 int num1 = parts[0].toInt(&ok1);
                 int num2 = parts[1].toInt(&ok2);
                 drawSector(this->scene, -num1, num2, ui->spinBoxStepDistance->value());
             }
             endIndex = buffer.indexOf('\n');
         }
        });

        comPortEchoTimer = new QTimer(this);
        connect(comPortEchoTimer, &QTimer::timeout, [&]()
        {
            if (!serial.isOpen()) {
            qDebug() << "Обрыв соединения! Причина:" << serial.errorString();
                comPortEchoTimer->stop();
                serial.close();
            }
        });

        comPortEchoTimer->start(500);
        ui->pushButtonDisconnect->setEnabled(true);
        ui->actionDisconnect->setEnabled(true);
    }
}


void MainWindow::on_pushButtonDisconnect_clicked()
{
    if (comPortEchoTimer) {
        comPortEchoTimer->stop();
        delete comPortEchoTimer;
        comPortEchoTimer = nullptr;
    }

    if (serial.isOpen()) {
        QObject::disconnect(&serial, nullptr, nullptr, nullptr);
        serial.close();
        qDebug() << "Порт закрыт.";
    }
    ui->comboBoxComPort->setDisabled(false);
    ui->pushButtonConnect->setDisabled(false);
    ui->actionConnect->setDisabled(false);
    ui->pushButtonDisconnect->setDisabled(true);
    ui->actionDisconnect->setDisabled(true);
    ui->horizontalSlider->setDisabled(true);
    timerUpdateComPortList->start();
}


void MainWindow::on_horizontalSlider_valueChanged(int value)
{
    if (serial.isOpen()) {
        char byteToSend = value;
        qint64 bytesWritten = serial.write(&byteToSend, 1);

        if (bytesWritten == -1) {
            qDebug() << "Ошибка записи:" << serial.errorString();
        } else if (!serial.waitForBytesWritten(1000)) {
            qDebug() << "Таймаут записи";
        } else {
            qDebug() << "Байт успешно отправлен";
        }
    }
}


void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}


void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "О программе", "Разработчик - Соколов Егор");
}


void MainWindow::on_actionConnect_triggered()
{
    on_pushButtonConnect_clicked();
}


void MainWindow::on_actionDisconnect_triggered()
{
    on_pushButtonDisconnect_clicked();
}


void MainWindow::on_spinBoxStepDistance_valueChanged(int arg1)
{
    updateStepDistance(arg1);
}

