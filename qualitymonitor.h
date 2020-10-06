#ifndef QUALITYMONITOR_H
#define QUALITYMONITOR_H

#include "mythread.h"
#include <QWidget>

#include "qcustomplot.h"

#include "mathtools.h"
#include "keyboard.h"
#include "adread.h"


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

    ADread *mAD;

    keyboard *mKeyboard;

    QVector<double> axixX_L, axixX_R;
    QVector<double> SPG_Data, CV_Data;
    QVector<double> SPG_Data_R, CV_Data_R;

    QVector<double> CV_1m, CV_path;
    QVector<double> CV_1m_R, CV_path_R;

    QVector<double> oldData_Aper_L, oldData_Aper_R;
    QVector<double> oldData_CV_L, oldData_CV_R;
    double datalenght_L, datalenght_R;

    QVector<double> historyData_x, historyData, historyData_CV;
    QString DataWrite_L, DataWrite_R, DataWrite_CV_L, DataWrite_CV_R;

    QStringList DataWrite_SPG, DataWrite_SPG_R;
    //QByteArray DataWrite_SPG;

    int Feedoutcenter_L, Feedoutcenter_R;

    int whichFrameRequestPassword;

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
    void Read_oldData(int);

    void Write_newData(int);

    void Setup_HistoryChart();

    void on_reset_clicked();

    void SetErrorTable();

    void set_SPG_Chart();

    void set_listview_historyFile();

    void set_Keyboard();

    void RunFrame_Display(float, float, float, float);




    void on_pushbutton_QMenble_clicked();

    void on_listView_historyFile_clicked(const QModelIndex &index);

    void on_comboBox_SideChose_currentIndexChanged(int index);

    void on_pushButton_historysearch_clicked();

    void on_checkBox_A_per_stateChanged(int arg1);

    void on_checkBox_CV_stateChanged(int arg1);

    void on_pushButton_Passwordcancel_clicked();

    void on_pushButton_PasswordOK_clicked();

    void on_pushButton_clicked();

    void on_pushButton_SettingSave_clicked();

    void on_pushButton_centerConfirm_clicked();

    void on_pushButton_startDetect_clicked();

    void on_pushButton_7_clicked();

    void deleteOldHistoryData();

public slots:
    void timerEvent(QTimerEvent *event);

    void DateTimeSlot();

    void on_Receive_ADval();

    void on_Receive_Trig();

    void Set_Graphics_L();
    void Set_Graphics_R();

    void slot();
    void count_ISR_times();

    void on_KeyboardSlot(QKeyEvent*);

    static void  ADtrig_ISR(int gpio, int level, uint32_t tick);
    static void  Running(int gpio, int level, uint32_t tick);

signals:
    void emit_adc_enable();

    void sig();


private:
    Ui::qualitymonitor *ui;

    QFileSystemModel *History_filemodel;

    QString RunDateTime;

    int timeid_DateTime, timeid_TrigCount, timeid_AlarmofCV, timeid_Alarm, timeid_replotSPG;
    int timeid_GUI_ADC_Value;
    int AlarmFlag, AlarmFlagofCV;

    bool overAper_L, overAper_R;
    bool overCV_per_L, overCV_per_R;

    QTimer *timer;

    bool eventFilter(QObject *,QEvent *);


};

#endif // QUALITYMONITOR_H
