/***************************************************************************
**                                                                        **
**  QCustomPlot, an easy to use, modern plotting widget for Qt            **
**  Copyright (C) 2011-2018 Emanuel Eichhammer                            **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author:                                    **
**  Website/Contact:                         **
**             Date:                                              **
**          Version:                                                **
****************************************************************************/

#include "qualitymonitor.h"
#include "ui_qualitymonitor.h"

#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QTimer>
#include <QDateTime>

#include "parameter.h"
#include "adread.h"
#include "pigpio.h"
#include <time.h>
#include "qcustomplot.h"

//parameter read index
#define  normalParameter    1
#define  EEParameter        2
#define  ShiftSchedule      3

//data index
#define LeftSide      0
#define RightSide     1
#define LeftSide_SPG  2
#define RightSide_SPG 3
#define LeftSide_CV   4
#define RightSide_CV  5


#define	trig_pin	27 // trigger
#define run_signal  22 // run signal
#define Stop_signal 17
#define AL_enable   18

//sample length
//unit = cm
#define SPG_SampleLength    4096

#define DetectCenter_Length 10000

/*
//combobox index
#define LeftSide    0
#define RightSide   1
*/

//interrupt flag
int flag = 0;
//count interrupt excute times
int isr_count_tick = 0;

time_t start_t, end_t;

qualitymonitor* qualitymonitor::ISR_excute_ptr = nullptr; //initialize ptr

qualitymonitor::qualitymonitor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::qualitymonitor)
{

    ui->setupUi(this);
    QWidget::showFullScreen();
    this->setCursor(Qt::BlankCursor);   //hide mouse
    ISR_excute_ptr = this;              //qualitymonitor::qualitymonitor ptr
    time_sleep(1);

    //set QM enable button to green
    ui->pushbutton_QMenble->setStyleSheet("background : green ; color : white");
    //ui->pushbutton_QMenble->setStyleSheet("color : white");

    timeid_DateTime = startTimer(500);
    //initial RunDateTime
    RunDateTime = QDate::currentDate().toString("yyyy-MM-dd");
    ui->dateEdit->setDate(QDate().currentDate());
    set_timeHour = 0;
    set_timeMinu = 0;
    system("timedatectl set-ntp no");

//setup parameter
    setup_shiftSchedule();
    setupParameter();
    SetErrorTable();

//set runframe on toplevel
    ui->runframe->raise();
    ui->Dateframe->raise();

    Setup_History();
    set_listview_historyFile();

    AlarmFlag = 0;
    AlarmFlagofCV = 0;


    on_pushButton_Search_clicked();
    on_pushButton_historysearch_clicked();

//Setup_Graphics;
    Set_Graphics_L();
    Set_Graphics_R();
    Setup_HistoryChart();
    ui->HistoryChart->installEventFilter(this);
    ui->frame_SPG->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint   \
                                 | Qt::Tool | Qt::X11BypassWindowManagerHint);
    ui->frame_SPG->setCursor(Qt::BlankCursor);
//GPIO setup
    gpioInitialise();
    gpioSetMode(trig_pin, PI_INPUT);            //ISR pin
    gpioSetMode(run_signal, PI_INPUT);            //Run ISR pin
    gpioSetPullUpDown(trig_pin, PI_PUD_DOWN);
    gpioSetPullUpDown(run_signal, PI_PUD_DOWN);

    gpioSetMode(Stop_signal, PI_OUTPUT);        //stop signal
    gpioSetMode(AL_enable, PI_OUTPUT);          //AL enable

    time_sleep(0.001);

    gpioSetISRFunc(trig_pin, FALLING_EDGE, 0, ADtrig_ISR); //ISR
    gpioSetISRFunc(run_signal, EITHER_EDGE, 0, Running);

    // test frame flags
    gpioWrite(Stop_signal, PI_LOW);
    ui->label_stopFlag->setText(QString::number(0));

    //connect(&mTrigger, SIGNAL(emit_trig_sig()), this, SLOT(slot()));
    connect(this, SIGNAL(sig()), this, SLOT(slot()));

    //connect(this, SIGNAL(on_trig(int)), this, SLOT(speed_cal(int)));

/**************  ADC Read  ***************************************/
    //ADread *mAD = new ADread;
    mAD = new ADread;
    //connect(mAD, SIGNAL(emit_AD_value(int)), this, SLOT(on_Receive_ADval(int)));
    connect(this, SIGNAL(emit_adc_enable()), mAD, SLOT(ADC_enable()));
    mAD->AD7606_RST();
    mAD->start();
    timeid_GUI_ADC_Value = startTimer(300);
    // init AD value array
    //org_ADC_value[3] = {0};
    //output_ADC_value[] = {0};
    //pulselength = ui->PulseLength->text().toDouble();
/*****************************************************************/
    time_sleep(0.1);

    //counter trig's timer
    timeid_TrigCount = startTimer(1000);

    //set password echo to ******
    ui->lineEdit_password->setEchoMode(QLineEdit::Password);
    ui->frame_password->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint   \
                                       | Qt::Tool | Qt::X11BypassWindowManagerHint);
    ui->frame_password->hide();
    ui->frame_password->setCursor(Qt::BlankCursor);


    is_DetectCenter = false;

    //creat keyboard dialog
    set_Keyboard();
}
void qualitymonitor::Running(int gpio, int level, uint32_t tick)
{
    if(level == 0)
    {
        //change to low (a falling edge)
        //stop
        qDebug() << "stop";
    }
    else if(level == 1)
    {
        //change to high (a rising edge)
        //run
        qDebug() << "run";
        ISR_excute_ptr->RunDateTime = QDateTime().currentDateTime().toString("yyyy-MM-dd");
        //qDebug() << ISR_excute_ptr->RunDateTime;

    }
    //qDebug() << gpio;
}
void qualitymonitor::ADtrig_ISR(int gpio, int level, uint32_t tick)
{

    //flag ++;
    isr_count_tick++;
    //read AD
    emit ISR_excute_ptr->sig();
/*
    if(flag == 5){
        //AD start to read
        flag = 0;
    }
*/
}

qualitymonitor::~qualitymonitor()
{
    gpioTerminate();
    delete ui;
}



void qualitymonitor::on_KeyboardSlot(QKeyEvent *keyPressEvent)
{
    //emit str to LineEdit
    if(ui->frame_password->isVisible())
        QGuiApplication::postEvent(ui->lineEdit_PasswordInput, keyPressEvent);
    else
        QGuiApplication::postEvent(QWidget::focusWidget(), keyPressEvent);
}

void qualitymonitor::set_Keyboard()
{
    //creat keyboard dialog
    mKeyboard = new keyboard(this);
    connect(mKeyboard, SIGNAL(KeyboardSig(QKeyEvent*)), this, SLOT(on_KeyboardSlot(QKeyEvent *)));
    mKeyboard->setWindowFlags(Qt::WindowDoesNotAcceptFocus |
                              Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);

    //install all LineEdit to eventFilter
    QList<QLineEdit*> AllLineEdit = this->findChildren<QLineEdit*>();
    foreach(QLineEdit *itm, AllLineEdit)
    {
        if(!itm->objectName().contains("spinbox"))
            itm->installEventFilter(this);
    }
}




bool qualitymonitor::eventFilter(QObject *watched,QEvent *event)
{
    //history chart rst
    if(watched == ui->HistoryChart)
    {
        if (event->type()==QEvent::MouseButtonDblClick){
            ui->HistoryChart->xAxis->rescale();
            //qDebug() << "hello";
        }
    }
    QString LineEditClassName = "QLineEdit";
    if(watched->metaObject()->className() == LineEditClassName && event->type() == QEvent::MouseButtonRelease)
    {
        //click LineEdit
        //QRect rect = QWidget::focusWidget()->rect();

        QRect rect;
        QPoint pt = QWidget::focusWidget()->pos();


        if(!ui->frame_password->isVisible()){
            rect = QWidget::focusWidget()->geometry();
            pt = QWidget::focusWidget()->mapToGlobal(QPoint(0, 0));
        }
        else{
            rect = ui->frame_password->geometry();
            pt = ui->frame_password->mapToGlobal(QPoint(0, 0));
        }
        mKeyboard->showKeyboard(pt, rect);        
    }
    else if(watched->metaObject()->className() == LineEditClassName && event->type() == QEvent::FocusOut)
        mKeyboard->hideKeyboard();

    return QWidget::eventFilter(watched,event);     //return event to top
}

void qualitymonitor::on_Receive_ADval()
{
    int AD_value = mAD->readADCvalue();
    uint AD_value_L = (AD_value >> 16 & 0x7fff);
    uint AD_value_R = (AD_value & 0x00007fff);
    ui->out1_pos->setText(QString::number(AD_value_L * 0.305, 'f', 0));
    ui->out2_pos->setText(QString::number(AD_value_R * 0.305, 'f', 0));

    //set LVDT button color & enable
    if(ui->out1_pos->text().toInt() > 1200 && ui->out1_pos->text().toInt() < 2000)
    {
        ui->pushButton_out1offset->setStyleSheet("color : green");
        ui->pushButton_out1offset->setEnabled(true);
    }
    else
    {
        ui->pushButton_out1offset->setStyleSheet("color : red");
        ui->pushButton_out1offset->setEnabled(false);
    }

    if(ui->out2_pos->text().toInt() > 1200 && ui->out2_pos->text().toInt() < 2000)
    {
        ui->pushButton_out2offset->setStyleSheet("color : green");
        ui->pushButton_out2offset->setEnabled(true);
    }
    else
    {
        ui->pushButton_out2offset->setStyleSheet("color : red");
        ui->pushButton_out2offset->setEnabled(false);
    }

    // set testframe ADC value
    // AD7606 each LSB = 0.000305 uV
    // add LVDT offset (201026)
    ui->test_inputL->setText(QString::number(AD_value_L * 0.305 * adjustRate_L -outputOffset_L, 'f', 0) + "\t mV");
    ui->test_inputR->setText(QString::number(AD_value_R * 0.305 * adjustRate_R -outputOffset_R, 'f', 0) + "\t mV");

    //AD_L = QString().setNum(AD_L.toInt());
    ui->label_Binary_L->setText(QString("%1").arg(int(AD_value_L -outputOffset_L /0.305 /adjustRate_L), 16, 2, QLatin1Char('0')));
    ui->label_Binary_R->setText(QString("%1").arg(int(AD_value_R -outputOffset_R /0.305 /adjustRate_R), 16, 2, QLatin1Char('0')));

    ui->label_testTranscodeL->setText(QString::number(int(AD_value_L -outputOffset_L /0.305 /adjustRate_L)));
    ui->label_testTranscodeR->setText(QString::number(int(AD_value_R -outputOffset_R /0.305 /adjustRate_R)));

    //ui->test_inputL->setText(QString::number(AD_value_L* 3.093* 2/ 4096, 'f',2));
    //ui->test_inputR->setText(QString::number(AD_value_R* 3.093* 2/ 4096, 'f',2));

}
void qualitymonitor::on_Receive_Trig()
{   //prepare to delete
    //Set_GraphicsView();
    //qDebug() << "on_Receive_Trig :" << thread()->currentThreadId();
}

void qualitymonitor::RunFrame_Display(float AD_value_L, float AD_value_R, float SD_L, float SD_R)
{

    //set runframe A%
    ui->label_L_A_per->setText("A%: "+QString::number(Mymathtool.A_per(Feedoutcenter_L, AD_value_L), 'f', 2)+" %");
    ui->label_R_A_per->setText("A%: "+QString::number(Mymathtool.A_per(Feedoutcenter_R, AD_value_R), 'f', 2)+" %");
    ui->label_L_CV1m->setText("1mCV%: " +QString::number(SD_L, 'f', 2) + " %");
    ui->label_R_CV1m->setText("1mCV%: " +QString::number(SD_R, 'f', 2) + " %");

    //change runframe A% text color
    if(qAbs(Mymathtool.A_per(Feedoutcenter_L, AD_value_L)) > ui->L_limit_Aper->text().toFloat()*0.65)
    {
        if(qAbs(Mymathtool.A_per(Feedoutcenter_L, AD_value_L)) > ui->L_limit_Aper->text().toFloat())
            ui->label_L_A_per->setStyleSheet("color : red ; font-size : 15px");
        else
            ui->label_L_A_per->setStyleSheet("color : yellow ; font-size : 15px");
    } // end if
    else
        ui->label_L_A_per->setStyleSheet("color : green ; font-size : 15px");

    if(qAbs(Mymathtool.A_per(Feedoutcenter_R, AD_value_R)) > ui->R_limit_Aper->text().toFloat()*0.65)
    {
        if(qAbs(Mymathtool.A_per(Feedoutcenter_R, AD_value_R)) > ui->R_limit_Aper->text().toFloat())
            ui->label_R_A_per->setStyleSheet("color : red ; font-size : 15px");
        else
            ui->label_R_A_per->setStyleSheet("color : yellow ; font-size : 15px");
    }
    else
        ui->label_R_A_per->setStyleSheet("color : green ; font-size : 15px");
}

void qualitymonitor::updateAllconfig()
{
    // config LVDT output offset
    outputOffset_L = ui->L_outputoffset->text().toInt();
    outputOffset_R = ui->R_outputoffset->text().toInt();

    // config adjust rate
    adjustRate_L = ui->L_adjustrate->text().toFloat() / 100;
    adjustRate_R = ui->R_adjustrate->text().toFloat() / 100;

    offsetAdjust = (ui->OffsetAdjust->text().toFloat() +1) / 100;

    //qDebug() << adjustRate_L;
    Filter1 = (ui->Filter_1->text().toFloat() +1)/ 100;
    Filter2 = (ui->Filter_2->text().toFloat() +1)/ 1000;

    //
    //onePulseLength = ui->PulseLength->text().toFloat();


    QString MachineType = ui->comboBox_machineType->currentText();
    if(MachineType == "DV2")
        onePulseLength = 5.00692;
    else if(MachineType == "HSD - 961")
        onePulseLength = 2.00692;

    //qDebug() << onePulseLength;
}
void qualitymonitor::slot()
{
    //each trig do once
    //
    /************************************************************
     *
     *
     *
     * ***********************************************************/

    int AD_value = mAD->readADCvalue();

    org_ADC_value[2] = org_ADC_value[1];
    org_ADC_value[1] = org_ADC_value[0];
    org_ADC_value[0] = AD_value;

    // output_ADC_value is none-offset-adjust
    output_ADC_value_L[2] = output_ADC_value_L[1];
    output_ADC_value_L[1] = output_ADC_value_L[0];
    display_ADC_value_L[2] = display_ADC_value_L[1];
    display_ADC_value_L[1] = display_ADC_value_L[0];
    //output_ADC_value_L[0] = Filter1 * ((org_ADC_value[0] >> 16 & 0x7fff)) + (1 - Filter1) * output_ADC_value_L[1];
    // trans output to mV
    float noneFilterInput_L = (((org_ADC_value[0] >> 16) & 0x7fff) * 0.305);
    // add LVDT offset, adjust rate
    noneFilterInput_L = (noneFilterInput_L - outputOffset_L) * adjustRate_L;
    // low-pass filter,
    output_ADC_value_L[0] = ((noneFilterInput_L - output_ADC_value_L[1]) * Filter1 + output_ADC_value_L[1]);
    display_ADC_value_L[0] = (noneFilterInput_L - display_ADC_value_L[1]) * Filter2 + display_ADC_value_L[1];
    // add offset adjust
    display_ADC_value_L[0] = (display_ADC_value_L[0] - Feedoutcenter_L) *offsetAdjust + Feedoutcenter_L;

    // output_ADC_value is none-offset-adjust
    output_ADC_value_R[2] = output_ADC_value_R[1];
    output_ADC_value_R[1] = output_ADC_value_R[0];
    display_ADC_value_R[2] = display_ADC_value_R[1];
    display_ADC_value_R[1] = display_ADC_value_R[0];
    // trans output to mV
    float noneFilterInput_R = (org_ADC_value[0] & 0x7fff) * 0.305;
    // add LVDT offset, add adjust rate
    noneFilterInput_R = (noneFilterInput_R - outputOffset_R) * adjustRate_R;
    // low-pass filter
    output_ADC_value_R[0] = ((noneFilterInput_R - output_ADC_value_R[1]) * Filter1 + output_ADC_value_R[1]);
    display_ADC_value_R[0] = (noneFilterInput_R - display_ADC_value_R[1]) * Filter2 + display_ADC_value_R[1];
     // add offset adjust
    display_ADC_value_R[0] = (display_ADC_value_R[0] - Feedoutcenter_R) *offsetAdjust + Feedoutcenter_R;

    //float AD_value_L = AD_value >> 16 & 0x7fff;
    //float AD_value_R = AD_value & 0x00007fff;

    float AD_value_L = output_ADC_value_L[0];
    float AD_value_R = output_ADC_value_R[0];

    //qDebug() << AD_value;
    static float avg_AD_value_1cm = 0;
    static float avg_AD_value_forGUI = 0;
    static float avg_AD_value_forWrite = 0;

    static float avg_AD_value_1cm_R = 0;
    static float avg_AD_value_forGUI_R = 0;
    static float avg_AD_value_forWrite_R = 0;

    static int DetectCenter_times = 0;
    static int avg_DetectCenter_L = 0;
    static int avg_DetectCenter_R = 0;


    //SampleTimes is count trig times
    //static int SampleTimes = 0;

    //static double datalenght_L = ;
    //static double datalenght_R = ui->Chart_R->xAxis->range().upper;

    static float avg_CV1m = 0;
    static float avg_CV1m_R = 0;

    static double SampleTimes[3] = {0};
    /* *********************************
     * SampleTimes[0] = SPG's flag
     * SampleTimes[1] = GUI's flag(A% CV1m)
     * SampleTimes[2] = A% CV 100m datawrite
     * *********************************/

    int OneCentimeterSampleTimes =0;
    if(onePulseLength != 0)
        OneCentimeterSampleTimes =  10 / onePulseLength; //unit = mm // (how many sample times / 1cm)

    //GUI_SampleLength = 100cm
    //GUI A% update every 100cm
    //GUI CV1m update every 50m
    int GUI_SampleLength = 100 ;

    //Datawrite_SampleLength = 100m
    int Datawrite_SampleLength = 10000 ;
/*
    ui->label_test1->setNum(OneCentimeterSampleTimes);
    ui->label_test2->setNum(GUI_SampleLength);
    ui->label_test3->setNum(Datawrite_SampleLength);
*/

    //avg_AD_value_forGUI is used to calculating average of A% & CV% per SampleLength
    avg_AD_value_1cm      += AD_value_L / OneCentimeterSampleTimes;
    avg_AD_value_forGUI   += AD_value_L / GUI_SampleLength / OneCentimeterSampleTimes;
    avg_AD_value_forWrite += display_ADC_value_L[0] / Datawrite_SampleLength / OneCentimeterSampleTimes;

    avg_AD_value_1cm_R      += AD_value_R / OneCentimeterSampleTimes;
    avg_AD_value_forGUI_R   += AD_value_R / GUI_SampleLength / OneCentimeterSampleTimes;
    avg_AD_value_forWrite_R += display_ADC_value_R[0] / Datawrite_SampleLength / OneCentimeterSampleTimes;

    //qDebug() << avg_AD_value_1cm, avg_AD_value_forGUI;

    SampleTimes[0]++;
    //SampleTimes[1]++;
    //SampleTimes[2]++;


    bool SPG_Flag       =   SampleTimes[0] == OneCentimeterSampleTimes;
    //bool GUI_Flag       =   SampleTimes[1] == GUI_SampleLength;
    //bool DataWrite_Flag =   SampleTimes[2] == Datawrite_SampleLength;



    //keep SPG_Data length always be SPG_SampleLength(4096)
    //SPG_Data is used to cal fft
    //DataWrite_SPG is used to store
    if(SPG_Data.length() >= SPG_SampleLength){
        SPG_Data.removeFirst();
        DataWrite_SPG.removeFirst();
        // Write_newData(Write_SPG);
    }
    if(SPG_Data_R.length() >= SPG_SampleLength){
        SPG_Data_R.removeFirst();
        DataWrite_SPG_R.removeFirst();
        // Write_newData(Write_SPG);
    }

    if(SPG_Flag)
    {
        //qDebug() << avg_AD_value_1cm;
        /***********************************************************
         * Center detect
         * *********************************************************/

        if(is_DetectCenter)
        {
            DetectCenter_times ++;

            //DetectCenter_L.append(AD_value_L);
            //DetectCenter_R.append(AD_value_R);
            //avg_DetectCenter_L += avg_AD_value_1cm / DetectCenter_Length;
            //avg_DetectCenter_R += avg_AD_value_1cm_R / DetectCenter_Length;

            avg_DetectCenter_L += avg_AD_value_1cm;
            avg_DetectCenter_R += avg_AD_value_1cm_R;

            //avg_DetectCenter_R += output_ADC_value_R[0] / DetectCenter_Length;

            ui->Chart_DetectCenter_L->graph(0)->addData(DetectCenter_times, avg_AD_value_1cm);
            ui->Chart_DetectCenter_R->graph(0)->addData(DetectCenter_times, avg_AD_value_1cm_R);

            //ui->label_2->setText(QString::number(DetectCenter_times));

            if(DetectCenter_times == DetectCenter_Length)
            {
                //ui->label_center_L_test->setText(QString::number(int(avg_DetectCenter_L)));
                //ui->label_center_R_test->setText(QString::number(int(avg_DetectCenter_R)));
                ui->label_center_L_test->setText(QString::number((avg_DetectCenter_L / DetectCenter_Length)));
                ui->label_center_R_test->setText(QString::number((avg_DetectCenter_R / DetectCenter_Length)));

                //ui->Chart_DetectCenter_L->yAxis->rescale();
                //ui->Chart_DetectCenter_R->yAxis->rescale();
                //ui->Chart_DetectCenter_L->replot();
                //ui->Chart_DetectCenter_R->replot();
                // clear chart
                ui->Chart_DetectCenter_L->graph(0)->data()->clear();
                ui->Chart_DetectCenter_R->graph(0)->data()->clear();
                //qDebug() << DetectCenter_L.length();
                //DetectCenter_L.clear();
                //DetectCenter_R.clear();
                // reset all conditions
                DetectCenter_times = 0;
                avg_DetectCenter_L = 0;
                avg_DetectCenter_R = 0;


                ui->pushButton_startDetect->setText("Done!");

                // kill timer for detect center
                this->killTimer(timeid_DetectCenter);
                is_DetectCenter = false;
            }
        }
        /***********************************************************
         * Center detect end
         * *********************************************************/

        //every 1cm store AD_value
        SPG_Data.append(avg_AD_value_1cm);
        SPG_Data_R.append(avg_AD_value_1cm_R);        



        DataWrite_SPG.append(QString::number(avg_AD_value_1cm));
        DataWrite_SPG_R.append(QString::number(avg_AD_value_1cm_R));



        /************test******************/
        //ui->label_test1->setNum(avg_AD_value_1cm);
        //ui->label_test1R->setNum(avg_AD_value_1cm_R);
        //ui->label_2->setNum(output_ADC_value[0]);
        /**********************************/

        avg_AD_value_1cm = 0;   //clear avg_AD_value_1cm
        avg_AD_value_1cm_R = 0;   //clear avg_AD_value_1cm_R

        //qDebug() << SampleTimes[0];
        SampleTimes[0] = 0; //reset flag

        SampleTimes[1]++;
        SampleTimes[2]++;

        bool GUI_Flag = SampleTimes[1] == GUI_SampleLength;

        if(GUI_Flag)
        {
            //every 100cm do once
            float A_per = (avg_AD_value_forGUI - Feedoutcenter_L) / Feedoutcenter_L *100;
            float A_per_R = (avg_AD_value_forGUI_R - Feedoutcenter_R) / Feedoutcenter_R *100;

            CV_1m.append(avg_AD_value_forGUI);
            CV_1m_R.append(avg_AD_value_forGUI_R);

            static float avg_AD_value_forCV = 0;
            static float avg_AD_value_forCV_R = 0;
            avg_AD_value_forCV += avg_AD_value_forGUI /50;
            avg_AD_value_forCV_R += avg_AD_value_forGUI_R /50;

            //qDebug() << A_per;
            //qDebug() << CV_1m.length();
            float SD = 0;
            float SD_R = 0;
            if(CV_1m.length() == 50 && CV_1m_R.length() == 50)
            {
                // every 50m do once
                foreach(double CV_number, CV_1m)
                    SD += pow(CV_number - avg_AD_value_forCV , 2);

                foreach(double CV_number, CV_1m_R)
                    SD_R += pow(CV_number - avg_AD_value_forCV_R , 2);

                SD = sqrt(SD / (CV_1m.length() - 1)) / avg_AD_value_forCV *100;
                SD_R = sqrt(SD_R / (CV_1m_R.length() - 1)) / avg_AD_value_forCV_R *100;

                RunFrame_Display(AD_value_L, AD_value_R \
                                 , SD, SD_R);

                CV_1m.clear();
                CV_1m_R.clear();

                avg_AD_value_forCV = 0;
                avg_AD_value_forCV_R = 0;

                // 50cm *20 = 100m
                // for data write CV
                avg_CV1m += SD /20;
                avg_CV1m_R += SD_R /20;
            }



            /************test******************/
            ui->label_test2->setText(QString::number(avg_AD_value_forGUI, 'f', 0));
            ui->label_test2R->setText(QString::number(avg_AD_value_forGUI_R, 'f', 0));
            /**********************************/


            //Over A% limit more than 3s, emit STOP sig.
            overAper_L = qAbs(A_per) >= ui->L_limit_Aper->text().toFloat();
            overAper_R = qAbs(A_per_R) >= ui->R_limit_Aper->text().toFloat();

            //qDebug() << overAper;
            if(overAper_L || overAper_R)
                if(AlarmFlag == 0)
                {
                    timeid_Alarm = startTimer(10000);
                    AlarmFlag = 1;
                    //qDebug() << "alarm";
                }
            /*
            if(overAper_R)
                if(AlarmFlag == 0)
                {
                    timeid_Alarm_R = startTimer(3000);
                    AlarmFlag = 1;
                    //qDebug() << "alarm";
                }
            */
            //Over 1mCV% limit more than 5s, emit STOP sig.
            overCV_per_L = SD > ui->L_limit_CVper->text().toFloat();
            overCV_per_R = SD_R > ui->R_limit_CVper->text().toFloat();

            if(overCV_per_L || overCV_per_R)
                if(AlarmFlagofCV == 0)
                {
                    timeid_AlarmofCV = startTimer(10000);
                    AlarmFlagofCV = 1;
                }
            /*
            if(overCV_per_R)
                if(AlarmFlagofCV == 0)
                {
                    timeid_AlarmofCV_R = startTimer(5000);
                    AlarmFlagofCV = 1;
                }
            */

            avg_AD_value_forGUI = 0;    //clear avg_AD_value_forGUI
            avg_AD_value_forGUI_R = 0;    //clear avg_AD_value_forGUI

            SampleTimes[1] = 0; //reset flag
            bool DataWrite_Flag =   SampleTimes[2] == Datawrite_SampleLength;


            if(DataWrite_Flag)
            {
                A_per = (avg_AD_value_forWrite - Feedoutcenter_L) / Feedoutcenter_L *100;
                A_per_R = (avg_AD_value_forWrite_R - Feedoutcenter_R) / Feedoutcenter_R *100;

                if(on_RunFrame_Set_OutputCenter_L.length() > 5)
                    on_RunFrame_Set_OutputCenter_L.removeFirst();
                else
                    on_RunFrame_Set_OutputCenter_L.append(avg_AD_value_forWrite);
                if(on_RunFrame_Set_OutputCenter_R.length() > 5)
                    on_RunFrame_Set_OutputCenter_R.removeFirst();
                else
                    on_RunFrame_Set_OutputCenter_R.append(avg_AD_value_forWrite_R);
                //set Data to be Writed, and to be plot
                /************test******************/
                ui->label_test3->setText(QString::number(avg_AD_value_forWrite, 'f', 0));
                ui->label_test3R->setText(QString::number(avg_AD_value_forWrite_R, 'f', 0));
                /**********************************/

                //datalenght unit is kilometer, every 0.1km record data
                datalenght_L += 0.1;
                datalenght_R += 0.1;

                //datalenght_L += Datawrite_SampleLength / 100;
                //datalenght_R += Datawrite_SampleLength / 100;




                ui->Chart_L->graph(0)->addData(datalenght_L, A_per);
                ui->Chart_L->graph(1)->addData(datalenght_L, avg_CV1m);

                ui->Chart_R->graph(0)->addData(datalenght_R, A_per_R);
                ui->Chart_R->graph(1)->addData(datalenght_R, avg_CV1m_R);

                //append new data
                //DataWrite_L.append(QString::number(avg_AD_value_forWrite)).append("\n");
                DataWrite_L.append(QString::number(A_per)).append("\n");
                DataWrite_CV_L.append(QString::number(avg_CV1m)).append("\n");

                DataWrite_R.append(QString::number(A_per_R)).append("\n");
                DataWrite_CV_R.append(QString::number(avg_CV1m_R)).append("\n");

                Write_newData(LeftSide);
                Write_newData(LeftSide_CV);

                Write_newData(RightSide);
                Write_newData(RightSide_CV);

                //Write_newData(Write_R);

                //display xAxis to 30000m
                if(datalenght_L >= 30){
                    ui->Chart_L->xAxis->setRange(datalenght_L-30, datalenght_L + 1);
                    //ui->Chart_L->xAxis2->setRange(datalenght_L-30000, datalenght_L);
                }
                else{
                    ui->Chart_L->xAxis->setRange(0, 31);
                    //ui->Chart_L->xAxis2->setRange(0, 30000);
                }

                if(datalenght_R >= 30){
                    ui->Chart_R->xAxis->setRange(datalenght_R-30, datalenght_R + 1);
                    //ui->Chart_R->xAxis2->setRange(datalenght_R-30000, datalenght_R);
                }
                else{
                    ui->Chart_R->xAxis->setRange(0, 31);
                    //ui->Chart_R->xAxis2->setRange(0, 30000);
                }



                //set yAxis range - A% path
                float L_Aper_Limit = ui->L_limit_Aper->text().toFloat() * 1.5;
                float R_Aper_Limit = ui->R_limit_Aper->text().toFloat() * 1.5;

                ui->Chart_L->yAxis->setRange(-L_Aper_Limit, L_Aper_Limit);
                ui->Chart_R->yAxis->setRange(-R_Aper_Limit, R_Aper_Limit);

                //set yAxis2 range - CV path
                float L_CVper_Limit = ui->L_limit_CVper->text().toFloat() * 1.5;
                float R_CVper_Limit = ui->R_limit_CVper->text().toFloat() * 1.5;

                ui->Chart_L->yAxis2->setRange(-0.5, L_CVper_Limit);
                ui->Chart_R->yAxis2->setRange(-0.5, R_CVper_Limit);

                ui->Chart_L->replot();
                ui->Chart_R->replot();

                avg_CV1m = 0;
                avg_AD_value_forWrite = 0;

                avg_CV1m_R = 0;
                avg_AD_value_forWrite_R = 0;
                SampleTimes[2] = 0; //reset flag
            }
        }
    }
}
void qualitymonitor::Read_oldData(int index)
{
    switch(index)
    {
        case LeftSide:
        {
            //read current date Data
            QString oldDataFilename_L = QDir().currentPath() + "/History/LeftSide/" + RunDateTime;
            QFile oldData(oldDataFilename_L);
            if(oldData.exists())
            {
                //series = new QSplineSeries();

                if(!oldData.open(QFile::ReadOnly | QFile::Text))
                {
                    qDebug() << "Cannot read old data!";
                }
                QTextStream Readin(&oldData);

                DataWrite_L = Readin.readAll();
                QStringList oldDataRead = DataWrite_L.split("\n");
                oldDataRead.removeLast();
                //qDebug() << read_data;
                double i = 0;

                foreach (QString str, oldDataRead) {
                    //ui->HistoryChart->graph(0)->addData(i , str.toDouble());
                    //historyData.append(str.toDouble());
                    //historyData_x.append(i);
                    oldData_Aper_L.append(str.toDouble());
                    axixX_L.append(i);
                    i += 0.1;
                }

                oldData.close();
            }
            break;
        }
        case RightSide:
        {
            //read current date Data
            QString oldDataFilename_R = QDir().currentPath() + "/History/RightSide/" + RunDateTime;
            QFile oldData(oldDataFilename_R);
            if(oldData.exists())
            {
                //series = new QSplineSeries();

                if(!oldData.open(QFile::ReadOnly | QFile::Text))
                {
                    qDebug() << "Cannot read old data!";
                }
                QTextStream Readin(&oldData);

                DataWrite_R = Readin.readAll();
                QStringList oldDataRead = DataWrite_R.split("\n");
                oldDataRead.removeLast();
                //qDebug() << read_data;
                double i = 0;

                foreach (QString str, oldDataRead) {
                    //ui->HistoryChart->graph(0)->addData(i , str.toDouble());
                    //historyData.append(str.toDouble());
                    //historyData_x.append(i);
                    oldData_Aper_R.append(str.toDouble());
                    axixX_R.append(i);
                    i += 0.1;
                }

                oldData.close();
            }
            break;
        }
        case LeftSide_CV:
        {
            QString oldDataFilename_L_CV = QDir().currentPath() + "/History/LeftSide/" + RunDateTime + "_CV";
            QFile oldData(oldDataFilename_L_CV);
            if(oldData.exists())
            {
                if(!oldData.open(QFile::ReadOnly | QFile::Text))
                {
                    qDebug() << "Cannot read old data!";
                }
                QTextStream Readin(&oldData);

                DataWrite_CV_L = Readin.readAll();
                QStringList oldDataRead = DataWrite_CV_L.split("\n");
                oldDataRead.removeLast();
                //qDebug() << oldDataRead;

                foreach (QString str, oldDataRead) {
                    //ui->HistoryChart->graph(0)->addData(i , str.toDouble());
                    //historyData.append(str.toDouble());
                    //historyData_x.append(i);
                    oldData_CV_L.append(str.toDouble());
                    //axixX_L.append(i);
                    //i+=100;
                }

                oldData.close();
            }
            break;
        }
        case RightSide_CV:
        {
            QString oldDataFilename_R_CV = QDir().currentPath() + "/History/RightSide/" + RunDateTime + "_CV";
            QFile oldData(oldDataFilename_R_CV);
            if(oldData.exists())
            {
                if(!oldData.open(QFile::ReadOnly | QFile::Text))
                {
                    qDebug() << "Cannot read old data!";
                }
                QTextStream Readin(&oldData);

                DataWrite_CV_R = Readin.readAll();
                QStringList oldDataRead = DataWrite_CV_R.split("\n");
                oldDataRead.removeLast();
                //qDebug() << oldDataRead;

                foreach (QString str, oldDataRead) {
                    //ui->HistoryChart->graph(0)->addData(i , str.toDouble());
                    //historyData.append(str.toDouble());
                    //historyData_x.append(i);
                    oldData_CV_R.append(str.toDouble());
                    //axixX_L.append(i);
                    //i+=100;
                }

                oldData.close();
            }
            break;
        }
    }
}

void qualitymonitor::deleteOldHistoryData()
{
    QDate Today = QDate().currentDate();
    QString Filename = QDir().currentPath() + "/History";
    QDir dir(Filename + "/LeftSide");

    // remove dot and dotdot
    dir.setFilter(QDir::Files);

    // Left side
    QFileInfoList fileList = dir.entryInfoList();

    foreach(QFileInfo file, fileList)
    {
        QDate fileDate = QDate().fromString(file.fileName(), Qt::ISODate);
        if(fileDate < Today.addDays(-14))
            dir.remove(file.fileName());
    }

    // Right side
    dir.setPath(Filename + "/RightSide");
    fileList = dir.entryInfoList();

    foreach(QFileInfo file, fileList)
    {
        QDate fileDate = QDate().fromString(file.fileName(), Qt::ISODate);
        if(fileDate < Today.addDays(-14))
            dir.remove(file.fileName());
    }

}
void qualitymonitor::Write_newData(int index)
{

    deleteOldHistoryData();

    switch(index){
        case LeftSide:{
            QString Filename_L = QDir().currentPath() + "/History/LeftSide/" + RunDateTime;
            QFile write(Filename_L);

            if(!write.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream Writeout_L(&write);
            //DataWrite_L.append(QString::number( ui->out1_pos->text().toFloat()) + "\n");
            Writeout_L << DataWrite_L;
            write.flush();
            write.close();
            break;
        }
        case RightSide:{
            QString Filename_R = QDir().currentPath() + "/History/RightSide/" + RunDateTime;
            QFile write(Filename_R);

            if(!write.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream Writeout_R(&write);
            //DataWrite_R.append(QString::number( ui->out2_pos->text().toFloat()) + "\n");
            Writeout_R << DataWrite_R;
            write.flush();
            write.close();
            break;
        }
        case LeftSide_SPG :{
        //Side chose
            QString Filename = QDir().currentPath() + "/SPG_data_L";
            QFile WriteSPG_L(Filename);

            if(!WriteSPG_L.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream out(&WriteSPG_L);
            out << DataWrite_SPG.join("\n");
            WriteSPG_L.flush();
            WriteSPG_L.close();
            break;
        }
        case RightSide_SPG :{
        //Side chose
            QString Filename = QDir().currentPath() + "/SPG_data_R";
            QFile WriteSPG_R(Filename);

            if(!WriteSPG_R.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream out(&WriteSPG_R);
            out << DataWrite_SPG_R.join("\n");
            WriteSPG_R.flush();
            WriteSPG_R.close();
            break;
        }
        case LeftSide_CV:{
            QString Filename_L = QDir().currentPath() + "/History/LeftSide/" + RunDateTime + "_CV";
            QFile write(Filename_L);
            if(!write.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream out(&write);
            out << DataWrite_CV_L;
            write.flush();
            write.close();
            break;
        }
        case RightSide_CV:{
            QString Filename_R = QDir().currentPath() + "/History/RightSide/" + RunDateTime + "_CV";
            QFile write(Filename_R);
            if(!write.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream out(&write);
            out << DataWrite_CV_R;
            write.flush();
            write.close();
            break;
        }
    }
}

void qualitymonitor::Set_Graphics_L()
{
    static double datalenght = 0;
    //Read_oldData();
    Read_oldData(LeftSide);
    Read_oldData(LeftSide_CV);

    ui->Chart_L->addGraph();
    ui->Chart_L->addGraph(ui->Chart_L->xAxis, ui->Chart_L->yAxis2);

    ui->Chart_L->graph(0)->setData(axixX_L, oldData_Aper_L);
    ui->Chart_L->graph(1)->setData(axixX_L, oldData_CV_L);


    datalenght_L = axixX_L.length()*0.1;        //axixX_L's unit is cm, need to transfer to m.
    //qDebug() << datalenght_L;

    //set CV yAxis visible
    ui->Chart_L->yAxis2->setVisible(true);
    ui->Chart_L->yAxis2->setTickLabelColor(Qt::blue);

    QPen pen = ui->Chart_L->graph(0)->pen();
    pen.setWidth(3);
    QLinearGradient linearGradient(0, 0, 0, 320);
    linearGradient.setColorAt(0,    Qt::red);
    linearGradient.setColorAt(0.19, Qt::red);
    linearGradient.setColorAt(0.20, Qt::yellow);
    linearGradient.setColorAt(0.34, Qt::yellow);
    linearGradient.setColorAt(0.35, Qt::green);
    linearGradient.setColorAt(0.65, Qt::green);
    linearGradient.setColorAt(0.66, Qt::yellow);
    linearGradient.setColorAt(0.80, Qt::yellow);
    linearGradient.setColorAt(0.81, Qt::red);
    linearGradient.setColorAt(1.0,  Qt::red);

    pen.setBrush(QBrush(linearGradient));

    ui->Chart_L->graph(0)->setPen(pen);

    QPen CV_pen;
    CV_pen.setWidth(2);
    CV_pen.setColor(Qt::blue);
    ui->Chart_L->graph(1)->setPen(CV_pen);

    QFont xAxis_TickLabelFont;
    xAxis_TickLabelFont.setPointSize(8);
    ui->Chart_L->xAxis->setTickLabelFont(xAxis_TickLabelFont);

    // set axes ranges:
    if(datalenght_L >= 30){
        ui->Chart_L->xAxis->setRange(datalenght_L -30, datalenght_L +1);
        //ui->Chart_L->xAxis2->setRange(datalenght-30000, datalenght);
    }
    else{
        ui->Chart_L->xAxis->setRange(0, 31);
        //ui->Chart_L->xAxis2->setRange(0, 30000);
    }

    float L_Aper_Limit = ui->L_limit_Aper->text().toFloat() * 1.5;
    float CV_per_Limit = ui->L_limit_CVper->text().toFloat() * 1.5;

    ui->Chart_L->yAxis->setRange(-L_Aper_Limit, L_Aper_Limit);
    ui->Chart_L->yAxis2->setRange(-0.5, CV_per_Limit);

    //clear array, release memory
    oldData_Aper_L.clear();
    oldData_CV_L.clear();
    axixX_L.clear();

    ui->Chart_L->replot();
}

void qualitymonitor::Set_Graphics_R()
{
    static double datalenght = 0;

    Read_oldData(RightSide);
    Read_oldData(RightSide_CV);

    ui->Chart_R->addGraph();
    ui->Chart_R->addGraph(ui->Chart_R->xAxis, ui->Chart_R->yAxis2);

    ui->Chart_R->graph(0)->setData(axixX_R, oldData_Aper_R);
    ui->Chart_R->graph(1)->setData(axixX_R, oldData_CV_R);


    datalenght_R = axixX_R.length()*0.1;        //axixX_L's unit is cm, need to transfer to m.
    //qDebug() << datalenght_L;

    //ui->Chart_R->graph(0)->setData(axixX_R, mData_R);

    //set CV yAxis visible
    ui->Chart_R->yAxis2->setVisible(true);
    ui->Chart_R->yAxis2->setTickLabelColor(Qt::blue);

    QPen pen = ui->Chart_R->graph(0)->pen();
    pen.setWidth(3);
    QLinearGradient linearGradient(0, 0, 0, 320);
    linearGradient.setColorAt(0,    Qt::red);
    linearGradient.setColorAt(0.19, Qt::red);
    linearGradient.setColorAt(0.20, Qt::yellow);
    linearGradient.setColorAt(0.34, Qt::yellow);
    linearGradient.setColorAt(0.35, Qt::green);
    linearGradient.setColorAt(0.65, Qt::green);
    linearGradient.setColorAt(0.66, Qt::yellow);
    linearGradient.setColorAt(0.80, Qt::yellow);
    linearGradient.setColorAt(0.81, Qt::red);
    linearGradient.setColorAt(1.0,  Qt::red);

    pen.setBrush(QBrush(linearGradient));

    ui->Chart_R->graph(0)->setPen(pen);

    QPen CV_pen;
    CV_pen.setWidth(2);
    CV_pen.setColor(Qt::blue);
    ui->Chart_R->graph(1)->setPen(CV_pen);


    QFont xAxis_TickLabelFont;
    xAxis_TickLabelFont.setPointSize(8);
    ui->Chart_R->xAxis->setTickLabelFont(xAxis_TickLabelFont);

    // set axes ranges:
    if(datalenght_R >= 30){
        ui->Chart_R->xAxis->setRange(datalenght_R -30, datalenght_R +1);
        //ui->Chart_R->xAxis2->setRange(datalenght-30000, datalenght);
    }
    else{
        ui->Chart_R->xAxis->setRange(0, 31);
        //ui->Chart_R->xAxis2->setRange(0, 30000);
    }

    float R_Aper_Limit = ui->R_limit_Aper->text().toFloat() * 1.5;
    float CV_per_Limit = ui->R_limit_CVper->text().toFloat() * 1.5;

    ui->Chart_R->yAxis->setRange(-R_Aper_Limit, R_Aper_Limit);
    ui->Chart_R->yAxis2->setRange(-0.5, CV_per_Limit);

    //clear array, release memory
    oldData_Aper_R.clear();
    oldData_CV_R.clear();
    axixX_R.clear();

    ui->Chart_R->replot();
}
void qualitymonitor::Setup_HistoryChart()
{
    ui->HistoryChart->addGraph();    
    ui->HistoryChart->addGraph(ui->HistoryChart->xAxis, ui->HistoryChart->yAxis2);

    ui->HistoryChart->xAxis->setRange(0, 300);
    ui->HistoryChart->yAxis->setRange(-0.5, 10);

    //set chart can Drag, Zoom & Selected
    ui->HistoryChart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    //set CV yAxis visible
    ui->HistoryChart->yAxis2->setVisible(true);
    ui->HistoryChart->yAxis2->setTickLabelColor(Qt::blue);

    ui->HistoryChart->xAxis->setLabel("Length(km)");
    ui->HistoryChart->yAxis->setLabel("A%");
    ui->HistoryChart->yAxis2->setLabel("CV%");

    QPen pen = ui->HistoryChart->graph(0)->pen();
    pen.setWidth(3);


    QLinearGradient linearGradient(0, 0, 0, 320);
    linearGradient.setColorAt(0,    Qt::red);
    linearGradient.setColorAt(0.19, Qt::red);
    linearGradient.setColorAt(0.20, Qt::yellow);
    linearGradient.setColorAt(0.34, Qt::yellow);
    linearGradient.setColorAt(0.35, Qt::green);
    linearGradient.setColorAt(0.65, Qt::green);
    linearGradient.setColorAt(0.66, Qt::yellow);
    linearGradient.setColorAt(0.80, Qt::yellow);
    linearGradient.setColorAt(0.81, Qt::red);
    linearGradient.setColorAt(1.0,  Qt::red);

    pen.setBrush(QBrush(linearGradient));

    ui->HistoryChart->graph(0)->setPen(pen);
    QPen CV_pen;
    CV_pen.setWidth(2);
    CV_pen.setColor(Qt::blue);
    ui->HistoryChart->graph(1)->setPen(CV_pen);

    float Aper_Limit = ui->L_limit_Aper->text().toFloat() * 1.5;
    float CV_per_Limit = ui->L_limit_CVper->text().toFloat() * 1.5;

    ui->HistoryChart->yAxis->setRange(-Aper_Limit, Aper_Limit);
    ui->HistoryChart->yAxis2->setRange(-0.5, CV_per_Limit);

    connect(ui->HistoryChart->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(fixed_yAxisRange(QCPRange)));
    connect(ui->HistoryChart->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(fixed_xAxisRange(QCPRange)));
}


void qualitymonitor::set_SPG_Chart()
{
    ui->SPG_Chart->addGraph();
    ui->SPG_Chart2->addGraph();

    //QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    //ui->SPG_Chart->xAxis->setTicker(logTicker);

    QSharedPointer<QCPAxisTickerText> TransTologTicker(new QCPAxisTickerText);
    ui->SPG_Chart->xAxis->setTicker(TransTologTicker);
    ui->SPG_Chart2->xAxis->setTicker(TransTologTicker);

    //ui->SPG_Chart->xAxis->setScaleType(QCPAxis::stLogarithmic);

    ui->SPG_Chart->xAxis->setRange(1, 10000);
    ui->SPG_Chart->yAxis->setRange(-0.5, 10);

    ui->SPG_Chart2->xAxis->setRange(1, 10000);
    ui->SPG_Chart2->yAxis->setRange(-0.5, 10);

    QVector<double> SPG = Mymathtool.SPG(SPG_Data);
    QVector<double> SPG_R = Mymathtool.SPG(SPG_Data_R);

    //QVector<double> interval(SPG.length());
    QVector<QString> interval(SPG.length());
    QVector<double> x(SPG.length());


    //clear data when work done
    int chw = 5 * log2(4096 / 4) - 4;
/*
    for(int i= 0; i <= chw; i++){                //set how many chs
        interval[i] = pow(2 , 0.2*(i+5));       //pow()用來求 x 的 y 次方
        //printf("%f\n", interval[i]);
    }
*/
    for(int i= 0; i <= chw; i++){                //set how many chs
        interval[i] = QString::number(pow(2 , 0.2*(i+5)), 'f', 0);       //pow()用來求 x 的 y 次方
        //printf("%f\n", interval[i]);
    }
    for(int i= 0; i < SPG.length(); i++){
        x[i] = i+1;
        //set ticklabel
        if(i % 5 == 0)
            TransTologTicker->addTick(i, interval[i]);
    }



    QCPBars *SPG_Bar = new QCPBars(ui->SPG_Chart->xAxis, ui->SPG_Chart->yAxis);
    QCPBars *SPG_Bar_R = new QCPBars(ui->SPG_Chart2->xAxis, ui->SPG_Chart2->yAxis);

    //setData to Bar Chart
    //SPG_Bar->setData(interval, SPG);
    SPG_Bar->setData(x, SPG);
    SPG_Bar_R->setData(x, SPG_R);


    SPG_Bar->setWidth(1);
    SPG_Bar_R->setWidth(1);

    //setRange
    ui->SPG_Chart->xAxis->setRange(1, 50);
    ui->SPG_Chart->yAxis->rescale();

    ui->SPG_Chart2->xAxis->setRange(1, 50);
    ui->SPG_Chart2->yAxis->rescale();


    ui->SPG_Chart->replot();
    ui->SPG_Chart2->replot();

    //clear data
    SPG_Bar->data()->clear();
    SPG_Bar_R->data()->clear();

}


void qualitymonitor::set_listview_historyFile()
{
    //setup listview historyFile
    History_filemodel = new QFileSystemModel(this);
    QString HistoryRootPath = QDir().currentPath() + "/History";

    //make sure all the director existed
    if(!QDir(HistoryRootPath).exists())
        QDir().mkdir("History");
    if(!QDir(HistoryRootPath + "/RightSide").exists())
        QDir(HistoryRootPath).mkdir("RightSide");
    if(!QDir(HistoryRootPath + "/LeftSide").exists())
        QDir(HistoryRootPath).mkdir("LeftSide");

    History_filemodel->setRootPath(HistoryRootPath);
    History_filemodel->setFilter(QDir::NoDotAndDotDot | QDir::Files);

    ui->listView_historyFile->setModel(History_filemodel);
    //default Left Side
    ui->listView_historyFile->setRootIndex(History_filemodel->setRootPath(HistoryRootPath + "/LeftSide"));
}


void qualitymonitor::on_listView_historyFile_clicked(const QModelIndex &index)
{
    //history file view
    QString filename = History_filemodel->fileInfo(index).absoluteFilePath();
    //qDebug() << filename;
    //init Data
    historyData.clear();
    historyData_x.clear();
    historyData_CV.clear();
    if(!ui->HistoryChart->graph(0)->data()->isEmpty())
    {
        ui->HistoryChart->graph(0)->data().clear();
        ui->HistoryChart->graph(1)->data().clear();
    }

    Read_historyData(filename);
    ui->HistoryChart->graph(0)->setData(historyData_x, historyData);
    ui->HistoryChart->graph(1)->setData(historyData_x, historyData_CV);

    if(historyData_x.length() > 0)
    {
        ui->HistoryChart->xAxis->rescale();
        //ui->HistoryChart->xAxis2->rescale();

        //ui->HistoryChart->yAxis->rescale();
    }

    ui->HistoryChart->replot();
    //ui->HistoryChart->graph(0)->data()->clear();
}
void qualitymonitor::Read_historyData(QString filename)
{
    QFile ReadHistory(filename);
    QFile ReadHistory_CV(filename + "_CV");

    //series = new QSplineSeries();


        if(!ReadHistory.open(QFile::ReadOnly | QFile::Text))
        {
            qDebug() << "Cannot read old data!";
        }
        QTextStream Readin(&ReadHistory);
        QStringList Historylist = Readin.readAll().split("\n");
        Historylist.removeLast();
        //qDebug() << read_data;
        double i = 0;
        foreach (QString str, Historylist) {
            //ui->HistoryChart->graph(0)->addData(i , str.toDouble());
            historyData.append(str.toDouble());
            historyData_x.append(i);
            i+=0.1;
        }
        ReadHistory.close();


        if(!ReadHistory_CV.open(QFile::ReadOnly | QFile::Text))
        {
            qDebug() << "Cannot read old data!";
        }
        QTextStream Readin_CV(&ReadHistory_CV);
        QStringList Historylist_CV = Readin_CV.readAll().split("\n");
        Historylist_CV.removeLast();
        //qDebug() << Historylist_CV;

        foreach (QString str, Historylist_CV) {
            //ui->HistoryChart->graph(0)->addData(i , str.toDouble());
            historyData_CV.append(str.toDouble());
            //historyData_x.append(i);
            i+=0.1;
        }
        ReadHistory_CV.close();
}
void qualitymonitor::on_comboBox_SideChose_currentIndexChanged(int index)
{
    //side chose combobox
    QString HistoryRootPath = QDir().currentPath() + "/History";

    ui->HistoryChart->graph(0)->data()->clear();
    ui->HistoryChart->graph(1)->data()->clear();
    ui->HistoryChart->replot();

    //qDebug() << index;
    switch(index){
        case LeftSide:{
            ui->listView_historyFile->setRootIndex(History_filemodel->setRootPath(HistoryRootPath + "/LeftSide"));
            float Aper_Limit = ui->L_limit_Aper->text().toFloat() * 1.5;
            float CV_per_Limit = ui->L_limit_CVper->text().toFloat() * 1.5;

            ui->HistoryChart->yAxis->setRange(-Aper_Limit, Aper_Limit);
            ui->HistoryChart->yAxis2->setRange(-0.5, CV_per_Limit);

            ui->HistoryChart->replot();
            break;
        }

        case RightSide:{
            ui->listView_historyFile->setRootIndex(History_filemodel->setRootPath(HistoryRootPath + "/RightSide"));
            float Aper_Limit = ui->R_limit_Aper->text().toFloat() * 1.5;
            float CV_per_Limit = ui->R_limit_CVper->text().toFloat() * 1.5;

            ui->HistoryChart->yAxis->setRange(-Aper_Limit, Aper_Limit);
            ui->HistoryChart->yAxis2->setRange(-0.5, CV_per_Limit);

            ui->HistoryChart->replot();
            break;
        }
    }
    /*
    if(!ui->HistoryChart->graph(0)->data()->isEmpty())
    {
        ui->HistoryChart->graph(0)->data().clear();
        ui->HistoryChart->graph(1)->data().clear();
        ui->HistoryChart->replot();
    }*/
}



/***********************************************************
 *
 *
 *
 *
 *
 *
 *
 *  Test finished!
 *
 *
 *
 *
 *
 *
 *
 * *********************************************************/

void qualitymonitor::on_pushButton_ErrorSig_clicked(QString reasons)
{
    parameter ErrorRecord;
    //error happened do, and be slot
    //name *item as add QtableWidget item

    //insertRow in Row 0
    ui->tableWidget->insertRow(0);
    //add Date, Time and Reason
    //Reason need modify as sig
    QTableWidgetItem *item = new QTableWidgetItem();
    item = new QTableWidgetItem(QString(QDate().currentDate().toString("yyyy/MM/dd")));
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
    ui->tableWidget->setItem(0, 0, item);
    item = new QTableWidgetItem(QString(QTime().currentTime().toString()));
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
    ui->tableWidget->setItem(0, 1, item);
    item = new QTableWidgetItem(ui->label_whichShift->text());
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
    ui->tableWidget->setItem(0, 2, item);
    item = new QTableWidgetItem(reasons);
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
    ui->tableWidget->setItem(0, 3, item);

    QString beWrite;
    beWrite = QString(";%1;%2;%3;%4").arg(QString(QDate().currentDate().toString("yyyy/MM/dd")))   \
              .arg(QString(QTime().currentTime().toString()))                                   \
              .arg(ui->label_whichShift->text())                                                \
              .arg(reasons.append("\n"))                                                        \
              .append(ErrorRecord.Read(QDir().currentPath() + "/errorhistory", 0));

    //qDebug() << beWrite;

    /*
    QString result;
    foreach(const QString &str, beWrite){
        result.append(str);
    }/
    qDebug() << result;*/
    ErrorRecord.Write(QDir().currentPath() + "/errorhistory" , beWrite);
}
void qualitymonitor::DateTimeSlot()
{   //label_14
    QDateTime DateTime = QDateTime::currentDateTime();
    //int toUTC = QString(ui->lineEdit_UTC->text()).toInt();

    QString Date = DateTime.toString("yyyy/MM/dd");
    QString Time = DateTime.toString("hh:mm:ss");
    ui->Date -> setText(Date);
    ui->Time -> setText(Time);

    //qDebug() << QThread::currentThread();

    whichShift();

    QTime time = QTime(DateTime.time().hour(), DateTime.time().minute()).addSecs(set_timeHour *3600 + set_timeMinu *60);
    ui->label_hh->setNum(time.hour());
    ui->label_mm->setNum(time.minute());
    ui->label_ss->setNum(DateTime.time().second());
}
void qualitymonitor::whichShift()
{
    QDate currentDate = QDate().currentDate();
    QDateTime currentDateTime = QDateTime().currentDateTime();
    QDateTime DateTime_start_shift1 = QDateTime(currentDate, QTime(startshift1, 0));

    if(currentDateTime.time() < DateTime_start_shift1.time())
    {
        // if currentDateTime less than first shift, means current was counted yesterday.
        currentDate = currentDate.addDays(-1);
    }// end if

    QDate shift_OneDay = currentDate.addDays(1);

    DateTime_start_shift1 = QDateTime(currentDate, QTime(startshift1, 0));
    QDateTime DateTime_start_shift2 = QDateTime(currentDate, QTime(startshift2, 0));
    QDateTime DateTime_start_shift3 = QDateTime(currentDate, QTime(startshift3, 0));
    QDateTime DateTime_start_shift4 = QDateTime(currentDate, QTime(startshift4, 0));
    QDateTime DateTime_end_shift1, DateTime_end_shift2, DateTime_end_shift3, DateTime_end_shift4;

    DateTime_end_shift1 = QDateTime(currentDate, QTime(endshift1, 0));
    DateTime_end_shift2 = QDateTime(currentDate, QTime(endshift2, 0));
    DateTime_end_shift3 = QDateTime(currentDate, QTime(endshift3, 0));
    DateTime_end_shift4 = QDateTime(currentDate, QTime(endshift4, 0));


    if((startshift1 == 0) || (endshift1 <= startshift1)){
        // if first shift cross day, and shift others shifts 1 day.
        DateTime_end_shift1 = QDateTime(shift_OneDay, QTime(endshift1, 0));

        DateTime_start_shift2 = QDateTime(shift_OneDay, QTime(startshift2, 0));
        DateTime_start_shift3 = QDateTime(shift_OneDay, QTime(startshift3, 0));
        DateTime_start_shift4 = QDateTime(shift_OneDay, QTime(startshift4, 0));
        DateTime_end_shift2 = QDateTime(shift_OneDay, QTime(endshift2, 0));
        DateTime_end_shift3 = QDateTime(shift_OneDay, QTime(endshift3, 0));
        DateTime_end_shift4 = QDateTime(shift_OneDay, QTime(endshift4, 0));
    }// end if
    else if((startshift2 == 0) ||(endshift2 <= startshift2)){
        DateTime_end_shift2 = QDateTime(shift_OneDay, QTime(endshift2, 0));

        DateTime_start_shift3 = QDateTime(shift_OneDay, QTime(startshift3, 0));
        DateTime_start_shift4 = QDateTime(shift_OneDay, QTime(startshift4, 0));
        DateTime_end_shift3 = QDateTime(shift_OneDay, QTime(endshift3, 0));
        DateTime_end_shift4 = QDateTime(shift_OneDay, QTime(endshift4, 0));
    }
    else if((startshift3 == 0) || (endshift3 <= startshift3)){
        DateTime_end_shift3 = QDateTime(shift_OneDay, QTime(endshift3, 0));

        DateTime_start_shift4 = QDateTime(shift_OneDay, QTime(startshift4, 0));
        DateTime_end_shift4 = QDateTime(shift_OneDay, QTime(endshift4, 0));
    }
    else if((startshift4 == 0) || (endshift4 <= startshift4)){
        DateTime_end_shift4 = QDateTime(shift_OneDay, QTime(endshift4, 0));
    }



    bool isshift_1 = (QDateTime().currentDateTime() >= DateTime_start_shift1) && (QDateTime().currentDateTime() <= DateTime_end_shift1);
    bool isshift_2 = (QDateTime().currentDateTime() >= DateTime_start_shift2) && (QDateTime().currentDateTime() <= DateTime_end_shift2);
    bool isshift_3 = (QDateTime().currentDateTime() >= DateTime_start_shift3) && (QDateTime().currentDateTime() <= DateTime_end_shift3);
    bool isshift_4 = (QDateTime().currentDateTime() >= DateTime_start_shift4) && (QDateTime().currentDateTime() <= DateTime_end_shift4);

    /*******************************************
    // debug schedule
    qDebug() << currentDate;
    qDebug() << DateTime_start_shift1 << DateTime_end_shift1;
    qDebug() << DateTime_start_shift2 << DateTime_end_shift2;
    qDebug() << DateTime_start_shift3 << DateTime_end_shift3;
    qDebug() << DateTime_start_shift4 << DateTime_end_shift4;
    ********************************************/
    if(isshift_1)
        ui->label_whichShift->setText("班別 1");

    else if(isshift_2)
        ui->label_whichShift->setText("班別 2");
    else if(isshift_3)
        ui->label_whichShift->setText("班別 3");
    else if(isshift_4)
        ui->label_whichShift->setText("班別 4");
/*
    else if(isshift_3){
        if((ui->comboBox_shifts->currentText() != "3") || (ui->comboBox_shifts->currentText() == "4"))
            ui->label_whichShift->setText("Recheck Shift Schedule");
        else
            ui->label_whichShift->setText("班別 3");
    }// end else if
    else if(isshift_4){
        if(ui->comboBox_shifts->currentText() != "4")
            ui->label_whichShift->setText("Recheck Shift Schedule");

        else
            ui->label_whichShift->setText("班別 4");
    }// end else if
*/
    else
        ui->label_whichShift->setText("Recheck Shift Schedule");

}

void qualitymonitor::count_ISR_times()
{
    //float pulselength = ui->PulseLength->text().toFloat();
    static int ISR_counter[3] = {0};
    // lowpass filter
    ISR_counter[2] = ISR_counter[1];
    ISR_counter[1] = ISR_counter[0];
    //ISR_counter[0] = (isr_count_tick - ISR_counter[1]) * Filter1 + ISR_counter[1];
    if(isr_count_tick > 2000)
        ISR_counter[0] = (isr_count_tick + ISR_counter[1]) /2;
    else
        ISR_counter[0] = isr_count_tick;
    //float currentSpeed = ISR_counter[0] *pulselength *60 /1000;
    ui->label_speed->setText("Speed: " + QString::number(ISR_counter[0] *onePulseLength *60 /1000, 'f' , 0) + "\t m/min");
    // show speed in test frame
    ui->label_testframe_speed->setText(QString::number(ISR_counter[0] *onePulseLength *60 /1000, 'f' , 0) + "\t m/min");
    ui->trig_count->setText(QString::number(ISR_counter[0]) + "\t times/sec");
    isr_count_tick = 0; //
    //qDebug() << "count";
}


void qualitymonitor::setupParameter()
{
    parameter initParameter;

    //read L-side parameter
    QString L_feedoutcenter = QString::number(initParameter.L_feedoutcenter());
    QString L_outputoffset  = QString::number(initParameter.L_outputoffset());
    QString L_adjustrate    = QString::number(initParameter.L_adjustrate());
    QString L_outputweight  = QString::number(initParameter.L_outputweight(),'f',2);
    QString L_limit_Aper    = QString::number(initParameter.L_limit_Aper());
    QString L_limit_CVper   = QString::number(initParameter.L_limit_CVper());

    //setup L-side parameter
    ui->L_feedoutcenter ->setText(L_feedoutcenter);
    ui->L_outputoffset  ->setText(L_outputoffset);
    ui->L_adjustrate    ->setText(L_adjustrate);
    ui->L_outputweight  ->setText(L_outputweight);
    ui->L_limit_Aper    ->setText(L_limit_Aper);
    ui->L_limit_CVper   ->setText(L_limit_CVper);

    // set Feedoutcenter_L used to calculate A%
    Feedoutcenter_L = ui->L_feedoutcenter->text().toInt();

    //read R-side parameter
    QString R_feedoutcenter = QString::number(initParameter.R_feedoutcenter());
    QString R_outputoffset  = QString::number(initParameter.R_outputoffset());
    QString R_adjustrate    = QString::number(initParameter.R_adjustrate());
    QString R_outputweight  = QString::number(initParameter.R_outputweight(),'f',2);
    QString R_limit_Aper    = QString::number(initParameter.R_limit_Aper());
    QString R_limit_CVper   = QString::number(initParameter.R_limit_CVper());

    //setup R-side parameter
    ui->R_feedoutcenter ->setText(R_feedoutcenter);
    ui->R_outputoffset  ->setText(R_outputoffset);
    ui->R_adjustrate    ->setText(R_adjustrate);
    ui->R_outputweight  ->setText(R_outputweight);
    ui->R_limit_Aper    ->setText(R_limit_Aper);
    ui->R_limit_CVper   ->setText(R_limit_CVper);

    // set Feedoutcenter_R used to calculate A%
    Feedoutcenter_R = ui->R_feedoutcenter->text().toInt();

    //setup EE parameter
    int MachineType = initParameter.MachineType();
    QString Filter_1    = QString::number(initParameter.Filter_1());
    QString Filter_2    = QString::number(initParameter.Filter_2());
    QString OffsetAdjust  = QString::number(initParameter.OffsetAdjust());

    ui->comboBox_machineType->setCurrentIndex(MachineType);
    ui->Filter_1    ->setText(Filter_1);
    ui->Filter_2    ->setText(Filter_2);
    ui->OffsetAdjust->setText(OffsetAdjust);

    //set LVDT offset
    //QString outputL_offset = QString::number(initParameter.Out1_Offset());
    //QString outputR_offset = QString::number(initParameter.Out2_Offset());

    ui->out1_offset->setText(L_outputoffset);
    ui->out2_offset->setText(R_outputoffset);

    QString password = initParameter.Password();

    //qDebug() << password;
    ui->lineEdit_password->setText(password);

    // set current center in test frame
    ui->label_CurrentCenter_L->setText(ui->L_feedoutcenter->text());
    ui->label_CurrentCenter_R->setText(ui->R_feedoutcenter->text());


/*
    QFile password(":/password");
    if(!password.open(QFile::ReadOnly | QFile::Text))
        qDebug() << "Cannot load password";
    QTextStream readSavedPassword(&password);
    ui->->setText(readSavedPassword.readAll());
    */

    QStringList shiftschedule = initParameter.shiftschedule().split("\n");
    ui->comboBox_shifts->setCurrentText(shiftschedule.at(0));
    ui->label_startShift_1->setNum(shiftschedule.at(1).toInt());
    ui->label_endShift_1->setNum(shiftschedule.at(2).toInt());
    ui->label_startShift_2->setNum(shiftschedule.at(3).toInt());
    ui->label_endShift_2->setNum(shiftschedule.at(4).toInt());
    ui->label_startShift_3->setNum(shiftschedule.at(5).toInt());
    ui->label_endShift_3->setNum(shiftschedule.at(6).toInt());
    ui->label_startShift_4->setNum(shiftschedule.at(7).toInt());
    ui->label_endShift_4->setNum(shiftschedule.at(8).toInt());

    startshift1 = ui->label_startShift_1->text().toInt();
    endshift1 = ui->label_endShift_1->text().toInt();
    startshift2 = ui->label_startShift_2->text().toInt();
    endshift2 = ui->label_endShift_2->text().toInt();
    startshift3 = ui->label_startShift_3->text().toInt();
    endshift3 = ui->label_endShift_3->text().toInt();
    startshift4 = ui->label_startShift_4->text().toInt();
    endshift4 = ui->label_endShift_4->text().toInt();

    updateAllconfig();

}



void qualitymonitor::Setup_History()
{
    //set DateTime range
    ui->dateEdit_HistoryStartDate   ->setDate(QDate::currentDate().addDays(-14));
    ui->dateEdit_HistoryEndDate     ->setDate(QDate::currentDate());
    ui->dateEdit_HistoryStartDate   ->setCalendarPopup(true);
    ui->dateEdit_HistoryEndDate     ->setCalendarPopup(true);
    ui->dateEdit_HistoryStartDate   ->setMaximumDate(QDate::currentDate());
    ui->dateEdit_HistoryStartDate   ->setMinimumDate(QDate::currentDate().addDays(-14));
    ui->dateEdit_HistoryEndDate     ->setMaximumDate(QDate::currentDate());
    ui->dateEdit_HistoryEndDate     ->setMinimumDate(QDate::currentDate().addDays(-14));
}

void qualitymonitor::SetErrorTable()
{
    //ui->lineEdit->setPlaceholderText("search");
    //date selected range
    parameter SetErrorTable;
    ui->dateEdit_StartDate  ->setDate(QDate::currentDate().addDays(-14));
    ui->dateEdit_EndDate    ->setDate(QDate::currentDate());
    ui->dateEdit_StartDate  ->setCalendarPopup(true);
    ui->dateEdit_EndDate    ->setCalendarPopup(true);
    ui->dateEdit_StartDate  ->setMaximumDate(QDate::currentDate());
    ui->dateEdit_StartDate  ->setMinimumDate(QDate::currentDate().addDays(-14));
    ui->dateEdit_EndDate    ->setMaximumDate(QDate::currentDate());
    ui->dateEdit_EndDate    ->setMinimumDate(QDate::currentDate().addDays(-14));

    QFile recordFile(QDir().currentPath()+"/errorhistory");
    if(!recordFile.exists())
    {
        // if record file does not exist
        if(!recordFile.open(QFile::WriteOnly | QFile::Text))
            qDebug() << "Cannot write new data";

        QTextStream create(&recordFile);
        create << "hello";
        recordFile.close();
        recordFile.flush();
    }

    //cal how many data row number on last record
    QString ErrorHistory =  SetErrorTable.Read(QDir().currentPath() + "/errorhistory", 0);
    int rowNumber = ErrorHistory.count("\n");

    //creat table
    ui->tableWidget->setColumnCount(4);
    ui->tableWidget->setRowCount(ErrorHistory.count("\n"));
    //load data
    for (int i = 0; i < rowNumber; i++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item = new QTableWidgetItem(ErrorHistory.section(";", i*4+1, i*4+1));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 0, item);
        item = new QTableWidgetItem(ErrorHistory.section(";", i*4+2, i*4+2));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 1, item);
        item = new QTableWidgetItem(ErrorHistory.section(";", i*4+3, i*4+3));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 2, item);
        item = new QTableWidgetItem(ErrorHistory.section(";", i*4+4, i*4+4));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 3, item);
    }
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QStringList labels;
    labels<<"Date"<<"Time"<<"Shift"<<"Reason";
    ui->tableWidget->setHorizontalHeaderLabels(labels);
    ui->tableWidget->verticalHeader()->setVisible(false);
}
void qualitymonitor::on_pushButton_2_clicked()
{   //thread stop
    //mThread->start();
    //mThread->Stop = true;

}

void qualitymonitor::on_pushButton_Search_clicked()
{
    //error history search button
    QDate StartDate = ui->dateEdit_StartDate->date();
    QDate EndDate   = ui->dateEdit_EndDate  ->date();
    //qDebug() << StartDate <<"\n"<< EndDate <<"\n";
    int RowCount = ui->tableWidget->rowCount();

    //search
    if(StartDate.operator>(EndDate))
        qDebug () << "Search Logic ERROR!";
    else
        for(int row = 0; row < RowCount; row++)
        {
            QString rowData;
            rowData = ui->tableWidget->item(row, 0)->text();
            //search depend on Date
            QDate HappendedDate = QDate::fromString(rowData, Qt::ISODate);
        //show & hide row
            if(HappendedDate.operator<=(EndDate) && HappendedDate.operator>=(StartDate))
                ui->tableWidget->showRow(row);

            else
                ui->tableWidget->hideRow(row);
        }
}

void qualitymonitor::toSaveData(int indx){
    parameter writeParameter;
    Feedoutcenter_L = ui->L_feedoutcenter->text().toInt();
    Feedoutcenter_R = ui->R_feedoutcenter->text().toInt();
    switch (indx) {
    case normalParameter :
    {
        QString saveData;
        QString L_feedoutcenter = writeParameter.TrantoNumberType(ui->L_feedoutcenter->text());
        QString L_outputoffset  = writeParameter.TrantoNumberType(ui->L_outputoffset->text());
        QString L_adjustrate    = writeParameter.TrantoNumberType(ui->L_adjustrate->text());
        QString L_outputweight  = writeParameter.TrantoNumberType(ui->L_outputweight->text());
        QString L_limit_Aper    = writeParameter.TrantoNumberType(ui->L_limit_Aper->text());
        QString L_limit_CVper   = writeParameter.TrantoNumberType(ui->L_limit_CVper->text());

        QString R_feedoutcenter = writeParameter.TrantoNumberType(ui->R_feedoutcenter->text());
        QString R_outputoffset  = writeParameter.TrantoNumberType(ui->R_outputoffset->text());
        QString R_adjustrate    = writeParameter.TrantoNumberType(ui->R_adjustrate->text());
        QString R_outputweight  = writeParameter.TrantoNumberType(ui->R_outputweight->text());
        QString R_limit_Aper    = writeParameter.TrantoNumberType(ui->R_limit_Aper->text());
        QString R_limit_CVper   = writeParameter.TrantoNumberType(ui->R_limit_CVper->text());


        saveData = (L_feedoutcenter     + "\n");
        saveData.append(L_outputoffset  + "\n");
        saveData.append(L_adjustrate    + "\n");
        saveData.append(L_outputweight  + "\n");
        saveData.append(L_limit_Aper    + "\n");
        saveData.append(L_limit_CVper   + "\n");
        saveData.append(R_feedoutcenter + "\n");
        saveData.append(R_outputoffset  + "\n");
        saveData.append(R_adjustrate    + "\n");
        saveData.append(R_outputweight  + "\n");
        saveData.append(R_limit_Aper    + "\n");
        saveData.append(R_limit_CVper         );

        //qDebug() << saveData;
        writeParameter.Write(QDir().currentPath() + "/config", saveData);

        // set current center in test frame
        ui->label_CurrentCenter_L->setText(ui->L_feedoutcenter->text());
        ui->label_CurrentCenter_R->setText(ui->R_feedoutcenter->text());

        break;
    }
    case EEParameter :{
        QString saveData;
        int MachineType = ui->comboBox_machineType->currentIndex();
        QString Filter_1    = writeParameter.TrantoNumberType(ui->Filter_1->text());
        QString Filter_2    = writeParameter.TrantoNumberType(ui->Filter_2->text());
        QString OffsetAdjust  = writeParameter.TrantoNumberType(ui->OffsetAdjust->text());
        QString password    = ui->lineEdit_password->text();

        saveData = (QString::number(MachineType)         + "\n");
        saveData.append(Filter_1        + "\n");
        saveData.append(Filter_2        + "\n");
        saveData.append(OffsetAdjust      + "\n");
        saveData.append(password              );
        writeParameter.Write(QDir().currentPath() + "/EEconfig", saveData);

        break;
        }

    case ShiftSchedule:{
        QString saveData;

        QString Shifts = ui->comboBox_shifts->currentText();

        QString startShift1 = ui->label_startShift_1->text();
        QString endShift1 = ui->label_endShift_1->text();
        QString startShift2 = ui->label_startShift_2->text();
        QString endShift2 = ui->label_endShift_2->text();
        QString startShift3 = ui->label_startShift_3->text();
        QString endShift3 = ui->label_endShift_3->text();
        QString startShift4 = ui->label_startShift_4->text();
        QString endShift4 = ui->label_endShift_4->text();

        saveData = QString("%1\n%2\n%3\n%4\n%5\n%6\n%7\n%8\n%9")        \
                .arg(Shifts).arg(startShift1).arg(endShift1)            \
                .arg(startShift2).arg(endShift2)                        \
                .arg(startShift3).arg(endShift3)                        \
                .arg(startShift4).arg(endShift4);
        writeParameter.Write(QDir().currentPath() + "/ShiftSchedule", saveData);
        break;
    }
    }

    updateAllconfig();
}
void qualitymonitor::on_saveButton_clicked()
{
    toSaveData(normalParameter);
}
void qualitymonitor::on_saveEEpraButton_clicked()
{
    if(ui->Filter_1->text().toInt() > 99)
        ui->Filter_1->setText("99");
    if(ui->Filter_2->text().toInt() > 999)
        ui->Filter_2->setText("999");
    if(ui->OffsetAdjust->text().toInt() > 99)
        ui->OffsetAdjust->setText("99");
    toSaveData(EEParameter);
}

//frame switch
void qualitymonitor::on_IndexButton_clicked()
{
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
    mKeyboard->hideKeyboard();
    ui->frame_password->hide();
}
void qualitymonitor::on_LVDT_Button_clicked()
{
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
}
void qualitymonitor::on_parameter_Button_clicked()
{
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
    // set pre & next button
    ui->pushButton_parameter_nextpage->setEnabled(true);
    ui->pushButton_parameter_prepage->setEnabled(false);

    setupParameter();
}
void qualitymonitor::on_test_Button_clicked()
{
    ui->test->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }

}
void qualitymonitor::on_errorfram_Button_clicked()
{
    ui->errorhistory->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
}
void qualitymonitor::on_EEtestbutt_clicked()
{  //add type password
    ui->frame_password->show();
    whichFrameRequestPassword = 1;

    QPoint pt;
    QRect frame_passwordRect = ui->frame_password->geometry();
    QRect GlobalRect = QWidget::geometry();
    pt.setX(GlobalRect.x() + (GlobalRect.width() - frame_passwordRect.width()) / 2);
    pt.setY(GlobalRect.y() + (GlobalRect.height() - frame_passwordRect.height()) / 2);

    ui->frame_password->move(pt);

}
void qualitymonitor::on_pushButton_Settiing_clicked()
{
    ui->frame_password->show();
    whichFrameRequestPassword = 2;

    QPoint pt;
    QRect frame_passwordRect = ui->frame_password->geometry();
    QRect GlobalRect = QWidget::geometry();
    pt.setX(GlobalRect.x() + (GlobalRect.width() - frame_passwordRect.width()) / 2);
    pt.setY(GlobalRect.y() + (GlobalRect.height() - frame_passwordRect.height()) / 2);

    ui->frame_password->move(pt);

}

void qualitymonitor::on_pushButton_3_clicked()
{
    //pushbutton frame_search
    ui->frame_search->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }   
    ui->HistoryChart->graph(0)->data()->clear();
    ui->HistoryChart->graph(1)->data()->clear();
    ui->HistoryChart->replot();
}

void qualitymonitor::on_pushButton_shift_clicked()
{
    // shift frame
    ui->frame_setshift->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
    int HowManyShift = ui->comboBox_shifts->currentText().toInt();
    switch(HowManyShift)
    {
    case 2:
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(false);
        ui->pushButton_startplus_3->setEnabled(false);
        ui->pushButton_startminu_4->setEnabled(false);
        ui->pushButton_startplus_4->setEnabled(false);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endplus_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(false);
        ui->pushButton_endplus_3->setEnabled(false);
        ui->pushButton_endminu_4->setEnabled(false);
        ui->pushButton_endplus_4->setEnabled(false);

        ui->label_startShift_3->setNum(0);
        ui->label_endShift_3->setNum(0);
        ui->label_startShift_4->setNum(0);
        ui->label_endShift_4->setNum(0);

        ui->label_startShift_3->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : gray; font-size : 12pt");
        break;

    case 3:
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(true);
        ui->pushButton_startplus_3->setEnabled(true);
        ui->pushButton_startminu_4->setEnabled(false);
        ui->pushButton_startplus_4->setEnabled(false);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endplus_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(true);
        ui->pushButton_endplus_3->setEnabled(true);
        ui->pushButton_endminu_4->setEnabled(false);
        ui->pushButton_endplus_4->setEnabled(false);

        ui->label_startShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : gray; font-size : 12pt");

        break;
    case 4:
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(true);
        ui->pushButton_startplus_3->setEnabled(true);
        ui->pushButton_startminu_4->setEnabled(true);
        ui->pushButton_startplus_4->setEnabled(true);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endplus_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(true);
        ui->pushButton_endplus_3->setEnabled(true);
        ui->pushButton_endminu_4->setEnabled(true);
        ui->pushButton_endplus_4->setEnabled(true);

        ui->label_startShift_4->setNum(0);
        ui->label_endShift_4->setNum(0);

        ui->label_startShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : none; font-size : 12pt");


        break;
    }

    setupParameter();

}
void qualitymonitor::on_pushButton_OutputCenter_clicked()
{
    ui->frameOutputCenter->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }

    ui->Chart_DetectCenter_L->addGraph();
    ui->Chart_DetectCenter_R->addGraph();

    ui->Chart_DetectCenter_L->addGraph(ui->Chart_DetectCenter_L->xAxis, ui->Chart_DetectCenter_L->yAxis2);

    ui->Chart_DetectCenter_L->xAxis->setRange(0, DetectCenter_Length);
    ui->Chart_DetectCenter_R->xAxis->setRange(0, DetectCenter_Length);

    ui->Chart_DetectCenter_L->xAxis->setTickLabels(false);
    ui->Chart_DetectCenter_R->xAxis->setTickLabels(false);
    ui->Chart_DetectCenter_L->yAxis->setTickLabels(false);
    ui->Chart_DetectCenter_R->yAxis->setTickLabels(false);

    ui->Chart_DetectCenter_L->replot();
    ui->Chart_DetectCenter_R->replot();

    ui->pushButton_startDetect->setEnabled(true);
    ui->pushButton_startDetect->setText("Start");
}
void qualitymonitor::on_pushButton_clicked()
{
    //pushbutton home
    mKeyboard->hideKeyboard();
    ui->frame_password->hide();
}


void qualitymonitor::on_pushButton_SPG_clicked()
{
    //cal & set SPG
    //set_SPG_Chart();
    //timeid_replotSPG = startTimer(1000);
    ui->frame_SPG->show();
    ui->frame_SPG->move(this->geometry().topLeft());

    QRect FrameSPG_Rect = ui->frame_SPG->geometry();
    ui->frame_SPG->move(FrameSPG_Rect.x(), 480 - FrameSPG_Rect.height());

    set_SPG_Chart();

/*
    ui->frame_SPG->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
*/
}
void qualitymonitor::on_pushButton_7_clicked()
{
    //close frame SPG
    ui->frame_SPG->hide();
    //killTimer(this->timeid_replotSPG);
}



void qualitymonitor::on_pushButton_replot_clicked()
{
    //cal & set SPG
    set_SPG_Chart();
}


//display mode
void qualitymonitor::on_pushButton_5_clicked()
{
    if(ui->pushButton_5->text().operator==("Number"))
        ui->pushButton_5->setText("Graphics");
    else
        ui->pushButton_5->setText("Number");
}//end display mode

void qualitymonitor::on_pushButton_6_clicked()
{
    //exit button
    emit emit_adc_enable();
    killTimer(timeid_DateTime);
    killTimer(timeid_TrigCount);
    killTimer(timeid_GUI_ADC_Value);

    //exit(EXIT_FAILURE);
    //QApplication::closeAllWindows();
}
//set lvdt offset
void qualitymonitor::on_pushButton_out1offset_clicked()
{
    //QString Out1_offset = QString::number(QString(ui->out1_pos->text()).toInt() -1000);
    int Out1_offset = ui->out1_pos->text().toInt() -1000;
    ui->out1_offset->setNum(Out1_offset);
    ui->L_outputoffset->setText(QString::number(Out1_offset));
    toSaveData(normalParameter);
}
void qualitymonitor::on_pushButton_out2offset_clicked()
{
    //QString Out2_offset = QString::number(QString(ui->out2_pos->text()).toInt() -1000);
    int Out2_offset = ui->out2_pos->text().toInt() -1000;
    ui->out2_offset->setNum(Out2_offset);
    ui->R_outputoffset->setText(QString::number(Out2_offset));
    toSaveData(normalParameter);
}//end set lvdt offset


void qualitymonitor::fixed_xAxisRange(QCPRange boundRange)
{
    //while range over data range, fixed axis range
    if(boundRange.lower < 0){
        ui->HistoryChart->xAxis->setRange(0, boundRange.upper);
        //ui->HistoryChart->xAxis2->setRange(0, boundRange.upper);
    }
    if(boundRange.upper > historyData_x.last()){
        ui->HistoryChart->xAxis->setRange(boundRange.lower, historyData_x.last());
        //ui->HistoryChart->xAxis2->setRange(boundRange.lower, historyData_x.last());
    }
}

void qualitymonitor::fixed_yAxisRange(QCPRange)
{
    //ui->HistoryChart->yAxis->rescale();

    float Aper_Limit = 0;
    float CV_per_Limit = 0;

    if(ui->comboBox_SideChose->currentIndex() == 0)
    {
        Aper_Limit = ui->L_limit_Aper->text().toFloat() * 1.5;
        CV_per_Limit = ui->L_limit_CVper->text().toFloat() * 1.5;
    }
    else
    {
        Aper_Limit = ui->R_limit_Aper->text().toFloat() * 1.5;
        CV_per_Limit = ui->R_limit_CVper->text().toFloat() * 1.5;
    }
    ui->HistoryChart->yAxis->setRange(-Aper_Limit, Aper_Limit);
    ui->HistoryChart->yAxis2->setRange(-0.5, CV_per_Limit);
}


void qualitymonitor::timerEvent(QTimerEvent *event)
{
    //clock
    if(event->timerId() == timeid_DateTime){
        DateTimeSlot();

        Write_newData(LeftSide_SPG);
        Write_newData(RightSide_SPG);
    }
    /*********************************************
     * About alarm I got two options:
     * 1. two side seperate
     * 2. combine
     * now I test is 2nd option.
     * *******************************************/
    if (event->timerId() == timeid_Alarm){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overAper_L | overAper_R)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_Alarm);
                //clear CV data array
                CV_1m.clear();
                CV_1m_R.clear();

                on_pushButton_ErrorSig_clicked("Over A%");

                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlag = 0;
            }
    }
    else if (event->timerId() == timeid_AlarmofCV){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overCV_per_L | overCV_per_R)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_AlarmofCV);
                //clear CV data array
                CV_1m.clear();
                CV_1m_R.clear();

                on_pushButton_ErrorSig_clicked("Over CV%");
                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlagofCV = 0;
            }
    }
    /*
    //over limit
    else if (event->timerId() == timeid_Alarm){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overAper_L)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_Alarm);
                //clear CV data array
                CV_1m.clear();
                CV_1m_R.clear();

                on_pushButton_ErrorSig_clicked("Left side over A%");

                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlag = 0;
            }
    }
    if (event->timerId() == timeid_Alarm_R){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overAper_R)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_Alarm_R);
                //clear CV data array
                CV_1m.clear();
                CV_1m_R.clear();

                on_pushButton_ErrorSig_clicked("Right side over A%");

                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlag = 0;
            }
    }
    else if (event->timerId() == timeid_AlarmofCV){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overCV_per_L)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_AlarmofCV);
                //clear CV data array
                CV_1m.clear();
                CV_1m_R.clear();

                on_pushButton_ErrorSig_clicked("Left side over CV%");
                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlagofCV = 0;
            }
    }
    else if (event->timerId() == timeid_AlarmofCV_R){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overCV_per_L)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_AlarmofCV_R);
                //clear CV data array
                CV_1m.clear();
                CV_1m_R.clear();

                on_pushButton_ErrorSig_clicked("Right side over CV%");
                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlagofCV = 0;
            }
    }
    */
    //ISR counter
    else if(event->timerId() == timeid_TrigCount){
        count_ISR_times();
        //qDebug() << mAD->readADCvalue();
    }

    if(event->timerId() == timeid_GUI_ADC_Value)
    {
        on_Receive_ADval();
    }
    if(event->timerId() == timeid_DetectCenter)
    {
        ui->Chart_DetectCenter_L->yAxis->rescale();
        ui->Chart_DetectCenter_R->yAxis->rescale();

        ui->Chart_DetectCenter_L->replot();
        ui->Chart_DetectCenter_R->replot();
    }

    if(event->timerId() == timeid_pressed_toStart)
        if(ui->pushButton_on_runframe_setoutputcenter->isDown())
            on_pushButton_on_runframe_setoutputcenter_pressed_3s();


}






/***********************************************************************************
MyTrigger::MyTrigger()
{
    gpioInitialise();
}
void MyTrigger::run()
{
    qualitymonitor Set_GraphicsView;
    qDebug() << "mtrig: " << thread()->currentThreadId();
    //Set_GraphicsView.Set_GraphicsView();
    emit emit_trig_sig();
}
*************************************************************************************/

void qualitymonitor::on_reset_clicked()
{
    //test
    //emit reset sig to arduino
    CV_1m.clear();
    gpioWrite(Stop_signal, PI_LOW);     //reset stop signal
    ui->label_stopFlag->setNum(0);
}

void qualitymonitor::on_pushbutton_QMenble_clicked()
{
    //change QM enable button color
    if(ui->pushbutton_QMenble->isChecked())
        ui->pushbutton_QMenble->setStyleSheet("background : green ; color : white");
    else
        ui->pushbutton_QMenble->setStyleSheet("background : red ; color : white");
}



void qualitymonitor::on_pushButton_historysearch_clicked()
{
    QStringList DateFilters;
    QDate StartDate = ui->dateEdit_HistoryStartDate->date();
    QDate EndDate   = ui->dateEdit_HistoryEndDate->date();
    if(StartDate > EndDate)
    {
        //QMessageBox::warning(this, "Error!!", "Search Logic ERROR!",QMessageBox::Yes, this,Qt::FramelessWindowHint);
        QMessageBox warning;
        warning.setText("Search Logic ERROR!");
        warning.setWindowFlags(Qt::WindowDoesNotAcceptFocus | Qt::FramelessWindowHint |
                               Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
        warning.setIcon(QMessageBox::Warning);
        warning.exec();

        qDebug () << "Search Logic ERROR!";
    }
    while(StartDate <= EndDate)
    {
        DateFilters.append(StartDate.toString("yyyy-MM-dd"));
        StartDate = StartDate.addDays(1);
    }
    History_filemodel->setNameFilters(DateFilters);
    History_filemodel->setNameFilterDisables(false);
}

void qualitymonitor::on_checkBox_A_per_stateChanged(int arg1)
{
    //qDebug() << arg1;

    if(arg1 == 0)
    {
        //hide A% path
        ui->HistoryChart->graph(0)->setVisible(false);
        ui->HistoryChart->replot();
    }
    else{
        //show A% path
        ui->HistoryChart->graph(0)->setVisible(true);
        ui->HistoryChart->replot();
    }
}

void qualitymonitor::on_checkBox_CV_stateChanged(int arg1)
{
    //qDebug() << arg1;
    if(arg1 == 0)
    {
        //hide CV% path
        ui->HistoryChart->graph(1)->setVisible(false);
        ui->HistoryChart->replot();
    }
    else{
        //show CV% path
        ui->HistoryChart->graph(1)->setVisible(true);
        ui->HistoryChart->replot();
    }
}

void qualitymonitor::on_pushButton_Passwordcancel_clicked()
{
    //password cancel
    ui->frame_password->hide();
    mKeyboard->hideKeyboard();
    ui->lineEdit_PasswordInput->clear();
}

void qualitymonitor::on_pushButton_PasswordOK_clicked()
{
    //password ok
    QString InputPassword = ui->lineEdit_PasswordInput->text();
    bool ok = InputPassword == ui->lineEdit_password->text();
    if(ok)
    {
        //correct password
        switch (whichFrameRequestPassword) {
            case 1:
            // raise EEparameter frame
                ui->EEparameter->raise();
                if(!ui->Dateframe->isTopLevel()){
                    ui->Dateframe->raise();
                    ui->MenuFrame->raise();
                    setupParameter();
                }
            break;
            case 2:
            // raise Setting frame
                ui->SettingFrame->raise();
                if(!ui->Dateframe->isTopLevel()){
                    ui->Dateframe->raise();
                    ui->MenuFrame->raise();
                }
                setupParameter();
            break;
        }

        ui->frame_password->hide();
    }
    else
    {
        //wrong password
        QMessageBox warning;
        warning.setText("Wrong Password!");
        // allow password frame stay focus
        warning.setWindowFlags(Qt::FramelessWindowHint |     \
                               Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
        warning.setIcon(QMessageBox::Warning);
        warning.exec();
    }
    ui->lineEdit_PasswordInput->clear();
    mKeyboard->hideKeyboard();
}


void qualitymonitor::on_pushButton_SettingSave_clicked()
{

    QMessageBox warning;
    QString warningInfo;
    warningInfo = QString("<pre align='center' style='font-family:Arial' style='font-size:20pt'>"   \
                          "If changing Date or Time < br>might ERASE all of history or error record!!</pre>");
    warning.setText(warningInfo);
    warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

    warning.setCursor(Qt::BlankCursor);
    warning.setIcon(QMessageBox::Warning);
    warning.setWindowFlags(Qt::FramelessWindowHint |   \
                                     Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
    if(warning.exec() == QMessageBox::Yes)
    {
        //save setting frame data
        toSaveData(EEParameter);

        // set time to system time
        QDate Date = ui->dateEdit->date();
        QTime time = QTime(ui->label_hh->text().toInt(), ui->label_mm->text().toInt());

        QString set_time = QString("timedatectl set-time '%1'").arg(Date.toString("yyyy-MM-dd ") + time.toString("hh:mm:ss"));

        char* cmd = set_time.toLocal8Bit().data();
        system(cmd);
        // rst time adjust
        set_timeHour = 0;
        set_timeMinu = 0;
    }
}

void qualitymonitor::on_pushButton_centerConfirm_clicked()
{
    // push Center Confirm button
    if(!(ui->lineEdit_LeftCenter->text().isEmpty() | ui->lineEdit_RightCenter->text().isEmpty()))
    {
        float currentWeight_L   = ui->L_outputweight->text().toFloat();
        float currentWeight_R   = ui->R_outputweight->text().toFloat();
        int currentCenter_L     = ui->label_center_L_test->text().toFloat();
        int currentCenter_R     = ui->label_center_R_test->text().toFloat();

        int savedCenter_L     = ui->L_feedoutcenter->text().toFloat();
        int savedCenter_R     = ui->R_feedoutcenter->text().toFloat();

        float newWeight_L = ui->lineEdit_LeftCenter->text().toFloat();
        float newWeight_R = ui->lineEdit_RightCenter->text().toFloat();

        int newCenter_L = currentCenter_L * newWeight_L / currentWeight_L;
        int newCenter_R = currentCenter_R * newWeight_R / currentWeight_R;

        //qDebug() << newCenter_L << newCenter_R;
        // leak threshold

        QMessageBox CheckChangeCenter;
        CheckChangeCenter.setIcon(QMessageBox::Warning);
        CheckChangeCenter.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
        CheckChangeCenter.setDefaultButton(QMessageBox::Yes);

        CheckChangeCenter.setCursor(Qt::BlankCursor);
        CheckChangeCenter.setWindowFlags(Qt::FramelessWindowHint |   \
                                         Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);

        QString ChangeInfo;
        ChangeInfo = QString("<pre align='center' style='font-family:Arial'><p style='font-size:20pt'>"
                             "<b> Left &#9; Right</b></p>"

                             "<p style='font-size:16pt'>"
                             "%1 &#9; %2<br>"
                             "↓ &#9; ↓</p>"

                             "<p style='color:red' style='font-size:16pt'>"
                             "%3 &#9; %4</p></pre>").arg(savedCenter_L).arg(savedCenter_R).arg(newCenter_L).arg(newCenter_R);

        CheckChangeCenter.setText(ChangeInfo);

        //CheckChangeCenter.exec();
        /***************************************
         * ChangeInfo is used html to write:
         *
         *    Left     Right
         *  current   current
         *     ↓         ↓
         *  changed   changed
         * *************************************/
        if(CheckChangeCenter.exec() == QMessageBox::Yes)
        {
            ui->L_feedoutcenter->setText(QString::number(newCenter_L));
            ui->R_feedoutcenter->setText(QString::number(newCenter_R));
            //ui->label_center_L_test->text().clear();
            //ui->label_center_L_test->text().clear();

            toSaveData(normalParameter);

            ui->pushButton_startDetect->setEnabled(true);
            ui->pushButton_startDetect->setText("Start");
        }
        ui->lineEdit_LeftCenter->text().clear();
        ui->lineEdit_LeftCenter->text().clear();
    }
    else
    {
    /*
        QMessageBox warning;
        warning.setText("Please type in new weight");
        warning.setWindowFlags(Qt::WindowDoesNotAcceptFocus | Qt::FramelessWindowHint |     \
                               Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
        warning.setIcon(QMessageBox::Warning);
        warning.exec();

    */
    }
}

void qualitymonitor::on_pushButton_startDetect_clicked()
{
    // start detecting center
    // clear old data array
    DetectCenter_L.clear();
    DetectCenter_R.clear();
    DetectCenter_axixX_L.clear();
    DetectCenter_axixX_R.clear();

    ui->Chart_DetectCenter_L->replot();
    ui->Chart_DetectCenter_R->replot();

    // set detect flag to true
    timeid_DetectCenter = startTimer(100);
    ui->pushButton_startDetect->setText("Detecting...");
    ui->pushButton_startDetect->setEnabled(false);
    is_DetectCenter = true;
}

void qualitymonitor::setup_shiftSchedule()
{
    QFile config(QDir().currentPath() + "/ShiftSchedule");
    if(!config.exists())
    {
        // defalut shifts = 3
        ui->comboBox_shifts->setCurrentText("3");
        // set shift 4 to unable
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(true);
        ui->pushButton_startplus_3->setEnabled(true);
        ui->pushButton_startminu_4->setEnabled(false);
        ui->pushButton_startplus_4->setEnabled(false);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(true);
        ui->pushButton_endminu_4->setEnabled(false);
        ui->pushButton_endplus_4->setEnabled(false);

        ui->label_startShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : gray; font-size : 12pt");

        // list all shift schedule combobox in frame_shift
        /*
        QList<QComboBox*> AllshiftCombobox = this->findChildren<QComboBox*>();
        //qDebug() << AllshiftCombobox;
        foreach(QComboBox* itm, AllshiftCombobox)
        {
            // if combobox's object name has "startshift" or "endshift", then set item to 0 to 23
            if(itm->objectName().contains("startShift") || itm->objectName().contains("endShift"))
                for(int i = 0; i < 24; i++)
                    itm->addItem(QString::number(i));
        }
    */
    }
}


void qualitymonitor::on_comboBox_shifts_currentIndexChanged(const QString &arg1)
{
    switch(arg1.toInt())
    {
    case 2:
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(false);
        ui->pushButton_startplus_3->setEnabled(false);
        ui->pushButton_startminu_4->setEnabled(false);
        ui->pushButton_startplus_4->setEnabled(false);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endplus_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(false);
        ui->pushButton_endplus_3->setEnabled(false);
        ui->pushButton_endminu_4->setEnabled(false);
        ui->pushButton_endplus_4->setEnabled(false);

        ui->label_startShift_3->setNum(0);
        ui->label_endShift_3->setNum(0);
        ui->label_startShift_4->setNum(0);
        ui->label_endShift_4->setNum(0);

        ui->label_startShift_3->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : gray; font-size : 12pt");

        ui->label_startShift_3->setNum(0);
        ui->label_endShift_3->setNum(0);
        ui->label_startShift_4->setNum(0);
        ui->label_endShift_4->setNum(0);
        break;

    case 3:
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(true);
        ui->pushButton_startplus_3->setEnabled(true);
        ui->pushButton_startminu_4->setEnabled(false);
        ui->pushButton_startplus_4->setEnabled(false);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endplus_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(true);
        ui->pushButton_endplus_3->setEnabled(true);
        ui->pushButton_endminu_4->setEnabled(false);
        ui->pushButton_endplus_4->setEnabled(false);

        ui->label_startShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : gray; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : gray; font-size : 12pt");

        ui->label_startShift_4->setNum(0);
        ui->label_endShift_4->setNum(0);
        break;
    case 4:
        ui->pushButton_startminu_2->setEnabled(true);
        ui->pushButton_startplus_2->setEnabled(true);
        ui->pushButton_startminu_3->setEnabled(true);
        ui->pushButton_startplus_3->setEnabled(true);
        ui->pushButton_startminu_4->setEnabled(true);
        ui->pushButton_startplus_4->setEnabled(true);

        ui->pushButton_endminu_2->setEnabled(true);
        ui->pushButton_endplus_2->setEnabled(true);
        ui->pushButton_endminu_3->setEnabled(true);
        ui->pushButton_endplus_3->setEnabled(true);
        ui->pushButton_endminu_4->setEnabled(true);
        ui->pushButton_endplus_4->setEnabled(true);

        ui->label_startShift_4->setNum(0);
        ui->label_endShift_4->setNum(0);

        ui->label_startShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_3->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_startShift_4->setStyleSheet("background-color : none; font-size : 12pt");
        ui->label_endShift_4->setStyleSheet("background-color : none; font-size : 12pt");


        break;
    }
}




void qualitymonitor::on_pushButton_saveShiftSchedule_clicked()
{
    toSaveData(ShiftSchedule);

    startshift1 = ui->label_startShift_1->text().toInt();
    endshift1 = ui->label_endShift_1->text().toInt();
    startshift2 = ui->label_startShift_2->text().toInt();
    endshift2 = ui->label_endShift_2->text().toInt();
    startshift3 = ui->label_startShift_3->text().toInt();
    endshift3 = ui->label_endShift_3->text().toInt();
    startshift4 = ui->label_startShift_4->text().toInt();
    endshift4 = ui->label_endShift_4->text().toInt();
}

void qualitymonitor::on_pushButton_startminu_1_clicked()
{
    int hour = ui->label_startShift_1->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_startShift_1->setNum(hour);
}

void qualitymonitor::on_pushButton_startplus_1_clicked()
{
    int hour = ui->label_startShift_1->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_startShift_1->setNum(hour);
}

void qualitymonitor::on_pushButton_endminu_1_clicked()
{
    int hour = ui->label_endShift_1->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_endShift_1->setNum(hour);
}

void qualitymonitor::on_pushButton_endplus_1_clicked()
{
    int hour = ui->label_endShift_1->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_endShift_1->setNum(hour);
}

void qualitymonitor::on_pushButton_startminu_2_clicked()
{
    int hour = ui->label_startShift_2->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_startShift_2->setNum(hour);
}

void qualitymonitor::on_pushButton_startplus_2_clicked()
{
    int hour = ui->label_startShift_2->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_startShift_2->setNum(hour);
}

void qualitymonitor::on_pushButton_endminu_2_clicked()
{
    int hour = ui->label_endShift_2->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_endShift_2->setNum(hour);
}

void qualitymonitor::on_pushButton_endplus_2_clicked()
{
    int hour = ui->label_endShift_2->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_endShift_2->setNum(hour);
}

void qualitymonitor::on_pushButton_endminu_3_clicked()
{
    int hour = ui->label_endShift_3->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_endShift_3->setNum(hour);
}

void qualitymonitor::on_pushButton_endplus_3_clicked()
{
    int hour = ui->label_endShift_3->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_endShift_3->setNum(hour);
}

void qualitymonitor::on_pushButton_startminu_3_clicked()
{
    int hour = ui->label_startShift_3->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_startShift_3->setNum(hour);
}

void qualitymonitor::on_pushButton_startplus_3_clicked()
{
    int hour = ui->label_startShift_3->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_startShift_3->setNum(hour);
}

void qualitymonitor::on_pushButton_startminu_4_clicked()
{
    int hour = ui->label_startShift_4->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_startShift_4->setNum(hour);
}

void qualitymonitor::on_pushButton_startplus_4_clicked()
{
    int hour = ui->label_startShift_4->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_startShift_4->setNum(hour);
}

void qualitymonitor::on_pushButton_endminu_4_clicked()
{
    int hour = ui->label_endShift_4->text().toInt();
    hour--;
    if(hour < 0)
        hour = 23;
    ui->label_endShift_4->setNum(hour);
}

void qualitymonitor::on_pushButton_endplus_4_clicked()
{
    int hour = ui->label_endShift_4->text().toInt();
    hour++;
    if(hour > 23)
        hour = 0;
    ui->label_endShift_4->setNum(hour);
}



void qualitymonitor::on_pushButton_parameter_nextpage_clicked()
{
    ui->frame_parameter_2->raise();
    // set pre & next button
    ui->pushButton_parameter_nextpage->setEnabled(false);
    ui->pushButton_parameter_prepage->setEnabled(true);
}

void qualitymonitor::on_pushButton_parameter_prepage_clicked()
{
    ui->frame_parameter_2->lower();
    // set pre & next button
    ui->pushButton_parameter_nextpage->setEnabled(true);
    ui->pushButton_parameter_prepage->setEnabled(false);
}

void qualitymonitor::on_pushButton_hh_plus_clicked()
{
    // set-time hour ++
    set_timeHour++;
    // call back slot to update time
    DateTimeSlot();
}

void qualitymonitor::on_pushButton_hh_minus_clicked()
{
    // set-time hour --
    set_timeHour--;
    // call back slot to update time
    DateTimeSlot();
}

void qualitymonitor::on_pushButton_mm_plus_clicked()
{
    // set-time min ++
    set_timeMinu++;
    // call back slot to update time
    DateTimeSlot();

}

void qualitymonitor::on_pushButton_mm_minus_clicked()
{
    // set-time min --
    set_timeMinu--;
    // call back slot to update time
    DateTimeSlot();
}



void qualitymonitor::on_pushButton_on_runframe_setoutputcenter_pressed()
{
    timeid_pressed_toStart = startTimer(3000);
}

void qualitymonitor::on_pushButton_on_runframe_setoutputcenter_pressed_3s()
{
    this->killTimer(timeid_pressed_toStart);
    int savedCenter_L     = ui->L_feedoutcenter->text().toFloat();
    int savedCenter_R     = ui->R_feedoutcenter->text().toFloat();
    int newCenter_L = 0;
    int newCenter_R = 0;

    foreach(int itm ,on_RunFrame_Set_OutputCenter_L)
        newCenter_L += itm / on_RunFrame_Set_OutputCenter_L.length();

    foreach(int itm ,on_RunFrame_Set_OutputCenter_R)
        newCenter_R += itm / on_RunFrame_Set_OutputCenter_R.length();

    QMessageBox CheckChangeCenter;
    CheckChangeCenter.setIcon(QMessageBox::Warning);
    CheckChangeCenter.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    CheckChangeCenter.setDefaultButton(QMessageBox::Yes);


    CheckChangeCenter.setCursor(Qt::BlankCursor);
    CheckChangeCenter.setWindowFlags(Qt::FramelessWindowHint |   \
                                     Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);

    QString ChangeInfo;
    ChangeInfo = QString("<pre align='center' style='font-family:Arial'><p style='font-size:20pt'>"
                         "<b> Make sure current Weight is ideal.<br>"
                         "Left &#9; Right</b></p>"

                         "<p style='font-size:16pt'>"
                         "%1 &#9; %2<br>"
                         "↓ &#9; ↓</p>"

                         "<p style='color:red' style='font-size:16pt'>"
                         "%3 &#9; %4</p></pre>").arg(savedCenter_L).arg(savedCenter_R).arg(newCenter_L).arg(newCenter_R);

    CheckChangeCenter.setText(ChangeInfo);

    //CheckChangeCenter.exec();
    /***************************************
     * ChangeInfo is used html to write:
     *
     *    Left     Right
     *  current   current
     *     ↓         ↓
     *  changed   changed
     * *************************************/
    if(CheckChangeCenter.exec() == QMessageBox::Yes)
    {
        ui->L_feedoutcenter->setText(QString::number(newCenter_L));
        ui->R_feedoutcenter->setText(QString::number(newCenter_R));
        //ui->label_center_L_test->text().clear();
        //ui->label_center_L_test->text().clear();

        toSaveData(normalParameter);

        ui->pushButton_startDetect->setEnabled(true);
        ui->pushButton_startDetect->setText("Start");
    }
}
