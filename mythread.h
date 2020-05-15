#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QObject>
#include <QThread>
class MyThread : public QObject
{
    Q_OBJECT
public:
    explicit MyThread(QObject *parent = 0);
    void DoSetup(QThread &SigTrig);
    bool Stop;

signals:


public slots:
    void onDateTrigSig();

};

#endif // MYTHREAD_H
