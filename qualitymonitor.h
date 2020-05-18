#ifndef QUALITYMONITOR_H
#define QUALITYMONITOR_H

#include "mythread.h"
#include <QWidget>

#include <QtCharts>


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
    ~qualitymonitor();

private slots:
    void on_saveButton_clicked();

    void on_IndexButton_clicked();

    void on_LVDT_Button_clicked();

    void on_parameter_Button_clicked();

    void on_test_Button_clicked();

    void on_errorfram_Button_clicked();

    void on_EEtestbutt_clicked();

    void on_saveEEpraButton_clicked();

    void on_pushButton_2_clicked();

    void SetErrorTable();

    void on_pushButton_Search_clicked();

    void on_pushButton_ErrorSig_clicked();

    void on_pushButton_Settiing_clicked();

    void Setup_GraphicsView();

    //QLineSeries DataInput(int);

public slots:
    void DateTimeSlot();

private:
    Ui::qualitymonitor *ui;
    QTimer *timer;
    QGraphicsScene *scene;
};
#endif // QUALITYMONITOR_H
