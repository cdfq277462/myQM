#ifndef PARAMETER_H
#define PARAMETER_H
#include <QtCore>
#include <QString>

//參數設定
class parameter : public QObject
{
    Q_OBJECT
public:
    parameter();

    int     L_feedoutcenter();
    int     L_outputoffset();
    int     L_adjustrate();
    float   L_outputweight();
    float   L_limit_Aper();
    float   L_limit_CVper();

    int     R_feedoutcenter();
    int     R_outputoffset();
    int     R_adjustrate();
    float   R_outputweight();
    float   R_limit_Aper();
    float   R_limit_CVper();

    QString TrantoNumberType(QString);

    float   PulseLength();
    int     Filter_1();
    float   Filter_2();
    int     OffsetAdjust();

    int     Out1_Offset();
    int     Out2_Offset();

    int UTC();
    QString Password();

    QString shiftschedule();


    QString   Read(QString Filename, int);
    void    Write(QString Filename, QString);

};

#endif // PARAMETER_H
