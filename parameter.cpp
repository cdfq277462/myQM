#include "parameter.h"
#include "qualitymonitor.h"
#include "ui_qualitymonitor.h"

#include <QtCore>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDir>

#define  feedoutcenter  1
#define  outputoffset   2
#define  adjustrate     3
#define  outputweight   4
#define  limit_Aper     5
#define  limit_CVper    6

#define  R_side         6


#define  MachineType_EE     1
#define  Filter_1_EE        2
#define  Filter_2_EE        3
#define  OffsetAdjust_EE      4
#define  setPassword        5



QString mfilename  = QDir().currentPath() + "/config";
QString EEfilename = QDir().currentPath() + "/EEconfig";
QString ShiftSchedule = QDir().currentPath() + "/ShiftSchedule";

parameter::parameter()
{
    //qDebug() << "hello";
/*
    QFile config(mfilename);
    Read(mfilename, 0);
    Read(EEfilename, 0);
*/
}
QString parameter::Read(QString Filename, int index){

    QFile config(Filename);

    if(!config.open(QFile::ReadOnly | QFile::Text)){
        qDebug() << "Cannot initialize parameter!";
    }
    QTextStream in(&config);
    QString para;

    //index == 0 : read All data
    //index == 1 : read in line para
    if(index == 0){
            para = in.readAll();
            //qDebug() << para;
            return  para;
        }
    else{
            for(int i = 1; i <= index; i++)
                para = in.readLine(0);
            return  para;
    }
    config.close();
}


void parameter::Write(QString Filename, QString Datain){
    QFile config(Filename);

    if(!config.open(QFile::WriteOnly | QFile::Text)){
        qDebug() << "Cannot save parameter!";
        return;
    }
    QTextStream out(&config);
    out << Datain;
    config.flush();
    config.close();
}

QString parameter::TrantoNumberType(QString xin){
    float tmp = xin.toFloat();
    xin = QString::number(tmp);
    return xin;
}

//Normal Parameter
int  parameter::L_feedoutcenter(){
    int getdata = Read(mfilename, feedoutcenter).toInt();

    return getdata;
}
int  parameter::L_outputoffset(){
    // LVDT
    int getdata = Read(mfilename, outputoffset).toInt();
    return getdata;
}


int   parameter::L_adjustrate(){
    int getdata = Read(mfilename, adjustrate).toInt();
    return getdata;
}

float parameter::L_outputweight(){
    float getdata = Read(mfilename, outputweight).toFloat();
    return getdata;
}

float  parameter::L_limit_Aper(){
    float getdata = Read(mfilename, limit_Aper).toFloat();
    return getdata;
}

float  parameter::L_limit_CVper(){
    float getdata = Read(mfilename, limit_CVper).toFloat();
    return getdata;
}

int parameter::R_feedoutcenter(){
    int getdata = Read(mfilename, R_side + feedoutcenter).toInt();
    return getdata;
}
int parameter::R_outputoffset(){
    // LVDT
    int getdata = Read(mfilename, R_side + outputoffset).toInt();
    return getdata;
}
int parameter::R_adjustrate(){
    int getdata = Read(mfilename, R_side + adjustrate).toInt();
    return getdata;
}
float parameter::R_outputweight(){
    float getdata = Read(mfilename, R_side + outputweight).toFloat();
    return getdata;
}
float parameter::R_limit_Aper(){
    float getdata = Read(mfilename, R_side + limit_Aper).toFloat();
    return getdata;
}
float parameter::R_limit_CVper(){
    float getdata = Read(mfilename, R_side + limit_CVper).toFloat();
    return getdata;
}
//end Normal Parameter

//EE Parameter
int parameter::MachineType(){
    // return MachineType combobox index
    int index = Read(EEfilename, MachineType_EE).toInt();
    return index;
}
int parameter::Filter_1(){
    int getdata = Read(EEfilename, Filter_1_EE).toInt();
    return getdata;
}
float parameter::Filter_2(){
    float getdata = Read(EEfilename, Filter_2_EE).toFloat();
    return getdata;
}
int parameter::OffsetAdjust(){
    int getdata = Read(EEfilename, OffsetAdjust_EE).toInt();
    return getdata;
}
//end EE Parameter




//Setting

QString parameter::Password()
{
    QString getdata = Read(EEfilename, setPassword);
    return getdata;
}

QString parameter::shiftschedule()
{
    QFile config(ShiftSchedule);

    if(!config.open(QFile::ReadOnly | QFile::Text)){
        qDebug() << "Cannot initialize parameter!";
    }
    QTextStream in(&config);
    QString para;
    para = in.readAll();
    config.close();
    //qDebug() << para;
    return  para;

}


