#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QPainter>
#include <QFileDialog>
#include <iostream>
using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mPlayer = new videoPlayer();

    //"mPlayer" send,use func processOK(),    "this" recv, use func getProcessRes
    connect(mPlayer,SIGNAL(processOK(QImage)),this,SLOT(getProcessRes(QImage)));
    //connect(mPlayer, SIGNAL(finished()), mPlayer, SLOT(deleteLater()));
    connect(mPlayer,SIGNAL(videoTimeOK(qint64)),this,SLOT(getVideoTime(qint64)));
}

MainWindow::~MainWindow()
{
    if(mPlayer->isRunning())
    {
        mPlayer->stop();
    }
    mPlayer->wait();
    delete mPlayer;
    delete ui;
}

void MainWindow::getProcessRes(QImage img)
{
    mImage = img;
    update();   //will trigger paintEvent
}

void MainWindow::getVideoTime(qint64 totalTime)
{
    mTimeTotal = totalTime / 1000000;   //total time in second

    ui->horizontalSlider->setRange(0, mTimeTotal);

    int hour = mTimeTotal / 60 / 60;        //
    int min  = (mTimeTotal / 60 ) % 60;     //
    int sec  = mTimeTotal % 60;             //

    ui->labelTime->setText(QString("00.00.00 / %1:%2:%3").arg(hour, 2, 10, QLatin1Char('0'))
                           .arg(min, 2, 10, QLatin1Char('0'))
                           .arg(sec, 2, 10, QLatin1Char('0')));
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    painter.setBrush(Qt::black);
    painter.drawRect(0, 0, this->width(), this->height());//draw black first
    if (mImage.size().width() <= 0)
        return;
    QImage img = mImage.scaled(this->size(),Qt::KeepAspectRatio);
    int x = this->width() - img.width();
    int y = this->height() - img.height();
    x /= 2;
    y /= 2;
    painter.drawImage(QPoint(x,y),img);
}

void MainWindow::on_pushButtonStart_clicked()
{
    if(!mPlayer->isRunning())
    {
        mPlayer->start();
    }
}

void MainWindow::on_pushButtonStop_clicked()
{
    if(mPlayer->isRunning())
    {
        mPlayer->stop();
    }
}

void MainWindow::on_pushButtonSeekPrev_clicked()
{
    cout<<"on_pushButtonSeekPrev_clicked"<<endl;
}

void MainWindow::on_pushButtonSeekNext_clicked()
{
    cout<<"on_pushButtonSeekNext_clicked"<<endl;
}

void MainWindow::on_pushButtonOpen_clicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "chose file to play", "/","(*.264 *.mp4 *.rmvb *.avi *.mov *.flv *.mkv *.ts *.mp3 *.flac *.ape *.wav)");
    cout<<"on_pushButtonOpen_clicked:"<< filePath.toStdString() <<endl;
    ui->lineEditFilePath->setText(filePath);
    mPlayer->setFilePath(filePath);
}

void MainWindow::on_horizontalSlider_sliderMoved(int position)
{
    cout << "on_horizontalSlider_sliderMoved" <<position <<endl;
}
