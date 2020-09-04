#include "readerThread.h"

#include <iostream>
#include <QFile>

#include "alsaCaptureAudio.h"


readerThread::readerThread()
    : mStop(false), mPause(false), mFilePath("") {
    std::cout << "readerThread constructed: threadid = " << QThread::currentThreadId() << std::endl;
}

readerThread::~readerThread() {
    std::cout << "readerThread decstructed: threadid = " << QThread::currentThreadId() << std::endl;
}

void readerThread::stop() {
    std::cout <<"readerThread::stop call. id=" << currentThreadId() << std::endl;
    mMutex.lock();
    mStop = true;
    mMutex.unlock();
}

void readerThread::pause(bool bpause) {
    std::cout <<"readerThread::pause call. id=" << currentThreadId() << std::endl;
    mMutex.lock();
    mPause = bpause;
    mMutex.unlock();
}

void readerThread::setFilePath(QString path) {
    mFilePath = path;
    QFile file(mFilePath);
    if (!file.exists()) {    //
        std::cout << "file not exist : " << mFilePath.toStdString() << std::endl;
    }
    std::cout << "path set : " << mFilePath.toStdString() << std::endl;
}

void readerThread::run() {
    std::cout << "readerThread run: threadid = " << QThread::currentThreadId() << std::endl;
    FILE *pFile = fopen("test.pcm", "wb");
    alsaCaptureAudio acap;

    while (true) {
        mMutex.lock();
        if(mStop) {
            mMutex.unlock();
            break;
        } else if (mPause) {
            mMutex.unlock();
            QThread::msleep(10);
            continue;
        }
        mMutex.unlock();

        audioData adata;
        acap.readFrames(adata);
        std::cout << "readerThread runing: threadid = " << QThread::currentThreadId()<<",read "<< adata.len <<" bytes data"<< std::endl;
        int rc = fwrite(adata.buf, sizeof(char), adata.len, pFile);
        if (rc != adata.len) {
            std::cout << "short write: wrote "<<rc<<" bytes." << std::endl;
        }

        QThread::msleep(100);
    }
    std::cout << "readerThread exit: threadid = " << QThread::currentThreadId() << std::endl;
    fclose(pFile);
    return;
}

