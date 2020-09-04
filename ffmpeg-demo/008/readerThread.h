#pragma once

#include <QThread>
#include <QMutex>
#include <QImage>


class readerThread : public QThread {

    Q_OBJECT

public:
    explicit readerThread();
    ~readerThread();
    void pause(bool);
    void stop();
    void setFilePath(QString path);

protected:
    void run(); //这是 QThread 定义的方法

signals:
    void sigGotFrame(QImage);

private:
    bool mStop;     //
    bool mPause;     //
    QMutex mMutex;  //
    QString mFilePath;   //local file path
};


