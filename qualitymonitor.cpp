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

#include <QtCharts>
using namespace QtCharts;

#include "mythread.h"
#include "parameter.h"
#include "adread.h"
#include "pigpio.h"
#include <time.h>
#include "qcustomplot.h"

//parameter read index
#define  normalParameter    1
#define  EEParameter        2

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

//setup parameter
    setupParameter();
    SetErrorTable();
//set runframe on toplevel
    ui->runframe->raise();
    ui->Dateframe->raise();
    //qDebug() << "main: " << QThread::currentThread();
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
    mAD->start();
    timeid_GUI_ADC_Value = startTimer(30);
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


    //creat keyboard dialog
    set_Keyboard();
}
void qualitymonitor::Running(int gpio, int level, uint32_t tick)
{
    if(level == 0)
    {
        //change to low (a falling edge)
        //stop
        //qDebug() << "stop";
    }
    else if(level == 1)
    {
        //change to high (a rising edge)
        //run
        //qDebug() << "run";
        ISR_excute_ptr->RunDateTime = QDateTime().currentDateTime().toString("yyyy-MM-dd");
        qDebug() << ISR_excute_ptr->RunDateTime;

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
    uint AD_value_L = AD_value >> 16 & 0x7fff;
    uint AD_value_R = AD_value & 0x00007fff;
    ui->out1_pos->setText(QString::number(AD_value_L));
    ui->out2_pos->setText(QString::number(AD_value_R));

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

    //set testframe ADC value
    // AD7606 each LSB = 0.000152 uV
    ui->test_inputL->setText(QString::number(AD_value_L* 0.000152));
    ui->test_inputR->setText(QString::number(AD_value_R* 0.000152));


    QString AD_L = QString::number(AD_value_L, 2);
    QString AD_R = QString::number(AD_value_R, 2);

    //AD_L = QString().setNum(AD_L.toInt());

    ui->label_Binary_L->setText(AD_L);
    ui->label_Binary_R->setText(AD_R);

    //ui->test_inputL->setText(QString::number(AD_value_L* 3.093* 2/ 4096, 'f',2));
    //ui->test_inputR->setText(QString::number(AD_value_R* 3.093* 2/ 4096, 'f',2));

}
void qualitymonitor::on_Receive_Trig()
{   //prepare to delete
    //Set_GraphicsView();
    //qDebug() << "on_Receive_Trig :" << thread()->currentThreadId();
}

void qualitymonitor::RunFrame_Display(float AD_value_L, float AD_value_R)
{

    //set runframe A%
    ui->label_L_A_per->setText("A%: "+QString::number(Mymathtool.A_per(ui->L_feedoutcenter->text(), AD_value_L), 'f', 2)+" %");
    ui->label_R_A_per->setText("A%: "+QString::number(Mymathtool.A_per(ui->R_feedoutcenter->text(), AD_value_R), 'f', 2)+" %");

    //change runframe A% text color
    if(qAbs(Mymathtool.A_per(ui->L_feedoutcenter->text(), AD_value_L)) > ui->L_limit_Aper->text().toFloat()*0.65)
    {
        if(qAbs(Mymathtool.A_per(ui->L_feedoutcenter->text(), AD_value_L)) > ui->L_limit_Aper->text().toFloat())
            ui->label_L_A_per->setStyleSheet("color : red ; font-size : 15px");
        else
            ui->label_L_A_per->setStyleSheet("color : yellow ; font-size : 15px");
    }
    else
        ui->label_L_A_per->setStyleSheet("color : green ; font-size : 15px");

    if(qAbs(Mymathtool.A_per(ui->R_feedoutcenter->text(), AD_value_R)) > ui->R_limit_Aper->text().toFloat()*0.65)
    {
        if(qAbs(Mymathtool.A_per(ui->R_feedoutcenter->text(), AD_value_R)) > ui->R_limit_Aper->text().toFloat())
            ui->label_R_A_per->setStyleSheet("color : red ; font-size : 15px");
        else
            ui->label_R_A_per->setStyleSheet("color : yellow ; font-size : 15px");
    }
    else
        ui->label_R_A_per->setStyleSheet("color : green ; font-size : 15px");
}
void qualitymonitor::slot()
{
    //each trig do once
    //
    /************************************************************
     *
     * R side still NOT added
     *
     * ***********************************************************/
    int AD_value = mAD->readADCvalue();
    float AD_value_L = AD_value >> 16 & 0x7fff;
    float AD_value_R = AD_value & 0x00007fff;
    //qDebug() << AD_value;
    static float avg_AD_value_1cm = 0;
    static float avg_AD_value_forGUI = 0;
    static float avg_AD_value_forWrite = 0;

    static float avg_AD_value_1cm_R = 0;
    static float avg_AD_value_forGUI_R = 0;
    static float avg_AD_value_forWrite_R = 0;

    //SampleTimes is count trig times
    //static int SampleTimes = 0;

    //static double datalenght_L = ;
    //static double datalenght_R = ui->Chart_R->xAxis->range().upper;

    static int CV1m_times = 0;
    static int CV10m_times = 0;

    static float avg_CV1m = 0;
    static float avg_CV1m_R = 0;

    static float avg_CV10m = 0;
    static float avg_CV100m = 0;

    static double SampleTimes[3] = {0};
    /* *********************************
     * SampleTimes[0] = SPG's flag
     * SampleTimes[1] = GUI's flag(A% CV1m)
     * SampleTimes[2] = A% CV 100m datawrite
     * *********************************/
    static int tmp = ui->PulseLength->text().toFloat();
    int OneCentimeterSampleTimes =0;
    if(tmp != 0)
        OneCentimeterSampleTimes =  10 / tmp; //unit = mm // (how many sample times / 1cm)

    //GUI_SampleLength = 100cm
    //GUI A% update every 10m
    //GUI CV1m update every 10m
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
    avg_AD_value_forWrite += AD_value_L / Datawrite_SampleLength / OneCentimeterSampleTimes;

    avg_AD_value_1cm_R      += AD_value_R / OneCentimeterSampleTimes;
    avg_AD_value_forGUI_R   += AD_value_R / GUI_SampleLength / OneCentimeterSampleTimes;
    avg_AD_value_forWrite_R += AD_value_R / Datawrite_SampleLength / OneCentimeterSampleTimes;

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
        //Write_newData(Write_SPG);
    }
    if(SPG_Data_R.length() >= SPG_SampleLength){
        SPG_Data_R.removeFirst();
        DataWrite_SPG_R.removeFirst();
        //Write_newData(Write_SPG);
    }

    if(SPG_Flag)
    {
        //every 1cm store AD_value
        SPG_Data.append(avg_AD_value_1cm);
        SPG_Data_R.append(avg_AD_value_1cm_R);

        DataWrite_SPG.append(QString::number(avg_AD_value_1cm));
        DataWrite_SPG_R.append(QString::number(avg_AD_value_1cm_R));

        CV_1m.append(avg_AD_value_1cm);
        CV_1m_R.append(avg_AD_value_1cm_R);

        /************test******************/
        ui->label_test1->setNum(avg_AD_value_1cm);
        /**********************************/

        avg_AD_value_1cm = 0;   //clear avg_AD_value_1cm
        avg_AD_value_1cm_R = 0;   //clear avg_AD_value_1cm_R

        SampleTimes[0] = 0; //reset flag

        SampleTimes[1]++;
        SampleTimes[2]++;

        bool GUI_Flag = SampleTimes[1] == GUI_SampleLength;

        if(GUI_Flag)
        {
            //every 100cm do once
            float A_per = (avg_AD_value_forGUI - Feedoutcenter_L) / Feedoutcenter_L *100;
            float A_per_R = (avg_AD_value_forGUI_R - Feedoutcenter_R) / Feedoutcenter_R *100;

            //qDebug() << A_per;
            //ui->label->setText(QString::number(A_per, 'f', 2)+" %");                //remove later; used to debug
            //qDebug() << CV_1m.length(); 
            float SD = 0;
            float SD_R = 0;

            foreach(double CV_number, CV_1m)
                SD += pow(CV_number - avg_AD_value_forGUI , 2);

            foreach(double CV_number, CV_1m_R)
                SD_R += pow(CV_number - avg_AD_value_forGUI_R , 2);
            /*
            for (int i = 0; i < GUI_SampleLength; i++)
                SD += pow(CV_1m.at(GUI_SampleLength - i) - avg_AD_value_forGUI, 2);
            */
            SD = sqrt(SD / (CV_1m.length() - 1)) / avg_AD_value_forGUI *100;
            SD_R = sqrt(SD_R / (CV_1m_R.length() - 1)) / avg_AD_value_forGUI_R *100;

            ui->label_L_CV1m->setText("1mCV%: " +QString::number(SD, 'f', 2) + "%");
            ui->label_R_CV1m->setText("1mCV%: " +QString::number(SD_R, 'f', 2) + "%");

            avg_CV1m += SD/100;
            avg_CV1m_R += SD_R/100;

            /************test******************/
            ui->label_test2->setNum(avg_AD_value_forGUI);
            /**********************************/
            RunFrame_Display(AD_value_L, AD_value_R);

            //Over A% limit more than 3s, emit STOP sig.
            overAper_L = qAbs(A_per) >= ui->L_limit_Aper->text().toFloat();
            overAper_R = qAbs(A_per_R) >= ui->R_limit_Aper->text().toFloat();

            //qDebug() << overAper;
            if(overAper_L)
                if(AlarmFlag == 0)
                {
                    timeid_Alarm = startTimer(3000);
                    AlarmFlag = 1;
                    //qDebug() << "alarm";
                }

            //Over 1mCV% limit more than 5s, emit STOP sig.
            overCV_per_L = SD > ui->L_limit_CVper->text().toFloat();
            overCV_per_R = SD_R > ui->R_limit_CVper->text().toFloat();

            if(overCV_per_L)
                if(AlarmFlagofCV == 0)
                {
                    timeid_AlarmofCV = startTimer(5000);
                    AlarmFlagofCV = 1;
                }

            CV_1m.clear();
            CV_1m_R.clear();

            avg_AD_value_forGUI = 0;    //clear avg_AD_value_forGUI
            avg_AD_value_forGUI_R = 0;    //clear avg_AD_value_forGUI

            SampleTimes[1] = 0; //reset flag
            bool DataWrite_Flag =   SampleTimes[2] == Datawrite_SampleLength;


            if(DataWrite_Flag)
            {
                A_per = (avg_AD_value_forWrite - Feedoutcenter_L) / Feedoutcenter_L *100;
                A_per_R = (avg_AD_value_forWrite_R - Feedoutcenter_R) / Feedoutcenter_R *100;

                //set Data to be Writed, and to be plot
                /************test******************/
                ui->label_test3->setNum(avg_AD_value_forWrite);
                /**********************************/

                //datalenght_L's unit is meter
                datalenght_L += Datawrite_SampleLength / 100;
                datalenght_R += Datawrite_SampleLength / 100;


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
                if(datalenght_L >= 30000){
                    ui->Chart_L->xAxis->setRange(datalenght_L-30000, datalenght_L);
                    //ui->Chart_L->xAxis2->setRange(datalenght_L-30000, datalenght_L);
                }
                else{
                    ui->Chart_L->xAxis->setRange(0, 30000);
                    //ui->Chart_L->xAxis2->setRange(0, 30000);
                }

                if(datalenght_R >= 30000){
                    ui->Chart_R->xAxis->setRange(datalenght_R-30000, datalenght_R);
                    //ui->Chart_R->xAxis2->setRange(datalenght_R-30000, datalenght_R);
                }
                else{
                    ui->Chart_R->xAxis->setRange(0, 30000);
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
                    i+=100;
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
                    i+=100;
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
void qualitymonitor::Write_newData(int index)
{
    /*******************************************************
     * Right side need to modify
     * ****************************************************/
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


    datalenght_L = axixX_L.length()*100;        //axixX_L's unit is cm, need to transfer to m.
    //qDebug() << datalenght_L;
    //clear array, release memory
    oldData_Aper_L.clear();
    oldData_CV_L.clear();
    axixX_L.clear();

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

    // set axes ranges:
    if(datalenght >= 30000){
        ui->Chart_L->xAxis->setRange(datalenght-30000, datalenght);
        //ui->Chart_L->xAxis2->setRange(datalenght-30000, datalenght);
    }
    else{
        ui->Chart_L->xAxis->setRange(0, 30000);
        //ui->Chart_L->xAxis2->setRange(0, 30000);
    }

    float L_Aper_Limit = ui->L_limit_Aper->text().toFloat() * 1.5;
    float CV_per_Limit = ui->L_limit_CVper->text().toFloat() * 1.5;

    ui->Chart_L->yAxis->setRange(-L_Aper_Limit, L_Aper_Limit);
    ui->Chart_L->yAxis2->setRange(-0.5, CV_per_Limit);

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


    datalenght_R = axixX_R.length()*100;        //axixX_L's unit is cm, need to transfer to m.
    //qDebug() << datalenght_L;
    //clear array, release memory
    oldData_Aper_R.clear();
    oldData_CV_R.clear();
    axixX_R.clear();
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

    // set axes ranges:
    if(datalenght >= 30000){
        ui->Chart_R->xAxis->setRange(datalenght-30000, datalenght);
        //ui->Chart_R->xAxis2->setRange(datalenght-30000, datalenght);
    }
    else{
        ui->Chart_R->xAxis->setRange(0, 30000);
        //ui->Chart_R->xAxis2->setRange(0, 30000);
    }

    float R_Aper_Limit = ui->R_limit_Aper->text().toFloat() * 1.5;
    float CV_per_Limit = ui->R_limit_CVper->text().toFloat() * 1.5;

    ui->Chart_R->yAxis->setRange(-R_Aper_Limit, R_Aper_Limit);
    ui->Chart_R->yAxis2->setRange(-0.5, CV_per_Limit);

    ui->Chart_R->replot();
}



void qualitymonitor::Setup_HistoryChart()
{
    ui->HistoryChart->addGraph();    
    ui->HistoryChart->addGraph(ui->HistoryChart->xAxis, ui->HistoryChart->yAxis2);

    ui->HistoryChart->xAxis->setRange(0, 50000);
    ui->HistoryChart->yAxis->setRange(-0.5, 10);

    //set chart can Drag, Zoom & Selected
    ui->HistoryChart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    //set CV yAxis visible
    ui->HistoryChart->yAxis2->setVisible(true);
    ui->HistoryChart->yAxis2->setTickLabelColor(Qt::blue);

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
            i+=100;
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
            i+=100;
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

void qualitymonitor::on_pushButton_ErrorSig_clicked(QString resons)
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
    item = new QTableWidgetItem(resons);
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
    ui->tableWidget->setItem(0, 2, item);

    QString beWrite;
    beWrite = QString(";%1;%2;%3").arg(QString(QDate().currentDate().toString("yyyy/MM/dd")))   \
              .arg(QString(QTime().currentTime().toString()))                                   \
              .arg(resons.append("\n"))                                                   \
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
}

void qualitymonitor::count_ISR_times()
{
    float pulselength = ui->PulseLength->text().toFloat();
    ui->label_L_speed->setText("Speed: " + QString::number(isr_count_tick *pulselength *60 /1000, 'f' , 1));
    ui->trig_count->setText(QString::number(isr_count_tick));
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
    QString PulseLength = QString::number(initParameter.PulseLength());
    QString Filter_1    = QString::number(initParameter.Filter_1());
    QString Filter_2    = QString::number(initParameter.Filter_2());
    QString BiasAdjust  = QString::number(initParameter.BiasAdjust());

    ui->PulseLength ->setText(PulseLength);
    ui->Filter_1    ->setText(Filter_1);
    ui->Filter_2    ->setText(Filter_2);
    ui->BiasAdjust  ->setText(BiasAdjust);

    //set LVDT offset
    QString outputL_offset = QString::number(initParameter.Out1_Offset());
    QString outputR_offset = QString::number(initParameter.Out2_Offset());

    ui->out1_offset->setText(outputL_offset);
    ui->out2_offset->setText(outputR_offset);

    QString password = QString::number(initParameter.Password());

    ui->lineEdit_password->setText(password);

/*
    QFile password(":/password");
    if(!password.open(QFile::ReadOnly | QFile::Text))
        qDebug() << "Cannot load password";
    QTextStream readSavedPassword(&password);
    ui->lineEdit_password->setText(readSavedPassword.readAll());
    */
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

    //cal how many data row number on last record
    QString ErrorHistory =  SetErrorTable.Read(QDir().currentPath() + "/errorhistory", 0);
    int rowNumber = ErrorHistory.count("\n");

    //creat table
    ui->tableWidget->setColumnCount(3);
    ui->tableWidget->setRowCount(ErrorHistory.count("\n"));
    //load data
    for (int i = 0; i < rowNumber; i++) {
        QTableWidgetItem *item = new QTableWidgetItem();
        item = new QTableWidgetItem(ErrorHistory.section(";", i*3+1, i*3+1));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 0, item);
        item = new QTableWidgetItem(ErrorHistory.section(";", i*3+2, i*3+2));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 1, item);
        item = new QTableWidgetItem(ErrorHistory.section(";", i*3+3, i*3+3));
        item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
        ui->tableWidget->setItem(i, 2, item);
    }
    ui->tableWidget->setAlternatingRowColors(true);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    QStringList labels;
    labels<<"Date"<<"Time"<<"Reason";
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

void qualitymonitor::toSaveDate(int indx){
    parameter writeParameter;
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
            break;
        }
        case EEParameter :{
            QString saveData;
            QString PulseLength = writeParameter.TrantoNumberType(ui->PulseLength->text());
            QString Filter_1    = writeParameter.TrantoNumberType(ui->Filter_1->text());
            QString Filter_2    = writeParameter.TrantoNumberType(ui->Filter_2->text());
            QString BiasAdjust  = writeParameter.TrantoNumberType(ui->BiasAdjust->text());
            QString outputL_offset = writeParameter.TrantoNumberType(ui->out1_offset->text());
            QString outputR_offset = writeParameter.TrantoNumberType(ui->out2_offset->text());
            QString password    = writeParameter.TrantoNumberType(ui->lineEdit_PasswordInput->text());

            saveData = (PulseLength         + "\n");
            saveData.append(Filter_1        + "\n");
            saveData.append(Filter_2        + "\n");
            saveData.append(BiasAdjust      + "\n");
            saveData.append(outputL_offset  + "\n");
            saveData.append(outputR_offset  + "\n");
            saveData.append(password              );
            writeParameter.Write(QDir().currentPath() + "/EEconfig", saveData);
        }
    }
}
void qualitymonitor::on_saveButton_clicked()
{
    toSaveDate(normalParameter);
}
void qualitymonitor::on_saveEEpraButton_clicked()
{
    toSaveDate(EEParameter);
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
}
void qualitymonitor::on_test_Button_clicked()
{
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
}
void qualitymonitor::on_pushButton_OutputCenter_clicked()
{
    ui->frameOutputCenter->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
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

    //exit(EXIT_FAILURE);
    //QApplication::closeAllWindows();
}
//set lvdt offset
void qualitymonitor::on_pushButton_out1offset_clicked()
{
    QString Out1_offset = QString::number(QString(ui->out1_pos->text()).toInt() -1000);
    ui->out1_offset->setText(Out1_offset);
    toSaveDate(EEParameter);
}
void qualitymonitor::on_pushButton_out2offset_clicked()
{
    QString Out2_offset = QString::number(QString(ui->out2_pos->text()).toInt() -1000);
    ui->out2_offset->setText(Out2_offset);
    toSaveDate(EEParameter);
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
        //ui->Chart_L->replot();
        //ui->Chart_R->replot();
        //Write_newData(Write_L);
        //Write_newData(Write_R);
        Write_newData(LeftSide_SPG);
        Write_newData(RightSide_SPG);
    }
    //over limit
    else if (event->timerId() == timeid_Alarm){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overAper_L || overAper_R)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_Alarm);
                //clear CV data array
                CV_1m.clear();

                on_pushButton_ErrorSig_clicked("over A%");

                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlag = 0;
            }
    }
    else if (event->timerId() == timeid_AlarmofCV){
        if(ui->pushbutton_QMenble->isChecked()) //QM enable
            if(overCV_per_L || overCV_per_L)
            {
                qDebug() << "STOP!";
                gpioWrite(Stop_signal, PI_HIGH);    //emit stop signal
                killTimer(this->timeid_AlarmofCV);
                //clear CV data array
                CV_1m.clear();

                on_pushButton_ErrorSig_clicked("over CV%");
                ui->label_stopFlag->setText(QString::number(1));
                on_errorfram_Button_clicked();

                AlarmFlagofCV = 0;
            }
    }

    //ISR counter
    else if(event->timerId() == timeid_TrigCount){
        count_ISR_times();
        //qDebug() << mAD->readADCvalue();
    }

    //replot SPG
    else if(event->timerId() == timeid_replotSPG)
    {
        set_SPG_Chart();
    }
    if(event->timerId() == timeid_GUI_ADC_Value)
    {
        on_Receive_ADval();
    }

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
                ui->EEprameter->raise();
                if(!ui->Dateframe->isTopLevel()){
                    ui->Dateframe->raise();
                    ui->MenuFrame->raise();
                }
            break;
            case 2:
            // raise Setting frame
                ui->SettingFrame->raise();
                if(!ui->Dateframe->isTopLevel()){
                    ui->Dateframe->raise();
                    ui->MenuFrame->raise();
                }
            break;
        }

        ui->frame_password->hide();
    }
    else
    {
        //wrong password
        QMessageBox warning;
        warning.setText("Wrong Password!");
        warning.setWindowFlags(Qt::WindowDoesNotAcceptFocus | Qt::FramelessWindowHint |     \
                               Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
        warning.setIcon(QMessageBox::Warning);
        warning.exec();
    }
    ui->lineEdit_PasswordInput->clear();
    mKeyboard->hideKeyboard();
}


void qualitymonitor::on_pushButton_SettingSave_clicked()
{
    //save setting frame data
    toSaveDate(EEParameter);
}

void qualitymonitor::on_pushButton_centerConfirm_clicked()
{
    if(!(ui->lineEdit_LeftCenter->text().isEmpty() | ui->lineEdit_RightCenter->text().isEmpty()))
    {
        float newL_center = ui->lineEdit_LeftCenter->text().toFloat();
        float newR_center = ui->lineEdit_RightCenter->text().toFloat();

        float oldL_center = ui->L_feedoutcenter->text().toFloat();
        float oldR_center = ui->R_feedoutcenter->text().toFloat();

        // leak threshold


    }
    else
    {
        QMessageBox warning;
        warning.setText("Please type in new weight");
        warning.setWindowFlags(Qt::WindowDoesNotAcceptFocus | Qt::FramelessWindowHint |
                               Qt::WindowStaysOnTopHint | Qt::Tool | Qt::X11BypassWindowManagerHint);
        warning.setIcon(QMessageBox::Warning);
        warning.exec();
    }
}

void qualitymonitor::on_pushButton_startDetect_clicked()
{

}




