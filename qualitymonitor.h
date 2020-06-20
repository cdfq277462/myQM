#ifndef QUALITYMONITOR_H
#define QUALITYMONITOR_H

#include "mythread.h"
#include <QWidget>

#include <QtCharts>

#include "mathtools.h"
using namespace QtCharts;



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

    QFileSystemWatcher watcher;
    mathtools Mymathtool;


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

    //QLineSeries load_preData();


    void on_pushButton_3_clicked();

    void Setup_History();

    void on_pushButton_5_clicked();

    void on_pushButton_out1offset_clicked();

    void on_pushButton_out2offset_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_OutputCenter_clicked();

    void Read_oldData();

    void count_ISR_times();

public slots:
    void DateTimeSlot();

    void on_Receive_ADval(float);

    void on_Receive_Trig();

    void ADC_ISR(QString);

    void Set_GraphicsView();

signals:
    void emit_adc_enable();

private:
    Ui::qualitymonitor *ui;
    QTimer *timer;
    QGraphicsScene *scene_L;
    QGraphicsScene *scene_R;

    QSplineSeries *series_L;
    QSplineSeries *series_R;
    QList<QSplineSeries *> m_serieslist;

};
/************************************************
class MyTrigger : public QThread
{
    Q_OBJECT

public:
    explicit MyTrigger();
    void run() override;

signals:
    void emit_trig_sig();
};
**************************************************/
#endif // QUALITYMONITOR_H
