#include "mathtools.h"

#define SAMPLE_LENGTH 500

mathtools::mathtools()
{

}

float mathtools:: A_per(QString current_feedOut_Center, float DataIn)
{
    float current_feedOut_Center_Num  = QString(current_feedOut_Center).toFloat();

    return (DataIn - current_feedOut_Center_Num) / current_feedOut_Center_Num * 100;
}
/*
float mathtools::CV_per(int *DataIn, float avg)
{
    //each 5m do it

    float SD = 0;
    for(int i = 0; i < SAMPLE_LENGTH; i++)
        avg = avg + DataIn[i] / SAMPLE_LENGTH;

    for(int i = 0; i < SAMPLE_LENGTH; i++)
        SD += sqrt(pow(DataIn[i] - avg, 2));

    return SD / SAMPLE_LENGTH;

}

float mathtools::CV_per1m(int *DataIn, QString)
{
    float avg, SD;
    for(int i = 0; i < (SAMPLE_LENGTH / 5); i++)
        avg = avg + DataIn[i] / (SAMPLE_LENGTH / 5);

    for(int i = 0; i < (SAMPLE_LENGTH / 5); i++)
        SD = sqrt(pow(DataIn[i] - avg, 2));

    return SD / (SAMPLE_LENGTH / 5);
}
*/
