
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
    //uint16_t status = 0;
    //int status = MI_OK;
    status = MI_OK;

    //memset(data, 0x00, 240);

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
    status = 0;
    RF_Power_Control(&MonLecteur, FALSE, 0);
    status = LEDBuzzer(&MonLecteur, LED_OFF);
    status = CloseCOM(&MonLecteur);
    qApp->quit();
}

// Open reader
void MainWindow::OpenReader(){
    status = OpenCOM(&MonLecteur);
    if (status != MI_OK){
        printf("Reader not found\n");
        goto done;
    }
    else{
        switch(MonLecteur.Type)
        {
        case ReaderTCP:
            sprintf(s_buffer, "IP : %s", MonLecteur.IPReader);
            break;
        case ReaderCDC:
            sprintf(s_buffer, "COM%d", MonLecteur.device);
            break;

        }
        printf("Reader found on %s\n", s_buffer);
    }


    char version[30];
    uint8_t serial[4];
    char stackReader[20];
    status = Version(&MonLecteur, version, serial, stackReader);
    if (status == MI_OK){
        printf("Reader firwmare is %s\n", version);
        printf("Reader serial is %02X%02X%02X%02X\n", serial[0], serial[1], serial[2], serial[3]);
        printf("Reader stack is %s\n", stackReader);
    }

    status = LEDBuzzer(&MonLecteur, LED_YELLOW_ON);
    if (status != MI_OK){
        printf("LED [FAILED]\n");
        goto close;
    }

    key_index = 0;
    status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyA, key_ff, key_index);
    if (status != MI_OK){
        printf("Load Key [FAILED]\n");
        goto close;
    }

    status = Mf_Classic_LoadKey(&MonLecteur, Auth_KeyB, key_ff, key_index);
    if (status != MI_OK){
        printf("Load Key [FAILED]\n");
        goto close;
    }

    // RF field ON
    RF_Power_Control(&MonLecteur, TRUE, 0);

    status = ISO14443_3_A_PollCard(&MonLecteur, atq, sak, uid, &uid_len);
    if (status != MI_OK){
        printf("No available tag in RF field\n");
        goto close;
    }

    printf("Tag found: UID=");
    for (i = 0; i < uid_len; i++)
        printf("%02X", uid[i]);
    printf(" ATQ=%02X%02X SAK=%02X\n", atq[1], atq[0], sak[0]);


    if ((atq[1] != 0x00) || ((atq[0] != 0x02) && (atq[0] != 0x04) && (atq[0] != 0x18))){
        printf("This is not a Mifare classic tag\n");
        goto tag_halt;
    }

    if ((sak[0] & 0x1F) == 0x08){
        // Mifare classic 1k : 16 sectors, 3+1 blocks in each sector
        printf("Tag appears to be a Mifare classic 1k\n");
        sect_count = 16;
    } else if ((sak[0] & 0x1F) == 0x18){
        // Mifare classic 4k : 40 sectors, 3+1 blocks in 32-first sectors, 15+1 blocks in the 8-last sectors
        printf("Tag appears to be a Mifare classic 4k\n");
        sect_count = 40;
    }


    //sect_count = 16;
    status = card_read(sect_count);

    goto tag_halt;

tag_halt:

    // Halt the tag
    status = ISO14443_3_A_Halt(&MonLecteur);
    if (status != MI_OK){
        printf("Failed to halt the tag\n");
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
        printf("Done\n");
    } else
    {
        printf("%s (%d)\n", GetErrorMessage(status), status);
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
        printf("Reading %d sectors...\n", sect_count);
        t0 = clock();
    }
    bloc = 0;
    for (sect = 0; sect < sect_count; sect++){
        if (!bench)
        printf("Reading sector %02d : ", sect);

        /*status = Mf_Classic_Authenticate(&MonLecteur, Auth_KeyB, FALSE, sect, key_ff, 0);

        status = Mf_Classic_Read_Sector(&MonLecteur, FALSE, sect, data, Auth_KeyA, 0);*/


        status = Mf_Classic_Read_Sector(&MonLecteur, TRUE, sect, data, Auth_KeyA, 0);

        if (status != MI_OK){
            if (bench)
                printf("Read sector %02d ", sect);
            printf("[Failed]\n");
            printf("  %s (%d)\n", GetErrorMessage(status), status);
            status = ISO14443_3_A_PollCard(&MonLecteur, atq, sak, uid, &uid_len);
            if (status != MI_OK){
                printf("No available tag in RF field\n");
                //goto close;
            }
        }
        else{
            if (!bench){
                printf("[OK]\n");
                // Display sector's data
                if (sect < 32)
                    bloc_count = 3;
                else
                    bloc_count = 15;
                for (bloc = 0; bloc < bloc_count; bloc++){
                    printf("%02d : ", bloc);
                    // Each blocks is 16-bytes wide
                    for (offset = 0; offset < 16; offset++){
                        printf("%02X ", data[16 * bloc + offset]);
                    }
                    for (offset = 0; offset < 16; offset++){
                        if (data[16 * bloc + offset] >= ' '){
                            printf("%c", data[16 * bloc + offset]);
                        } else
                            printf(".");

                    }
                    printf("\n");
                }
            }
        }
    }

    if (bench){
        t1 = clock();
        printf("Time elapsed: %ldms\n", (t1 - t0) / (CLOCKS_PER_SEC/1000));
    }
    return MI_OK;
}
