#ifndef ADREAD_H
#define ADREAD_H

#include <QtCore>
#include <QObject>
#include <QThread>
#include <QDebug>
#include <stdio.h>
#include <stdlib.h>

#include <pigpio.h>

class ADread : public QThread
{
    Q_OBJECT
public:
    ADread();
    void run() override;

signals:
    void emit_AD_value(int);

public slots:
    void ADC_enable();



};

#endif // ADREAD_H
