#ifndef MYTHREAD_H
#define MYTHREAD_H

#include <QtCore>
#include <QThread>
class MyThread : public QThread
{
    Q_OBJECT
public:
    explicit MyThread(QObject *parent = 0);
    void run();
    bool Stop;

signals:
    void sig(int);

};

#endif // MYTHREAD_H
