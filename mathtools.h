#ifndef MATHTOOLS_H
#define MATHTOOLS_H
#include <QtCore>
#include <QString>

//include A% CV% CV%1m

class mathtools  : public QObject
{
    Q_OBJECT

public:
    mathtools();

    float A_per(QString, QString);

    float CV_per(QString, QString);

    float CV_per1m(QString, QString);
};

#endif // MATHTOOLS_H
