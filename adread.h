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

    static ADread *ISR_excute_ptr;

    int ADC_value, ADC_value_R;
    //int ADC_value[2], ADC_value_R[2];



signals:
    void emit_AD_value(int);

public slots:
    void ADC_enable();

    void AD7606_IOSet();

    void AD7606_RST();

    void AD7606_startConv();

    uint readADCvalue();

    static void BUSY_ISR(int gpio, int level, uint32_t tick);

};



#endif // ADREAD_H
