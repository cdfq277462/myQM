#include "qualitymonitor.h"
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QApplication>
#include <QtCore>
#include <QtGui>
#include <QDebug>
#include <QThread>

#include "fft.h"
#include "mythread.h"
#include "parameter.h"

#include <pigpio.h>
#include <time.h>


#define LED         17  //test sig
#define	trig_pin	27 // trigger

#define ADC_Setup   1


/*
//interrupt flag
int flag = 0;
int i = 0;
time_t start_t, end_t;
*/

/*
void ADtrig_ISR(int gpio, int level, uint32_t tick){

    flag ++;
    //printf("%u\n", flag);
    //MyTrigger mTrigger;
    if(flag == 5){
        end_t = clock();
        qDebug() << "AD read" << difftime(end_t, start_t) ;
        //AD start to read
        //mTrigger.start();
        //mTrigger.wait();
        start_t = clock();
        flag = 0;
    }
}
*/
int main(int argc, char *argv[])
{
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

    QApplication a(argc, argv);
    qualitymonitor w;


    //gpioSetISRFunc(trig_pin, FALLING_EDGE, 0, ADtrig_ISR); //ISR
    //end setup pin mode

    //qDebug() << a.thread()->currentThreadId();

    QThread cThread;
    MyThread AD7606;
    AD7606.Setup(cThread, 1);
    AD7606.moveToThread(&cThread);
    cThread.start();

    //start_t = clock();


    w.show();
    return a.exec();
}
