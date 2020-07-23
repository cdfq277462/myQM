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

//write data index
#define Write_L     1
#define Write_R     2
#define Write_SPG   3

#define	trig_pin	27 // trigger
#define run_signal  22 // run signal
#define Stop_signal 17
#define AL_enable   18

//sample length
//unit = cm
#define SPG_SampleLength    4096

//combobox index
#define LeftSide    0
#define RightSide   1


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

    timeid_DateTime = startTimer(100);

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

//Setup_Graphics;
    Set_Graphics_L();
    Set_Graphics_R();
    Setup_HistoryChart();

//GPIO setup
    gpioInitialise();
    gpioSetMode(trig_pin, PI_INPUT);            //ISR pin
    gpioSetPullUpDown(trig_pin, PI_PUD_DOWN);
    gpioSetMode(Stop_signal, PI_OUTPUT);        //stop signal
    gpioSetMode(AL_enable, PI_OUTPUT);          //AL enable

    time_sleep(0.001);

    gpioSetISRFunc(trig_pin, FALLING_EDGE, 0, ADtrig_ISR); //ISR
    gpioSetISRFunc(run_signal, RISING_EDGE, 0, Running);

    gpioWrite(Stop_signal, PI_LOW);
    ui->label_stopFlag->setText(QString::number(0));

    //connect(&mTrigger, SIGNAL(emit_trig_sig()), this, SLOT(slot()));
    connect(this, SIGNAL(sig()), this, SLOT(slot()));

    //connect(this, SIGNAL(on_trig(int)), this, SLOT(speed_cal(int)));

/**************  ADC Read  ***************************************/
    ADread *mAD = new ADread;
    connect(mAD, SIGNAL(emit_AD_value(float)), this, SLOT(on_Receive_ADval(float)));
    connect(this, SIGNAL(emit_adc_enable()), mAD, SLOT(ADC_enable()));
    mAD->start();
    //pulselength = ui->PulseLength->text().toDouble();
/*****************************************************************/
    time_sleep(0.1);

    timeid_TrigCount = startTimer(1000);

    ui->lineEdit_password->setEchoMode(QLineEdit::Password);

}
void qualitymonitor::Running(int gpio, int level, uint32_t tick)
{

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
void qualitymonitor::on_Receive_ADval(float getAD_val)
{
    AD_value = getAD_val;
    ui->out1_pos->setText(QString::number(AD_value));
    ui->out2_pos->setText(QString::number(AD_value));

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
    //set runframe A%
    ui->label_L_A_per->setText("A%: "+QString::number(Mymathtool.A_per(ui->L_feedoutcenter->text(), AD_value), 'f', 2)+" %");
    ui->label_R_A_per->setText("A%: "+QString::number(Mymathtool.A_per(ui->R_feedoutcenter->text(), AD_value), 'f', 2)+" %");


    //change runframe A% text color
    if(qAbs(Mymathtool.A_per(ui->L_feedoutcenter->text(), AD_value)) > ui->L_limit_Aper->text().toFloat()*0.65)
    {
        if(qAbs(Mymathtool.A_per(ui->L_feedoutcenter->text(), AD_value)) > ui->L_limit_Aper->text().toFloat())
            ui->label_L_A_per->setStyleSheet("color : red ; font-size : 15px");
        else
            ui->label_L_A_per->setStyleSheet("color : yellow ; font-size : 15px");
    }
    else
        ui->label_L_A_per->setStyleSheet("color : green ; font-size : 15px");

    if(qAbs(Mymathtool.A_per(ui->R_feedoutcenter->text(), AD_value)) > ui->R_limit_Aper->text().toFloat()*0.65)
    {
        if(qAbs(Mymathtool.A_per(ui->R_feedoutcenter->text(), AD_value)) > ui->R_limit_Aper->text().toFloat())
            ui->label_R_A_per->setStyleSheet("color : red ; font-size : 15px");
        else
            ui->label_R_A_per->setStyleSheet("color : yellow ; font-size : 15px");
    }
    else
        ui->label_R_A_per->setStyleSheet("color : green ; font-size : 15px");


    //set testframe ADC value
    ui->test_inputL->setText(QString::number(AD_value* 3.111* 2/ 4096, 'f',2));
    ui->test_inputR->setText(QString::number(AD_value* 3.111* 2/ 4096, 'f',2));

    //if(AD_val > 3)
        //on_pushButton_ErrorSig_clicked();

}
void qualitymonitor::on_Receive_Trig()
{   //prepare to delete
    //Set_GraphicsView();
    //qDebug() << "on_Receive_Trig :" << thread()->currentThreadId();
}

void qualitymonitor::slot()
{
    //every trig do once
    //
    /************************************************************
     *
     * R side still NOT added
     *
     * ***********************************************************/
    static float avg_AD_value_1cm = 0;
    static float avg_AD_value_forGUI = 0;
    static float avg_AD_value_forWrite = 0;

    //SampleTimes is count trig times
    //static int SampleTimes = 0;

    static double datalenght_L = 0;
    static double datalenght_R = 0;

    static int CV1m_times = 0;
    static int CV10m_times = 0;

    static float avg_CV1m = 0;
    static float avg_CV10m = 0;
    static float avg_CV100m = 0;

    static double SampleTimes[3] = {0};
    /* *********************************
     * SampleTimes[0] = SPG's flag
     * SampleTimes[1] = GUI's flag(A% CV1m)
     * SampleTimes[2] = A% CV 100m datawrite
     * *********************************/
    int tmp = ui->PulseLength->text().toFloat();
    int OneCentimeterSampleTimes =0;
    if(tmp != 0)
        OneCentimeterSampleTimes =  10 / tmp; //unit = mm

    //GUI_SampleLength = 100cm
    //GUI A% update every 5m
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
    avg_AD_value_1cm      += AD_value / OneCentimeterSampleTimes;
    avg_AD_value_forGUI   += AD_value / GUI_SampleLength / OneCentimeterSampleTimes;
    avg_AD_value_forWrite += AD_value / Datawrite_SampleLength / OneCentimeterSampleTimes;

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
        Write_newData(Write_SPG);
    }

    if(SPG_Flag)
    {
        //every 1cm store AD_value
        SPG_Data.append(avg_AD_value_1cm);
        DataWrite_SPG.append(QString::number(avg_AD_value_1cm));
        CV_1m.append(avg_AD_value_1cm);

        /************test******************/
        ui->label_test1->setNum(avg_AD_value_1cm);
        /**********************************/

        avg_AD_value_1cm = 0;   //clear avg_AD_value_1cm
        SampleTimes[0] = 0; //reset flag

        SampleTimes[1]++;
        SampleTimes[2]++;

        bool GUI_Flag = SampleTimes[1] == GUI_SampleLength;

        if(GUI_Flag)
        {
            //every 100cm do once
            float A_per = (avg_AD_value_forGUI - ui->L_feedoutcenter->text().toFloat()) / ui->L_feedoutcenter->text().toFloat() *100;

            ui->label->setText(QString::number(A_per, 'f', 2)+" %");                //remove later; used to debug
            //qDebug() << CV_1m.length();
            float SD = 0;
            foreach(double CV_number, CV_1m)
                SD += pow(CV_number - avg_AD_value_forGUI , 2);
            /*
            for (int i = 0; i < GUI_SampleLength; i++)
                SD += pow(CV_1m.at(GUI_SampleLength - i) - avg_AD_value_forGUI, 2);
            */
            SD = sqrt(SD / (CV_1m.length() - 1)) / avg_AD_value_forGUI *100;

            ui->label_L_CV1m->setText("1mCV%: " +QString::number(SD, 'f', 2) + "%");
            avg_CV1m += SD/100;
            /************test******************/
            ui->label_test2->setNum(avg_AD_value_forGUI);
            /**********************************/

            //Over A% limit more than 3s, emit STOP sig.
            overAper_L = qAbs(A_per) >= ui->L_limit_Aper->text().toFloat();
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
            if(overCV_per_L)
                if(AlarmFlagofCV == 0)
                {
                    timeid_AlarmofCV = startTimer(5000);
                    AlarmFlagofCV = 1;
                }

            //CV1m_times++;
            CV_1m.clear();
            avg_AD_value_forGUI = 0;    //clear avg_AD_value_forGUI
            SampleTimes[1] = 0; //reset flag
            bool DataWrite_Flag =   SampleTimes[2] == Datawrite_SampleLength;


            if(DataWrite_Flag)
            {
                A_per = (avg_AD_value_forWrite - ui->L_feedoutcenter->text().toFloat()) / ui->L_feedoutcenter->text().toFloat() *100;
                //set Data to be Writed, and to be plot
                /************test******************/
                ui->label_test3->setNum(avg_AD_value_forWrite);
                /**********************************/

                //datalenght_L's unit is meter
                datalenght_L += Datawrite_SampleLength / 100;

                ui->Chart_L->graph(0)->addData(datalenght_L, A_per);
                ui->Chart_L->graph(1)->addData(datalenght_L, avg_CV1m);

                DataWrite_L.append(QString::number(avg_AD_value_forWrite));
                DataWrite_CV_L.append(QString::number(avg_CV1m));


                //display xAxis to 30000m
                if(datalenght_L >= 30000){
                    ui->Chart_L->xAxis->setRange(datalenght_L-30000, datalenght_L);
                    ui->Chart_L->xAxis2->setRange(datalenght_L-30000, datalenght_L);
                }
                else{
                    ui->Chart_L->xAxis->setRange(0, 30000);
                    ui->Chart_L->xAxis2->setRange(0, 30000);
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
                SampleTimes[2] = 0; //reset flag
            }
        }
    }

    /*
    if(GUI_Flag)
    {


        if(CV1m_times == 20){
            //CV_1m.append(avg);
            //SD = (CV_1m.at(0) + CV_1m.at(1));
            ui->label_L_CV1m->setText("1mCV%: " +QString::number(avg_CV1m, 'f', 2) + "%");
            //CV_path.append(avg_CV1m);
            avg_CV10m += avg_CV1m /10;

            overCV_per_L = avg_CV1m > ui->L_limit_CVper->text().toFloat();
            if(overCV_per_L)
                if(AlarmFlagofCV == 0)
                {
                    timeid_AlarmofCV = startTimer(5000);
                    AlarmFlagofCV = 1;
                }
            CV1m_times = 0;
            CV10m_times++;

            avg_CV1m = 0;
            CV_1m.clear();
        }

        if(CV10m_times == 10)
        {
            //CV10m
            //avg_CV100m += avg_CV10m /10;
            ui->label_R_CV1m->setNum(avg_CV10m);

            avg_CV10m = 0;
            CV10m_times = 0;
        }

        //Over A% limit more than 3s, emit STOP sig.
        overAper_L = qAbs(A_per) >= ui->L_limit_Aper->text().toFloat();
        //qDebug() << overAper;
        if(overAper_L)
            if(AlarmFlag == 0)
            {
                timeid_Alarm = startTimer(3000);
                AlarmFlag = 1;
                //qDebug() << "alarm";
            }

        SampleTimes[1] = 0; //reset flag to false
        avg_AD_value_forGUI = 0;
    }
/*
    if(DataWrite_Flag)
    {
        ui->label_R_speed->setNum(avg_AD_value_forWrite);

        //every 100m, plot once
        datalenght_L += Datawrite_SampleLength;
        float A_per = (avg_AD_value_forWrite - ui->L_feedoutcenter->text().toFloat()) / ui->L_feedoutcenter->text().toFloat() *100;

        //ui->label->setText(QString::number(A_per, 'f', 2)+" %");                //remove later

        //get AD_val
        //qDebug() << datalenght;

        ui->label_2->setNum(A_per);
        //avg_CV100m = 0;

        DataWrite_L.append(QString::number(avg_AD_value_forWrite) + "\n");
        ui->Chart_L->graph(0)->addData(datalenght_L, A_per);
        ui->Chart_L->graph(1)->addData(datalenght_L, avg_CV100m);
        avg_CV10m = 0;
        avg_CV100m = 0;

        //display xAxis to 30000m
        if(datalenght_L >= 3000000){
            ui->Chart_L->xAxis->setRange(datalenght_L-3000000, datalenght_L);
            ui->Chart_L->xAxis2->setRange(datalenght_L-3000000, datalenght_L);
        }
        else{
            ui->Chart_L->xAxis->setRange(0, 3000000);
            ui->Chart_L->xAxis2->setRange(0, 3000000);
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


        //reset sampleTimes
        //SampleTimes = 0;
        avg_AD_value_forWrite = 0;
        SampleTimes[2] = 0;
    }
*/
}

void qualitymonitor::Read_historyData(QString filename)
{

    QFile ReadHistory(filename);
    //series = new QSplineSeries();

    if(!ReadHistory.open(QFile::ReadOnly | QFile::Text))
    {
        qDebug() << "Cannot read old data!";
    }
    QTextStream Readin(&ReadHistory);
    QStringList Historylist = Readin.readAll().split("\n");
    //qDebug() << read_data;
    double i = 0;
    foreach (QString str, Historylist) {
        historyData.append(str.toDouble());
        historyData_x.append(i);
        i++;
    }
    ReadHistory.close();
}
void qualitymonitor::Write_newData(int index)
{
    /*******************************************************
     * Right side need to modify
     * ****************************************************/
    switch(index){
        case Write_L:{
            QString Filename_L = QDir().currentPath() + "/History/LeftSide/real_input_L";
            QFile real_input_L(Filename_L);

            if(!real_input_L.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream Writeout_L(&real_input_L);
            //DataWrite_L.append(QString::number( ui->out1_pos->text().toFloat()) + "\n");
            Writeout_L << DataWrite_L;
            real_input_L.flush();
            real_input_L.close();
            break;
        }
        case Write_R:{
            QString Filename_R = QDir().currentPath() + "/History/RightSide/real_input_R";
            QFile real_input_R(Filename_R);

            if(!real_input_R.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream Writeout_R(&real_input_R);
            //DataWrite_R.append(QString::number( ui->out2_pos->text().toFloat()) + "\n");
            Writeout_R << DataWrite_L;
            real_input_R.flush();
            real_input_R.close();
            break;
        }
        case Write_SPG :{
            QString Filename = QDir().currentPath() + "/SPG_data";
            QFile SPG(Filename);

            if(!SPG.open(QFile::WriteOnly | QFile::Text))
            {
                qDebug() << "Cannot write new data";
            }
            QTextStream out(&SPG);
            out << DataWrite_SPG.join("\n");
            SPG.flush();
            SPG.close();
            break;
        }
    }
}

void qualitymonitor::Set_Graphics_L()
{
    static double datalenght = 0;
    datalenght = 0;

    ui->Chart_L->addGraph();
    ui->Chart_L->addGraph(ui->Chart_L->xAxis2, ui->Chart_L->yAxis2);

    //ui->Chart_L->graph(0)->setData(axixX_L, mData_L);
    //ui->Chart_L->graph(1)->setData(axixX_L, CV_path);

    //set CV yAxis visible
    ui->Chart_L->yAxis2->setVisible(true);
    ui->Chart_L->yAxis2->setTickLabelColor(Qt::blue);

    QPen pen = ui->Chart_L->graph(0)->pen();
    pen.setWidth(5);
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
        ui->Chart_L->xAxis2->setRange(datalenght-30000, datalenght);
    }
    else{
        ui->Chart_L->xAxis->setRange(0, 30000);
        ui->Chart_L->xAxis2->setRange(0, 30000);
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
    datalenght = 0;

    ui->Chart_R->addGraph();
    ui->Chart_R->addGraph(ui->Chart_R->xAxis2, ui->Chart_R->yAxis2);

    ui->Chart_R->yAxis2->setVisible(true);
    ui->Chart_R->yAxis2->setTickLabelColor(Qt::blue);

    //ui->Chart_R->graph(0)->setData(axixX_R, mData_R);

    QPen pen = ui->Chart_R->graph(0)->pen();
    pen.setWidth(5);
    QLinearGradient linearGradient(0, 0, 0, 200);
    linearGradient.setColorAt(0,    Qt::red);
    linearGradient.setColorAt(0.12, Qt::red);
    linearGradient.setColorAt(0.20, Qt::yellow);
    linearGradient.setColorAt(0.39, Qt::yellow);
    linearGradient.setColorAt(0.40, Qt::green);
    linearGradient.setColorAt(0.74, Qt::green);
    linearGradient.setColorAt(0.75, Qt::yellow);
    linearGradient.setColorAt(0.95, Qt::yellow);
    linearGradient.setColorAt(1.0,  Qt::red);

    pen.setBrush(QBrush(linearGradient));

    ui->Chart_R->graph(0)->setPen(pen);

    // set axes ranges:
    // set axes ranges:
    if(datalenght >= 30000){
        ui->Chart_R->xAxis->setRange(datalenght-30000, datalenght);
        ui->Chart_R->xAxis2->setRange(datalenght-30000, datalenght);
    }
    else{
        ui->Chart_R->xAxis->setRange(0, 30000);
        ui->Chart_R->xAxis2->setRange(0, 30000);
    }

    float R_Aper_Limit = ui->R_limit_Aper->text().toFloat() * 1.5;
    float CV_per_Limit = ui->R_limit_CVper->text().toFloat() * 1.5;

    ui->Chart_R->yAxis->setRange(-R_Aper_Limit, R_Aper_Limit);
    ui->Chart_R->yAxis2->setRange(-0.5, CV_per_Limit);

    ui->Chart_R->replot();

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
        //Write_newData(Write_SPG);
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
    }

}


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

void qualitymonitor::Setup_HistoryChart()
{
    ui->HistoryChart->addGraph();    
    ui->HistoryChart->xAxis->setRange(0, 50000);
    ui->HistoryChart->yAxis->setRange(-0.5, 10);
    //set chart can Drag, Zoom & Selected
    ui->HistoryChart->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

    connect(ui->HistoryChart->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(fixed_yAxisRange(QCPRange)));
    connect(ui->HistoryChart->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(fixed_xAxisRange(QCPRange)));
}


void qualitymonitor::set_SPG_Chart()
{
    ui->SPG_Chart->addGraph();
    //QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    //ui->SPG_Chart->xAxis->setTicker(logTicker);

    QSharedPointer<QCPAxisTickerText> TransTologTicker(new QCPAxisTickerText);
    ui->SPG_Chart->xAxis->setTicker(TransTologTicker);
    //ui->SPG_Chart->xAxis->setScaleType(QCPAxis::stLogarithmic);

    ui->SPG_Chart->xAxis->setRange(1, 10000);
    ui->SPG_Chart->yAxis->setRange(-0.5, 10);

    QVector<double> SPG = Mymathtool.SPG(SPG_Data);
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
    //setData to Bar Chart
    //SPG_Bar->setData(interval, SPG);
    SPG_Bar->setData(x, SPG);

    SPG_Bar->setWidth(1);
    //setRange
    ui->SPG_Chart->xAxis->setRange(1, 50);
    ui->SPG_Chart->yAxis->rescale();

    ui->SPG_Chart->replot();
    //clear data
    SPG_Bar->data()->clear();
}

void qualitymonitor::fixed_xAxisRange(QCPRange boundRange)
{
    //while range over data range, fixed axis range
    if(boundRange.lower < 0)
        ui->HistoryChart->xAxis->setRange(0, boundRange.upper);
    if(boundRange.upper > historyData_x.length())
        ui->HistoryChart->xAxis->setRange(boundRange.lower, historyData_x.length());
}

void qualitymonitor::fixed_yAxisRange(QCPRange)
{
    ui->HistoryChart->yAxis->rescale();
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
    //init Data
    historyData.clear();
    historyData_x.clear();

    Read_historyData(filename);
    ui->HistoryChart->graph(0)->setData(historyData_x, historyData);
    ui->HistoryChart->xAxis->rescale();
    ui->HistoryChart->yAxis->rescale();

    ui->HistoryChart->replot();
}

void qualitymonitor::on_comboBox_SideChose_currentIndexChanged(int index)
{
    //side chose combobox
    QString HistoryRootPath = QDir().currentPath() + "/History";

    switch(index){
        case LeftSide:{
            ui->listView_historyFile->setRootIndex(History_filemodel->setRootPath(HistoryRootPath + "/LeftSide"));
            break;
        }

        case RightSide:{
            ui->listView_historyFile->setRootIndex(History_filemodel->setRootPath(HistoryRootPath + "/RightSide"));
            break;
        }
    }
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
}



void qualitymonitor::Setup_History()
{
    //set DateTime range
    ui->dateTimeEdit    ->setDateTime(QDateTime::currentDateTime().addDays(-14));
    ui->dateTimeEdit_2  ->setDateTime(QDateTime::currentDateTime());
    ui->dateTimeEdit    ->setCalendarPopup(true);
    ui->dateTimeEdit_2  ->setCalendarPopup(true);
    ui->dateTimeEdit    ->setMaximumDate(QDate::currentDate());
    ui->dateTimeEdit    ->setMinimumDate(QDate::currentDate().addDays(-14));
    ui->dateTimeEdit_2  ->setMaximumDate(QDate::currentDate());
    ui->dateTimeEdit_2  ->setMinimumDate(QDate::currentDate().addDays(-14));
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

            saveData = (PulseLength         + "\n");
            saveData.append(Filter_1        + "\n");
            saveData.append(Filter_2        + "\n");
            saveData.append(BiasAdjust      + "\n");
            saveData.append(outputL_offset  + "\n");
            saveData.append(outputR_offset        );
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
    ui->EEprameter->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }

}
void qualitymonitor::on_pushButton_Settiing_clicked()
{
    ui->SettingFrame->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
}

void qualitymonitor::on_pushButton_3_clicked()
{
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

void qualitymonitor::on_pushButton_SPG_clicked()
{
    //cal & set SPG
    set_SPG_Chart();

    ui->frame_SPG->raise();
    if(!ui->Dateframe->isTopLevel()){
        ui->Dateframe->raise();
        ui->MenuFrame->raise();
    }
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

}



void qualitymonitor::on_pushButton_replot_clicked()
{
    //cal & set SPG
    set_SPG_Chart();
}


void qualitymonitor::on_pushbutton_QMenble_clicked()
{
    //change QM enable button color
    if(ui->pushbutton_QMenble->isChecked())
        ui->pushbutton_QMenble->setStyleSheet("background : green ; color : white");
    else
        ui->pushbutton_QMenble->setStyleSheet("background : red ; color : white");

}


