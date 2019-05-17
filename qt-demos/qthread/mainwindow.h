#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "mythread.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionsubitem0_triggered();

    void on_actionitem1_triggered();

    void on_actionsubitem1_triggered();

    void on_actionsubitem2_triggered();

    void on_pushButton0_clicked();

    void on_pushButton1_clicked();

    void on_pushButton2_clicked();

    void getProcessRes(int);
private:
    Ui::MainWindow *ui;
    myThread *t;
};

#endif // MAINWINDOW_H
