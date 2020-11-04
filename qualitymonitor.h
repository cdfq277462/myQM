#ifndef QUALITYMONITOR_H
#define QUALITYMONITOR_H

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
    void toSaveData(int);
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

    QVector<double> DetectCenter_L, DetectCenter_R;
    QVector<double> DetectCenter_axixX_L, DetectCenter_axixX_R;
    bool is_DetectCenter;

    int org_ADC_value[3];
    float output_ADC_value_L[3], output_ADC_value_R[3];
    float display_ADC_value_L[3], display_ADC_value_R[3];

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

    void updateAllconfig();




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

    void setup_shiftSchedule();

    void on_comboBox_shifts_currentIndexChanged(const QString &arg1);

    void on_pushButton_shift_clicked();

    void on_pushButton_saveShiftSchedule_clicked();

    void whichShift();

    void on_pushButton_startminu_1_clicked();

    void on_pushButton_startplus_1_clicked();

    void on_pushButton_endminu_1_clicked();

    void on_pushButton_endplus_1_clicked();

    void on_pushButton_startminu_2_clicked();

    void on_pushButton_startplus_2_clicked();

    void on_pushButton_endminu_2_clicked();

    void on_pushButton_endplus_2_clicked();

    void on_pushButton_endminu_3_clicked();

    void on_pushButton_endplus_3_clicked();

    void on_pushButton_startminu_3_clicked();

    void on_pushButton_startplus_3_clicked();

    void on_pushButton_endminu_4_clicked();

    void on_pushButton_endplus_4_clicked();

    void on_pushButton_startminu_4_clicked();

    void on_pushButton_startplus_4_clicked();

    void on_pushButton_parameter_nextpage_clicked();

    void on_pushButton_parameter_prepage_clicked();

    void on_pushButton_hh_plus_clicked();

    void on_pushButton_hh_minus_clicked();

    void on_pushButton_mm_plus_clicked();

    void on_pushButton_mm_minus_clicked();


    void on_pushButton_on_runframe_setoutputcenter_pressed();

    void on_pushButton_on_runframe_setoutputcenter_pressed_3s();



    void on_checkBox_shift_1_clicked();

    void on_checkBox_errorCVper_clicked();

    void on_checkBox_shift_2_clicked();

    void on_checkBox_shift_3_clicked();

    void on_checkBox_shift_4_clicked();

    void on_checkBox_errorAper_clicked();

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
    bool isRunning;

    int timeid_DateTime, timeid_TrigCount, timeid_AlarmofCV, timeid_Alarm, timeid_replotSPG, timeid_AlarmofCV_R, timeid_Alarm_R;
    int timeid_GUI_ADC_Value, timeid_DetectCenter;
    int AlarmFlag, AlarmFlagofCV;
    int timeid_pressed_toStart;

    int outputOffset_L, outputOffset_R;
    // 調整率
    float adjustRate_L, adjustRate_R;
    // 偏移調整
    float offsetAdjust;

    float Filter1, Filter2;

    int set_timeHour, set_timeMinu;

    QVector<int> on_RunFrame_Set_OutputCenter_L, on_RunFrame_Set_OutputCenter_R;


    bool overAper_L, overAper_R;
    bool overCV_per_L, overCV_per_R;

    float onePulseLength;
    QTimer *timer;

    bool eventFilter(QObject *,QEvent *);

    int startshift1, endshift1, startshift2, endshift2, startshift3, endshift3, startshift4, endshift4;

};

#endif // QUALITYMONITOR_H
