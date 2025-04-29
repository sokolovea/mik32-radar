#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QTime>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <qgraphicsscene.h>
#include <QGraphicsTextItem>

#define MAX_DISTANCE_CM (400)

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pushButtonConnect_clicked();
    void on_pushButtonDisconnect_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_actionExit_triggered();

    void on_actionAbout_triggered();

    void on_actionConnect_triggered();

    void on_actionDisconnect_triggered();

    void on_spinBoxStepDistance_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    QTimer *timerUpdateComPortList;
    QSerialPort serial;
    QTimer *comPortEchoTimer;
    bool isConnected = false;
    QGraphicsScene* scene;
    QTimer *fadeTimer;
    std::vector<QGraphicsTextItem*> stepDistancesArray;

    void updateComPortsList();
    void updateStepDistance(int newStep);
};
#endif // MAINWINDOW_H
