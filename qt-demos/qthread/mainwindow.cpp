#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    t = new myThread();

    //"t" send,use func processOK(),    "this" recv, use func getProcessRes
    connect(t,SIGNAL(processOK(int)),this,SLOT(getProcessRes(int)));
}

MainWindow::~MainWindow()
{
    delete t;
    delete ui;
}

void MainWindow::on_actionsubitem0_triggered()
{
    cout<<"on_actionsubitem0_triggered"<<endl;
}

void MainWindow::on_actionitem1_triggered()
{
    cout<<"on_actionitem1_triggered"<<endl;
}

void MainWindow::on_actionsubitem1_triggered()
{
    cout<<"on_actionsubitem1_triggered"<<endl;
}

void MainWindow::on_actionsubitem2_triggered()
{
    cout<<"on_actionsubitem2_triggered"<<endl;
}

void MainWindow::on_pushButton0_clicked()
{
    cout<<"on_pushButton0_clicked"<<endl;
    t->start();
}

void MainWindow::on_pushButton1_clicked()
{
    cout<<"on_pushButton1_clicked"<<endl;
    t->stop();
}

void MainWindow::on_pushButton2_clicked()
{
    cout<<"on_pushButton2_clicked"<<endl;
}

void MainWindow::getProcessRes(int res)
{
    cout<<"getProcessRes,res="<<res<<endl;
}
