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


//interrupt flag
int flag = 0;
int i = 0;
time_t start_t, end_t;

void ADtrig_ISR(int gpio, int level, uint32_t tick){
    flag ++;
    //printf("%u\n", flag);
    MyTrigger mTrigger;
    if(flag == 5){
        qDebug() << "AD read";
        //AD start to read
        mTrigger.start();
        mTrigger.wait():

        flag = 0;
    }
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qualitymonitor w;

    gpioInitialise();
    gpioSetMode(LED, PI_OUTPUT);
    gpioSetMode(trig_pin, PI_INPUT);
    gpioSetPullUpDown(trig_pin, PI_PUD_UP); //set trig_pin to edge trig
    time_sleep(0.001);
    gpioSetISRFunc(trig_pin, FALLING_EDGE, 0, ADtrig_ISR); //ISR
    //end setup pin mode

    start_t = clock();
    qDebug() << a.thread()->currentThreadId();

    QThread cThread;
    MyThread AD7606;
    AD7606.Setup(cThread, 1);
    AD7606.moveToThread(&cThread);
    cThread.start();



    w.show();
    return a.exec();
}
