#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "videoplayer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void paintEvent(QPaintEvent *);

private:
    Ui::MainWindow *ui;
    QImage mImage;
    videoPlayer *mPlayer;
    qint64 mTimeTotal;   //

private slots:
    void getProcessRes(QImage);
    void getVideoTime(qint64);
    void on_pushButtonStart_clicked();
    void on_pushButtonStop_clicked();
    void on_pushButtonSeekPrev_clicked();
    void on_pushButtonSeekNext_clicked();
    void on_pushButtonOpen_clicked();
    void on_horizontalSlider_sliderMoved(int position);
};

#endif // MAINWINDOW_H
