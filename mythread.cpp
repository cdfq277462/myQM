#include "mythread.h"
#include <QtCore>
#include <QThread>
#include <QtDebug>

#include "fft.h"
//#include "ad7606.h"
#include <pigpio.h>
#include "mathtools.h"

MyThread::MyThread(QObject *parent) : QObject(parent)
{
    //mathtools mymathtols;
}

void MyThread::Setup(QThread &cThread, int index)
{
    switch (index) {
    case 1:
        connect(&cThread, SIGNAL(started()), this, SLOT(DoWork()));
        break;
    default:
        qDebug() << "Hello World!\n";

        break;
    }
}

void MyThread::DoWork()
{
    //qDebug() << ad7606();
    //qDebug() << "Hello World!\n";

}
