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

#define  normalParameter    1
#define  EEParameter        2


#define LED         17  //test sig
#define	trig_pin	27 // trigger

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

//set timer
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(DateTimeSlot()));
    timer->start(1000);

//setup parameter
    setupParameter();
    SetErrorTable();
//set runframe on toplevel
    ui->runframe->raise();
    ui->Dateframe->raise();
    //qDebug() << "main: " << QThread::currentThread();
    Setup_History();

    on_pushButton_Search_clicked();

//ADC interrupt
    gpioInitialise();
    gpioSetMode(LED, PI_OUTPUT);
    gpioSetMode(trig_pin, PI_INPUT);
    gpioSetPullUpDown(trig_pin, PI_PUD_DOWN); //set trig_pin to edge trig
    time_sleep(0.001);
    gpioSetISRFunc(trig_pin, FALLING_EDGE, 0, ADtrig_ISR); //ISR

    connect(&mTrigger, SIGNAL(emit_trig_sig()), this, SLOT(slot()));
    connect(this, SIGNAL(sig()), this, SLOT(slot()));

    //connect(this, SIGNAL(on_trig(int)), this, SLOT(speed_cal(int)));

/**************  ADC Read  ***************************************/
    ADread *mAD = new ADread;
    connect(mAD, SIGNAL(emit_AD_value(float)), this, SLOT(on_Receive_ADval(float)));
    connect(this, SIGNAL(emit_adc_enable()), mAD, SLOT(ADC_enable()));
    mAD->start();
/*****************************************************************/
    time_sleep(0.1);

    //Setup_GraphicsView();
    Read_oldData();

//ADC interrupt

    /*
    watcher.addPath ("/sys/class/gpio/gpio27/value");
    QObject::connect(&watcher, SIGNAL(fileChanged(QString)), this, SLOT(ADC_ISR(QString)));
    */
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(count_ISR_times()));
    timer->start(1000);


}
void qualitymonitor::count_ISR_times()
{
    ui->label_L_speed->setText("Speed: " + QString::number(isr_count_tick *2.221554 *60 /1000));
    isr_count_tick = 0; //

}
void qualitymonitor::ADtrig_ISR(int gpio, int level, uint32_t tick)
{//7983 Hz
    flag ++;
    //printf("%u\n", flag);
    //MyTrigger mTrigger;    
    isr_count_tick++;
    if(flag == 5){
        end_t = clock();
        qDebug() << "AD read" << difftime(end_t, start_t) ;
        //ISR_excute_ptr->on_Receive_Trig();
        emit ISR_excute_ptr->sig();
        //AD start to read
        start_t = clock();
        flag = 0;
    }
}

qualitymonitor::~qualitymonitor()
{
    gpioTerminate();
    delete ui;
}
void qualitymonitor::on_Receive_ADval(float AD_val)
{
    ui->out1_pos->setText(QString::number(AD_val));
    ui->out2_pos->setText(QString::number(AD_val));

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
    ui->label_L_A_per->setText("A%: "+QString::number(Mymathtool.A_per(ui->L_feedoutcenter->text(), ui->out1_pos->text()), 'f', 2)+" %");
    ui->label_R_A_per->setText("A%: "+QString::number(Mymathtool.A_per(ui->R_feedoutcenter->text(), ui->out2_pos->text()), 'f', 2)+" %");

    //set testframe ADC value
    ui->test_inputL->setText(QString::number(AD_val* 3.111* 2/ 4096));
    ui->test_inputR->setText(QString::number(AD_val* 3.111* 2/ 4096));

    //if(AD_val > 3)
        //on_pushButton_ErrorSig_clicked();

}


void qualitymonitor::on_Receive_Trig()
{   //AD trig
    //qDebug() << "TRIG!";
    //Set_GraphicsView();
    //mTrigger.start();
    //mTrigger.wait();
    qDebug() << "on_Receive_Trig :" << thread()->currentThreadId();
}
void qualitymonitor::slot()
{
    //mTrigger.start();
    //mTrigger.wait();
    //float A_per = Mymathtool.A_per(ui->L_feedoutcenter->text(), ui->out1_pos->text());
    ui->label->setText(QString::number(Mymathtool.A_per(ui->L_feedoutcenter->text(), ui->out1_pos->text()), 'f', 2)+" %");
    qDebug() << "on_slot :" << thread()->currentThreadId();
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
    //Setup_GraphicsView();
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

    QString CR_diameter = QString::number(initParameter.CR_diameter());
    QString DetectGear  = QString::number(initParameter.DetectGear());
    QString Filter_1    = QString::number(initParameter.Filter_1());
    QString Filter_2    = QString::number(initParameter.Filter_2());
    QString BiasAdjust  = QString::number(initParameter.BiasAdjust());

    ui->CR_diameter ->setText(CR_diameter);
    ui->DetectGear  ->setText(DetectGear);
    ui->Filter_1    ->setText(Filter_1);
    ui->Filter_2    ->setText(Filter_2);
    ui->BiasAdjust  ->setText(BiasAdjust);

    //set LVDT offset
    QString outputL_offset = QString::number(initParameter.Out1_Offset());
    QString outputR_offset = QString::number(initParameter.Out2_Offset());

    ui->out1_offset->setText(outputL_offset);
    ui->out2_offset->setText(outputR_offset);
}

void qualitymonitor::Read_oldData()
{
    QString Filename_L = QDir().currentPath() + "/real_input_L";
    QString Filename_R = QDir().currentPath() + "/real_input_R";

    series_L = new QSplineSeries();
    series_R = new QSplineSeries();
    m_serieslist.append(series_L);
    m_serieslist.append(series_R);

    QList<QPointF> mData_L, mData_R;
    QFile real_input_L(Filename_L);
    QFile real_input_R(Filename_R);

    //series = new QSplineSeries();

    if(!real_input_L.open(QFile::ReadOnly | QFile::Text) \
      |!real_input_R.open(QFile::ReadOnly | QFile::Text))
    {
        qDebug() << "Cannot initialize parameter!";
    }
    QTextStream Readin_L(&real_input_L);

    QString read_data_L = Readin_L.readAll();
    //qDebug() << read_data;

    for(int i =0; i < read_data_L.count("\n"); i++){
        int read_data_tras = read_data_L.section("\n",i,i).toInt();
        //qDebug() << read_data_tras;
        mData_L.append(QPointF(i, read_data_tras));
    }
    series_L->append(mData_L);
    //series->append(in.readAll());
    real_input_L.close();

    QTextStream Readin_R(&real_input_R);
    QString read_data_R = Readin_R.readAll();

    for(int i =0; i < read_data_R.count("\n"); i++){
        int read_data_tras = read_data_R.section("\n",i,i).toInt();
        //qDebug() << read_data_tras;
        mData_R.append(QPointF(i, read_data_tras));
    }
    series_R->append(mData_R);

    real_input_R.close();

}


void qualitymonitor::Set_GraphicsView()
{
    //cost a lot of time, need to modify
    clock_t start, end;
    start = clock();
    int dispalyrange = 500;
    //parameter real_input;
    //srand(time(NULL));    

/*
    QString real_data_read = real_input.Read(QDir().currentPath() + "/real_input", 0);
    QString real_data;
    //real_data.append(real_data_read).append(QString::number((rand()%30) + 1600) + "\n");
    real_data.append(real_data_read).append(ui->out1_pos->text() + "\n");
    int datalenght = real_data.count("\n");
    real_input.Write(QDir().currentPath() + "/real_input", real_data);


    if(datalenght < dispalyrange)
        for(int i = 0; i < datalenght - 1; i++){
            series_L->append(i, real_data.section("\n",i ,i ).toFloat());
            series_R->append(i, real_data.section("\n",i ,i ).toFloat()+100);

        }
    //qDebug()<< real_input << datalenght;

    else
        for(int i = datalenght - dispalyrange; i < datalenght - 1; i++){
            series_L->append(i, real_data.section("\n",i ,i ).toFloat());
            series_R->append(i, real_data.section("\n",i ,i ).toFloat()+100);
        }
    //series->append(i,1);
    *series = DataInput();
*/
    static int i = 0;

    float A_per = Mymathtool.A_per(ui->L_feedoutcenter->text(), ui->out1_pos->text());
    //series_L->append(i++, A_per);
    //qDebug() << A_per;


    QChart *chart_L = new QChart();
    QChart *chart_R = new QChart();
    chart_L->legend()->hide();
    chart_R->legend()->hide();

    QPen pen = series_L->pen();
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

    pen.setBrush(QBrush(linearGradient)); // or just pen.setColor("red");
    pen.setStyle(Qt::DotLine);

    series_L->setPen(pen);

    chart_L->addSeries(series_L);


    //chart_L->setBackgroundBrush(QBrush("gray"));


    //series_R << QPointF(1, 0.5) << QPointF(1.5, 4.5) << QPointF(2.4, 2.5) << QPointF(4.3, 12.5) \
            //<< QPointF(5.2, 3.5) << QPointF(7.4, 16.5) << QPointF(8.3, 7.5) << QPointF(10, 17);
    series_R->setPen(pen);
    chart_R->addSeries(series_R);


    chart_L->createDefaultAxes();
    chart_R->createDefaultAxes();

    //set display range
    chart_L->axisY()->setRange(-10, 10);
    chart_R->axisY()->setRange(0, 1000);
    chart_L->axisX()->setRange(series_L->count() - dispalyrange, series_L->count());
    //chart->setTitle("Simple line chart example");

    chart_L->setGeometry(0, 10, 380, 330);
    chart_R->setGeometry(0, 10, 380, 330);



    QChartView *chartView_L = new QChartView(chart_L);
    QChartView *chartView_R = new QChartView(chart_R);
    //chartView_L->resize(ui->graphicsView_L->size());
    chartView_L->setRenderHint(QPainter::Antialiasing);
    chartView_R->setRenderHint(QPainter::Antialiasing);


    scene_L = new QGraphicsScene(this);
    scene_R = new QGraphicsScene(this);

    //*scene.addItem(chart);
    //QGraphicsView view(&scene);
    scene_L->addItem(chart_L);
    ui->graphicsView_L->setScene(scene_L);
    scene_R->addItem(chart_R);
    ui->graphicsView_R->setScene(scene_R);
    //ui->graphicsView_L->repaint();

    end = clock();
    //qDebug() << difftime(end, start);
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
void qualitymonitor::on_pushButton_ErrorSig_clicked()
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
    item = new QTableWidgetItem(QString("Hello World!").append(""));
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);                     //set item cannot edit
    ui->tableWidget->setItem(0, 2, item);

    QString beWrite;
    beWrite = QString(";%1;%2;%3").arg(QString(QDate().currentDate().toString("yyyy/MM/dd")))   \
              .arg(QString(QTime().currentTime().toString()))                                   \
              .arg(QString("Hello World!\n"))                                                   \
              .append(ErrorRecord.Read(QDir().currentPath() + "/errorhistory", 0));

    qDebug() << beWrite;

    /*
    QString result;
    foreach(const QString &str, beWrite){
        result.append(str);
    }/
    qDebug() << result;*/
    ErrorRecord.Write(QDir().currentPath() + "/errorhistory" , beWrite);

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
            QString CR_diameter = writeParameter.TrantoNumberType(ui->CR_diameter->text());
            QString DetectGear  = writeParameter.TrantoNumberType(ui->DetectGear->text());
            QString Filter_1    = writeParameter.TrantoNumberType(ui->Filter_1->text());
            QString Filter_2    = writeParameter.TrantoNumberType(ui->Filter_2->text());
            QString BiasAdjust  = writeParameter.TrantoNumberType(ui->BiasAdjust->text());
            QString outputL_offset = writeParameter.TrantoNumberType(ui->out1_offset->text());
            QString outputR_offset = writeParameter.TrantoNumberType(ui->out2_offset->text());

            saveData = (CR_diameter         + "\n");
            saveData.append(DetectGear      + "\n");
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
