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
    void ReadBlock(int, char*);
    void on_Read_Clicked();
    void on_Write_Clicked();
    void ReadValue();
    void on_Increment_Clicked();
    void on_Decrement_Clicked();
    void on_ResetValue_Clicked();
    void on_Quit_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
