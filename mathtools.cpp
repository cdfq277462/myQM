#include "mathtools.h"

mathtools::mathtools()
{

}

float mathtools:: A_per(QString current_feedOut_Center, QString DataIn)
{
    int current_feedOut_Center_Num  = QString(current_feedOut_Center).toInt();
    int DataIn_Num                  = QString(DataIn).toInt();

    return (DataIn_Num - current_feedOut_Center_Num) / current_feedOut_Center_Num;
}

float mathtools::CV_per(QString DataIn, QString)
{
    //each 100m do it

}

float mathtools::CV_per1m(QString, QString)
{

}
