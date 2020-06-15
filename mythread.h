#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QtCore>
#include <QObject>
#include <QThread>

class MyThread : public QObject
{
    Q_OBJECT

public:
    explicit MyThread(QObject *parent = nullptr);
    void Setup(QThread &cThread, int);

signals:



public slots:
    void DoWork();
    //void onDateTrigSig();

};

class MyTrigger : public QThread
{
    Q_OBJECT

public:
    explicit MyTrigger();
    void run() override;

signals:
    void emit_trig_sig();
};

#endif //MYTHREAD_H
