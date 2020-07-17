#ifndef QUALITYMONITOR_H
#define QUALITYMONITOR_H

#include "mythread.h"
#include <QWidget>

#include <QtCharts>
#include "qcustomplot.h"

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

    static qualitymonitor *ISR_excute_ptr; //qualitymonitor::qualitymonitor ptr
    void setupParameter();
    void toSaveDate(int);
    ~qualitymonitor();

    mathtools Mymathtool;

    MyTrigger mTrigger;

    QVector<double> mData_L, mData_R, axixX_L, axixX_R;
    QVector<double> SPG_Data, CV_Data;
    QVector<double> CV_1m, CV_path;

    QVector<double> historyData_x, historyData;
    QString DataWrite_L, DataWrite_R;
    QStringList DataWrite_SPG;
    //QByteArray DataWrite_SPG;

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


    void on_pushButton_Search_clicked();

    void on_pushButton_ErrorSig_clicked(QString);

    void on_pushButton_Settiing_clicked();

    void on_pushButton_3_clicked();

    void on_pushButton_5_clicked();

    void on_pushButton_out1offset_clicked();

    void on_pushButton_out2offset_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_OutputCenter_clicked();

    void on_pushButton_replot_clicked();

    void on_pushButton_SPG_clicked();

    void fixed_yAxisRange(QCPRange);
    void fixed_xAxisRange(QCPRange);



    void Setup_History();

    void Read_historyData(QString);

    void Write_newData(int);

    void Setup_HistoryChart();

    void on_reset_clicked();

    void SetErrorTable();

    void set_SPG_Chart();

    void set_listview_historyFile();




    void on_pushbutton_QMenble_clicked();

    void on_listView_historyFile_clicked(const QModelIndex &index);

    void on_comboBox_SideChose_currentIndexChanged(int index);

public slots:
    void timerEvent(QTimerEvent *event);

    void DateTimeSlot();

    void on_Receive_ADval(float);

    void on_Receive_Trig();

    void Set_Graphics_L();
    void Set_Graphics_R();

    void slot();
    void count_ISR_times();

    static void  ADtrig_ISR(int gpio, int level, uint32_t tick);

signals:
    void emit_adc_enable();

    void sig();


private:
    Ui::qualitymonitor *ui;

    QFileSystemModel *History_filemodel;

    QDateTime RunDateTime;

    float AD_value;

    int timeid_DateTime, timeid_Alarm, timeid_TrigCount, timeid_AlarmofCV;
    int AlarmFlag, AlarmFlagofCV;

    bool overAper_L, overAper_R;
    bool overCV_per_L, overCV_per_R;

    QTimer *timer;

};

#endif // QUALITYMONITOR_H
