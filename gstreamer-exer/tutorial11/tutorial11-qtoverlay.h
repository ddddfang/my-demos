// ubuntu1604 安装qt5 参考:
// https://www.cnblogs.com/deng-c-q/p/10435700.html

//GStreamre会负责媒体的播放及控制，GUI会负责处理用户的交互操作以及创建显示的窗口

#ifndef _QTOVERLAY_
#define _QTOVERLAY_

#include <gst/gst.h>

#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QTimer>

class PlayerWindow : public QWidget
{
Q_OBJECT
public:
    PlayerWindow(GstElement *p);

    WId getVideoWId() const ;
    static gboolean postGstMessage(GstBus * bus, GstMessage * message, gpointer user_data);

private slots:
    void onPlayClicked() ;
    void onPauseClicked() ;
    void onStopClicked() ;
    void onAlbumAvaiable(const QString &album);
    void onState(GstState st);
    void refreshSlider();
    void onSeek();
    void onEos();

signals:
    void sigAlbum(const QString &album);
    void sigState(GstState st);
    void sigEos();

private:
    GstElement *pipeline;
    QPushButton *playBt;
    QPushButton *pauseBt;
    QPushButton *stopBt;
    QWidget *videoWindow;
    QSlider *slider;
    QHBoxLayout *buttonLayout;
    QVBoxLayout *playerLayout;
    QTimer *timer;

    GstState state;
    gint64 totalDuration;
};

#endif


