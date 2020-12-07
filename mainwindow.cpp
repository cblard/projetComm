
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

uint8_t key_ff[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

int card_read(BYTE sect_count);

BOOL bench = TRUE;

ReaderName MonLecteur;

uint16_t status = 0;
char pszHost[] = "192.168.1.4";
int i;
char s_buffer[64];
uint8_t atq[2];
uint8_t sak[1];
uint8_t uid[12];
uint16_t uid_len = 12;
uint8_t sect_count = 0;
uint8_t key_index;
uint8_t data[240];

void MainWindow::on_Connect_clicked(){
    status = MI_OK;

    //memset(data, 0x00, 240);

    //MonLecteur.Type = ReaderTCP;
    //strcpy(MonLecteur.IPReader, pszHost);
    MonLecteur.Type = ReaderCDC;
    MonLecteur.device = 0;
    status = OpenCOM(&MonLecteur);
    ui->Affichage->setText(ui->Affichage->toPlainText()+"OpenCOM" + status + "\n");
    qDebug() <<"OpenCOM" << status;

    char version[30];
    uint8_t serial[4];
    char stackReader[20];
    status = Version(&MonLecteur, version, serial, stackReader);
    ui->Affichage->setText(ui->Affichage->toPlainText()+ version + "\n");
    //ui->Affichage->update();
    if(status != MI_OK){
        ui->Affichage->setText(ui->Affichage->toPlainText()+"Connection failed\n");
    }
}

void MainWindow::on_Saisie_clicked(){
    QString Text = ui->Affichage_2->toPlainText();
    ui->Affichage_2->clear();
    qDebug() << "Text : " << Text;
}

void MainWindow::on_Quit_clicked(){
    status = 0;
    RF_Power_Control(&MonLecteur, FALSE, 0);
    status = LEDBuzzer(&MonLecteur, LED_OFF);
    status = CloseCOM(&MonLecteur);
    qApp->quit();
}

void MainWindow::on_OpenReaderButton_clicked(){
    OpenReader();
}

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
}

int MainWindow::card_read(BYTE sect_count)
{
    uint8_t data[240];
    clock_t t0, t1;
    uint8_t bloc_count, bloc, sect;
    uint8_t offset;
    status = 0;
    uint8_t atq[2];
    uint8_t sak[1];
    uint8_t uid[12];
    uint16_t uid_len = 12;

    if (bench){
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Reading " + sect_count + " sectors...\n");
        t0 = clock();
    }
    bloc = 0;
    for (sect = 0; sect < sect_count; sect++){
        if (!bench)
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Reading sector " + sect + "2d : ");

        /*status = Mf_Classic_Authenticate(&MonLecteur, Auth_KeyB, FALSE, sect, key_ff, 0);

        status = Mf_Classic_Read_Sector(&MonLecteur, FALSE, sect, data, Auth_KeyA, 0);*/


        status = Mf_Classic_Read_Sector(&MonLecteur, TRUE, sect, data, Auth_KeyA, 0);

        if (status != MI_OK){
            if (bench)
                ui->Affichage->setText(ui->Affichage->toPlainText()+ "Read sector " + sect + "2d ");
            ui->Affichage->setText(ui->Affichage->toPlainText()+ "[Failed]\n");
            ui->Affichage->setText(ui->Affichage->toPlainText()+ "  " + GetErrorMessage(status) + "(" + status + ")\n");
            status = ISO14443_3_A_PollCard(&MonLecteur, atq, sak, uid, &uid_len);
            if (status != MI_OK){
                ui->Affichage->setText(ui->Affichage->toPlainText()+ "No available tag in RF field\n");
                //goto close;
            }
        }
        else{
            if (!bench){
                ui->Affichage->setText(ui->Affichage->toPlainText()+ "[OK]\n");
                // Display sector's data
                if (sect < 32)
                    bloc_count = 3;
                else
                    bloc_count = 15;
                for (bloc = 0; bloc < bloc_count; bloc++){
                    ui->Affichage->setText(ui->Affichage->toPlainText()+ bloc + "2d : ");
                    // Each blocks is 16-bytes wide
                    for (offset = 0; offset < 16; offset++){
                        ui->Affichage->setText(ui->Affichage->toPlainText()+ data[16 * bloc + offset] + "2X ");
                    }
                    for (offset = 0; offset < 16; offset++){
                        if (data[16 * bloc + offset] >= ' '){
                            ui->Affichage->setText(ui->Affichage->toPlainText()+ data[16 * bloc + offset]);
                        } else
                            ui->Affichage->setText(ui->Affichage->toPlainText()+ ".");

                    }
                    ui->Affichage->setText(ui->Affichage->toPlainText()+ "\n");
                }
            }
        }
    }

    if (bench){
        t1 = clock();
        ui->Affichage->setText(ui->Affichage->toPlainText()+ "Time elapsed: " + (t1 - t0) / (CLOCKS_PER_SEC/1000) + "dms\n");
    }
    return MI_OK;
}

void MainWindow::on_Clear_clicked(){
    ui->Affichage->clear();
}
