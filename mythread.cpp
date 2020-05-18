#include "mythread.h"
#include <QtCore>
#include <QThread>

#include "fft.h"
MyThread::MyThread(QObject *parent) :
        QObject(parent)
{

}

void MyThread::DoSetup(QThread &SigTrig)
{
    connect(&SigTrig, SIGNAL(started()), this, SLOT(onDateTrigSig()));
}


void MyThread::onDateTrigSig()
{
   for(int i = 1; i < 1000; i++)
    {
        //qDebug() << i;

/*
        QMutex mutex;
        mutex.lock();
        if(this->Stop) break;
        mutex.unlock();
*/
       //msleep(500);
    }

}

MyADC_ISR::MyADC_ISR(QObject *parent) :
        QThread(parent)
{

}

void MyADC_ISR::run()
{
    while(1)
    {
           //SPG();
    }
}
