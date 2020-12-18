
#include "mainwindow.h"
#include "ui_mainwindow.h"
//#include "ODALID.h"
#include <QtGui>

#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "MfErrNo.h"

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

BOOL connected = false;

void MainWindow::on_Connect_clicked(){
    if(!connected){
        uint16_t status = 0;
        uint8_t serial[4];
        char version[30];
        char stackReader[20];

        unsigned char key_t1[6] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 };
        unsigned char key_t2[6] = { 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5 };
        unsigned char key_t3[6] = { 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5 };
        unsigned char key_t4[6] = { 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5 };

        MonLecteur.Type = ReaderCDC;
        MonLecteur.device = 0;

        status = OpenCOM(&MonLecteur);
        qDebug() <<"OpenCOM" << status;

        status = Version(&MonLecteur, version, serial, stackReader);

        if(status == 0){
            ui->Version_Label->setText(version);
            ui->Connected_Label->setText("Connected");
            connected = true;
        }

        status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyA, key_t1, 0);
        status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyB, key_t2, 0);
        status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyA, key_t3, 1);
        status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyB, key_t4, 1);

        this->ReadValue();
        ui->Affichage_1->setText("Last name");
        ui->Affichage_1->setText("First name");
    }
}

void MainWindow::ReadBlock(int bloc, char *data){
    BYTE offset;
    QString dataValue;

    int status = Mf_Classic_Read_Block(&MonLecteur, TRUE, bloc, (uint8_t*) data, Auth_KeyA, 0);

    for(offset = 0; offset < 16; offset++){
        dataValue.asprintf("%02X", data[16*bloc+offset]);
        qDebug() << "       " << dataValue;
    }
}

void MainWindow::on_Read_Clicked(){
    char data;
    this->ReadBlock(10, &data);
    ui->Affichage_1->setText((QString)data);
    this->ReadBlock(9, &data);
    ui->Affichage_2->setText((QString)data);
}

void MainWindow::on_Write_Clicked(){
    int status = 0;
    unsigned char LastName[16];
    unsigned char FirstName[16];

    strncpy((char*)LastName, ui->Affichage_1->toPlainText().toUtf8().data(), 16);
    strncpy((char*)FirstName, ui->Affichage_2->toPlainText().toUtf8().data(), 16);

    status = Mf_Classic_Write_Block(&MonLecteur, TRUE, 10, (uint8_t*)LastName, Auth_KeyB, 2);
    status = Mf_Classic_Write_Block(&MonLecteur, TRUE, 9, (uint8_t*)FirstName, Auth_KeyB, 2);
}

void MainWindow::ReadValue(){
    uint32_t value;
    int status = Mf_Classic_Read_Value(&MonLecteur, TRUE, 14, &value, Auth_KeyA, 1);
    ui->Value_Label->setText((QString)value);
}

void MainWindow::on_Increment_Clicked(){
    int status = Mf_Classic_Increment_Value(&MonLecteur, TRUE, 14, 1, 13, Auth_KeyB, 1);
    status = Mf_Classic_Restore_Value(&MonLecteur, FALSE, 13, 14, Auth_KeyB, 1);

    this->ReadValue();
}

void MainWindow::on_Decrement_Clicked(){
    int status = Mf_Classic_Decrement_Value(&MonLecteur, TRUE, 14, 1, 13, Auth_KeyB, 1);
    status = Mf_Classic_Restore_Value(&MonLecteur, FALSE, 13, 14, Auth_KeyB, 1);

    this->ReadValue();
}

void MainWindow::on_ResetValue_Clicked(){
    int status = Mf_Classic_Write_Value(&MonLecteur, TRUE, 14, 0, Auth_KeyA, 1);

    this->ReadValue();
}

void MainWindow::on_Quit_clicked(){
    int16_t status;
    RF_Power_Control(&MonLecteur, FALSE, 0);
    status = LEDBuzzer(&MonLecteur, LED_OFF);
    status = CloseCOM(&MonLecteur);
    qApp->quit();
}


/*
void MainWindow::OpenReader(){
    //status = OpenCOM(&MonLecteur);
    if (status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+"Reader not found\n");
        goto done;
    }
    else{
        switch(MonLecteur.Type)
        {
        case ReaderTCP:
            ui->Affichage->setText(ui->Affichage->toPlainText()+ s_buffer + "IP : " + MonLecteur.IPReader);
            break;
        case ReaderCDC:
            ui->Affichage->setText(ui->Affichage->toPlainText()+ s_buffer + "COM" + MonLecteur.device);
            break;

        }
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Reader found on " + s_buffer + "\n");
    }


    char version[30];
    uint8_t serial[4];
    char stackReader[20];
    status = Version(&MonLecteur, version, serial, stackReader);
    if (status == MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Reader firwmare is " + version + "\n");
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Reader serial is " + serial[0] + "2X" + serial[1] + "2X" + serial[2] + "2X" + serial[3] + "2X\n");
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Reader stack is " + stackReader + "\n");
    }

    status = LEDBuzzer(&MonLecteur, LED_YELLOW_ON);
    if (status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "LED [FAILED]\n");
        goto close;
    }

    key_index = 0;
    status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyA, key_ff, key_index);
    if (status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Load Key [FAILED]\n");
        goto close;
    }

    status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyB, key_ff, key_index);
    if (status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Load Key [FAILED]\n");
        goto close;
    }

    // RF field ON
    RF_Power_Control(&MonLecteur, TRUE, 0);

    status = ISO14443_3_A_PollCard(&MonLecteur, atq, sak, uid, &uid_len);
    if (status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "No available tag in RF field\n");
        goto close;
    }

    ui->Affichage->setText(ui->Affichage->toPlainText()+ "Tag found: UID=");
    for (i = 0; i < uid_len; i++)
        ui->Affichage->setText(ui->Affichage->toPlainText()+ uid[i] + "2X");
    ui->Affichage->setText(ui->Affichage->toPlainText()+ " ATQ=" + atq[1] + "2X" + atq[0] + "2X SAK=" + sak[0] + "2X\n");


    if ((atq[1] != 0x00) || ((atq[0] != 0x02) && (atq[0] != 0x04) && (atq[0] != 0x18))){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "This is not a Mifare classic tag\n");
        goto tag_halt;
    }

    if ((sak[0] & 0x1F) == 0x08){
        // Mifare classic 1k : 16 sectors, 3+1 blocks in each sector
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Tag appears to be a Mifare classic 1k\n");
        sect_count = 16;
    } else if ((sak[0] & 0x1F) == 0x18){
        // Mifare classic 4k : 40 sectors, 3+1 blocks in 32-first sectors, 15+1 blocks in the 8-last sectors
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Tag appears to be a Mifare classic 4k\n");
        sect_count = 40;
    }


    //sect_count = 16;
    status = card_read(sect_count);

    goto tag_halt;

tag_halt:

    // Halt the tag
    status = ISO14443_3_A_Halt(&MonLecteur);
    if (status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Failed to halt the tag\n");
        goto close;

    }
close:
    // Close the reader

    RF_Power_Control(&MonLecteur, FALSE, 0);


    CloseCOM(&MonLecteur);

done:
    // Display last error
    if (status == MI_OK)
    {
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Done\n");
    } else
    {
        ui->Affichage->setText(ui->Affichage->toPlainText() + "  " + GetErrorMessage(status) + "(" + status + ")\n");
    }
}*/
