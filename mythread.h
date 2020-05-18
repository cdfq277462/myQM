#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QObject>
#include <QThread>
class MyThread : public QThread
{
public:
    MyThread();

    void run();
    bool Stop;

signals:


public slots:
    //void onDateTrigSig();

};
#endif //MYTHREAD_H
