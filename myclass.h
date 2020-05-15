#ifndef MYCLASS_H
#define MYCLASS_H

#include <QtCore>

class myClass : public QObject
{
    Q_OBJECT
public:
    myClass();
    struct compx EE(struct compx ,struct compx );
    void FFT(struct compx *);

    void read();
    void write(float* , float* );

    void SPG(int*);
};

#endif // MYCLASS_H
