#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <QThread>
#include <QMutex>
#include <QImage>
#include <iostream>
#include <stdio.h>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

using namespace  std;


class videoPlayer:public QThread
{
    Q_OBJECT

public:
    explicit videoPlayer();
    ~videoPlayer();
    void stop();
    void setFilePath(QString path);
protected:
    void run();
    bool isRealtime(AVFormatContext *pFormatCtx);
signals:
    void processOK(QImage);
    void videoTimeOK(qint64);
private:
    bool mStop;     //
    QMutex mMutex;  //
    QString mFilePath;   //local file path
};


#endif // VIDEOPLAYER_H
