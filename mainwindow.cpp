#pragma comment(lib, "ODALID.lib")
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ODALID.h"
#include <QtGui>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

ReaderName MonLecteur;

char pszHost[] = "192.168.1.4";

void MainWindow::on_Connect_clicked(){
    uint16_t status = 0;
    //MonLecteur.Type = ReaderTCP;
    //strcpy(MonLecteur.IPReader, pszHost);
    MonLecteur.Type = ReaderCDC;
    MonLecteur.device = 0;
    status = OpenCOM(&MonLecteur);
    qDebug() << "OpenCOM" << status;

    char version[30];
    uint8_t serial[4];
    char stackReader[20];
    status = Version(&MonLecteur, version, serial, stackReader);
    ui->Affichage->setText(version);
    ui->Affichage->update();
}

void MainWindow::on_Saisie_clicked(){
    QString Text = ui->Affichage_2->toPlainText();
    qDebug() << "Text : " << Text;
}

void MainWindow::on_Quit_clicked(){
    int16_t status = 0;
    RF_Power_Control(&MonLecteur, FALSE, 0);
    status = LEDBuzzer(&MonLecteur, LED_OFF);
    status = CloseCOM(&MonLecteur);
    qApp->quit();
}
