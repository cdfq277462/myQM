#include "qualitymonitor.h"
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QApplication>
#include <QtCore>
#include <QtGui>
#include <QDebug>

#include "fft.h"
#include "mythread.h"
#include "parameter.h"

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

class EEparameter : public QObject
{
public:



};

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qualitymonitor w;
    //mythread isr;

    //isr.start();


    w.show();
    return a.exec();
}
