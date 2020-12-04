#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ODALID.h"

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
    void on_Connect_clicked();
    void on_Saisie_clicked();
    void on_Quit_clicked();
    void OpenReader();
    int card_read(BYTE);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
