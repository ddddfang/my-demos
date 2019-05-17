#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QThread>
#include <QMutex>

class myThread:public QThread
{
    Q_OBJECT

public:
    myThread();
    void stop();

private:
    bool mStop;     //
    QMutex mMutex;  //

    void run();
signals:
    void processOK(int);    //when thread process ok, send this signal, and with a int given;
};

#endif // MYTHREAD_H
