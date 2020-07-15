#ifndef MATHTOOLS_H
#define MATHTOOLS_H
#include <QtCore>
#include <QString>

#include <complex>
//include A% CV% CV%1m

class mathtools  : public QObject
{
    Q_OBJECT

public:
    mathtools();

public slots:

    float A_per(QString, float);

    QVector<double> SPG(QVector<double>);

/*
    float CV_per(int*, float);

    float CV_per1m(int*, QString);
*/
};



#endif // MATHTOOLS_H
