#include "qualitymonitor.h"
#include <stdio.h>
#include <stdlib.h>

#include <QCoreApplication>
#include <QApplication>
#include <QtCore>
#include <QtGui>
#include <QDebug>
#include <QThread>

#include "mythread.h"
#include "parameter.h"

#include <pigpio.h>
#include <time.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qualitymonitor w;

    w.show();
    return a.exec();
}
