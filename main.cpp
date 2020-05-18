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

//#include <pigpio.h>
#include <time.h>

#define LED		17  //test sig
#define	trig_pin	27 // trigger

//interrupt flag
int flag = 0;
int i = 0;
time_t start_t, end_t;
/*********************************************
void ADinput_ISR(int gpio, int level, uint32_t tick){
    flag = 1;
//	printf("%u\n", tick);
}
**********************************************/

/**********************************************************
test input, delete when publish
***********************************************************
int datain[4096] = {0};
#ifndef FFT_N
#define FFT_N   4096
#endif // FFT_N

#define FILE_NAME "SPG_test.txt"

void read()
{
    FILE *fpr;
//open file
    fpr=fopen(FILE_NAME,"r");
    //fpr=fopen("d:\\SPG_test.txt","r");
//read file
    for (int i = 0; i < FFT_N; i++){
        fscanf(fpr,"%d",&datain[i]);
        //printf("%d\t" , in[i]);
    }
//close file
    fclose(fpr);
}
**********************************************************/



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qualitymonitor w;
    //mythread isr;
/*************************************************
    gpioInitialise();
    gpioSetMode(LED, PI_OUTPUT);
    gpioSetMode(trig_pin, PI_INPUT);
    gpioSetPullUpDown(trig_pin, PI_PUD_UP); //set trig_pin to edge trig
    time_sleep(0.001);
    gpioSetISRFunc(trig_pin, FALLING_EDGE, 10000, ADinput_ISR); //ISR
    //end setup pin mode
    start_t = clock();
**************************************************/
    //isr.start();

    w.show();
    return a.exec();
}
