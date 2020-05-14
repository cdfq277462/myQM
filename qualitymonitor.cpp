#include "qualitymonitor.h"
#include "ui_qualitymonitor.h"

#include <QtCore>
#include <QtGui>
#include <QtDebug>
#include <QString>
#include <QFile>
#include <QMessageBox>
#include <QDateTime>

#include "mythread.h"
#include "parameter.h"


qualitymonitor::qualitymonitor(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::qualitymonitor)
{

    ui->setupUi(this);
    setupParameter();
    ui->runframe->raise();
    mThread = new MyThread(this);
    connect(mThread, SIGNAL(sig(int)), this, SLOT(onDateSlot(int)));
    mThread->start();

}

qualitymonitor::~qualitymonitor()
{
    delete ui;
}

void qualitymonitor::onDateSlot(int a)
{
    QDateTime DateTime = QDateTime::currentDateTime();
    //qDebug() << "DateTime";
    QString Date = DateTime.toString("yyyy/MM/dd");
    QString Time = DateTime.toString("hh:mm:ss");

    ui->Date -> setText(Date);
    ui->Time -> setText(Time);
    //ui->label_14    ->setText(Time);

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
    QString DetectGear = QString::number(initParameter.DetectGear());
    QString Filter_1 = QString::number(initParameter.Filter_1());
    QString Filter_2 = QString::number(initParameter.Filter_2());
    QString BiasAdjust = QString::number(initParameter.BiasAdjust());

    ui->CR_diameter->setText(CR_diameter);
    ui->DetectGear->setText(DetectGear);
    ui->Filter_1->setText(Filter_1);
    ui->Filter_2->setText(Filter_2);
    ui->BiasAdjust->setText(BiasAdjust);


}
void qualitymonitor::toSaveDate(int indx){
    parameter writeParameter;
    switch (indx) {
        case 1 :
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
            writeParameter.Write("D:/pyqttest/myQM/myQM/config", saveData);
            break;
        }
    case 2 :{
        QString saveData;
        QString CR_diameter = writeParameter.TrantoNumberType(ui->CR_diameter->text());
        QString DetectGear  = writeParameter.TrantoNumberType(ui->DetectGear->text());
        QString Filter_1    = writeParameter.TrantoNumberType(ui->Filter_1->text());
        QString Filter_2    = writeParameter.TrantoNumberType(ui->Filter_2->text());
        QString BiasAdjust  = writeParameter.TrantoNumberType(ui->BiasAdjust->text());
        saveData = (CR_diameter     + "\n");
        saveData.append(DetectGear  + "\n");
        saveData.append(Filter_1    + "\n");
        saveData.append(Filter_2    + "\n");
        saveData.append(BiasAdjust        );
        writeParameter.Write("D:/pyqttest/myQM/myQM/EEconfig", saveData);
    }

    }


}
void qualitymonitor::on_saveButton_clicked()
{
    toSaveDate(1);
}
void qualitymonitor::on_saveEEpraButton_clicked()
{
    toSaveDate(2);
}

void qualitymonitor::on_Index_clicked()
{
    if(!ui->Dateframe->isTopLevel())
        ui->Dateframe->raise();
}

void qualitymonitor::on_LVDT_Button_clicked()
{
    if(!ui->Dateframe->isTopLevel())
        ui->Dateframe->raise();
}

void qualitymonitor::on_parameter_Button_clicked()
{
    if(!ui->Dateframe->isTopLevel())
        ui->Dateframe->raise();
}

void qualitymonitor::on_test_Button_clicked()
{
    if(!ui->Dateframe->isTopLevel())
        ui->Dateframe->raise();
}

void qualitymonitor::on_errorfram_Button_clicked()
{
    if(!ui->Dateframe->isTopLevel())
        ui->Dateframe->raise();
}

void qualitymonitor::on_EEtestbutt_clicked()
{  //add type password
    ui->EEprameter->raise();
    if(!ui->Dateframe->isTopLevel())
        ui->Dateframe->raise();
}



void qualitymonitor::on_pushButton_2_clicked()
{
    mThread->Stop = true;
}
