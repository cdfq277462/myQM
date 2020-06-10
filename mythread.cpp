#include "mythread.h"
#include <QtCore>
#include <QThread>
#include <QtDebug>

#include "fft.h"
#include "ad7606.h"
//#include <pigpio.h>
#include "mathtools.h"

MyThread::MyThread()
{
    mathtools mymathtols;
    ad7606();
}

void MyThread::run()
{
        //qDebug() << "My Thread run\n" << thread()->currentThreadId();


}
