#include "mathtools.h"

#define SAMPLE_LENGTH 1000

mathtools::mathtools()
{

}

float mathtools:: A_per(QString current_feedOut_Center, QString DataIn)
{
    float current_feedOut_Center_Num  = QString(current_feedOut_Center).toFloat();
    float DataIn_Num                  = QString(DataIn).toFloat();

    return (current_feedOut_Center_Num - DataIn_Num) / DataIn_Num * 100;
}

float mathtools::CV_per(int *DataIn, QString)
{
    //each 10m do it
    float avg, SD;
    for(int i = 0; i < SAMPLE_LENGTH; i++)
        avg = avg + DataIn[i] / SAMPLE_LENGTH;

    for(int i = 0; i < SAMPLE_LENGTH; i++)
        SD = sqrt(pow(DataIn[i] - avg, 2));

    return SD / SAMPLE_LENGTH;
}

float mathtools::CV_per1m(int *DataIn, QString)
{
    float avg, SD;
    for(int i = 0; i < (SAMPLE_LENGTH / 10); i++)
        avg = avg + DataIn[i] / (SAMPLE_LENGTH / 10);

    for(int i = 0; i < (SAMPLE_LENGTH / 10); i++)
        SD = sqrt(pow(DataIn[i] - avg, 2));

    return SD / (SAMPLE_LENGTH / 10);
}
