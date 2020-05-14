#ifndef QUALITYMONITOR_H
#define QUALITYMONITOR_H

#include "mythread.h"
#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class qualitymonitor; }
QT_END_NAMESPACE

class qualitymonitor : public QWidget
{
    Q_OBJECT

public:
    qualitymonitor(QWidget *parent = nullptr);
    void setupParameter();
    void toSaveDate(int);
    MyThread *mThread;
    ~qualitymonitor();

private slots:
    void on_saveButton_clicked();

    void on_Index_clicked();

    void on_LVDT_Button_clicked();

    void on_parameter_Button_clicked();

    void on_test_Button_clicked();

    void on_errorfram_Button_clicked();

    void on_EEtestbutt_clicked();

    void on_saveEEpraButton_clicked();

    void on_pushButton_2_clicked();

public slots:
    void onDateSlot(int);

private:
    Ui::qualitymonitor *ui;
};
#endif // QUALITYMONITOR_H
