#include "mythread.h"
#include <QtCore>

MyThread::MyThread(QObject *parent) :
        QThread(parent)
{

}

void MyThread::run()
{
    for(int i = 1; i < 10000; i++)
    {
        QMutex mutex;
        mutex.lock();
        if(this->Stop) break;
        mutex.unlock();

        emit sig(i);
        this->msleep(100);
    }

}
