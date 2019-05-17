#include "mythread.h"
#include <QDebug>
myThread::myThread():mStop(false)
{
}

void myThread::stop()
{
    qDebug()<<"myThread::stop call. id="<<currentThreadId();
    //QMutexLocker locker(&mMutex);
    mMutex.lock();
    mStop = true;
    mMutex.unlock();
}

void myThread::run()
{
    unsigned int i=0;
    qDebug()<<"myThread::run enter. id="<<currentThreadId();

    mMutex.lock();
    mStop = false;
    mMutex.unlock();
    while(true)
    {
        //========================================
        mMutex.lock();
        if(mStop)
        {
            mMutex.unlock();
            break;
        }
        mMutex.unlock();
        //========================================

        msleep(10);
        i++;
        //qDebug()<<"i="<<i;
    }
    emit processOK(i);  //give it i as process result
    qDebug()<<"myThread::run exit. id="<<currentThreadId();
}
