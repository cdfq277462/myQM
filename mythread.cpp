#include "mythread.h"
#include <QtCore>
#include <QThread>
#include <QtDebug>

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
    qDebug() << "Nothing to do!";

}

MyTrigger::MyTrigger()
{

}

void MyTrigger::run()
{
    //qDebug() << "mtrig: " << thread()->currentThreadId();
    emit emit_trig_sig();

}
