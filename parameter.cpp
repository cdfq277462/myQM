#include "parameter.h"
#include "qualitymonitor.h"
#include "ui_qualitymonitor.h"

#include <QtCore>
#include <QDebug>
#include <QString>
#include <QFile>
#include <QTextStream>

#define  feedoutcenter  1
#define  outputoffset   2
#define  adjustrate     3
#define  outputweight   4
#define  limit_Aper     5
#define  limit_CVper    6

#define  R_side         6

#define  CR_diameter_EE     1
#define  DetectGear_EE      2
#define  Filter_1_EE        3
#define  Filter_2_EE        4
#define  BiasAdjust_EE      5


QString mfilename  = "D:/pyqttest/myQM/myQM/config";
QString EEfilename = "D:/pyqttest/myQM/myQM/EEconfig";
parameter::parameter()
{
    //qDebug() << "hello";

    QFile config(mfilename);
    Read(mfilename, 0);
    Read(EEfilename, 0);
}
float parameter::Read(QString Filename, int index){

    QFile config(Filename);

    if(!config.open(QFile::ReadOnly | QFile::Text)){
        qDebug() << "Cannot initialize parameter!";
        return -1;
    }

    QTextStream in(&config);
    QString para;
    //if index == 0, read all data
    if(index == 0){
        para = in.readAll();
        //qDebug() << para;
        float paraout = para.toFloat();
        return  paraout;
    }
    else{
        for(int i = 1; i <= index; i++)
            para = in.readLine(0);
        float paraout = para.toFloat();

        return  paraout;
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
    int getdata = Read(mfilename, feedoutcenter);
    return getdata;
}
int  parameter::L_outputoffset(){
    int getdata = Read(mfilename, outputoffset);
    return getdata;
}


int   parameter::L_adjustrate(){
    int getdata = Read(mfilename, adjustrate);
    return getdata;
}

float parameter::L_outputweight(){
    float getdata = Read(mfilename, outputweight);
    return getdata;
}

float  parameter::L_limit_Aper(){
    float getdata = Read(mfilename, limit_Aper);
    return getdata;
}

float  parameter::L_limit_CVper(){
    float getdata = Read(mfilename, limit_CVper);
    return getdata;
}

int parameter::R_feedoutcenter(){
    int getdata = Read(mfilename, R_side + feedoutcenter);
    return getdata;
}
int parameter::R_outputoffset(){
    int getdata = Read(mfilename, R_side + outputoffset);
    return getdata;
}
int parameter::R_adjustrate(){
    int getdata = Read(mfilename, R_side + adjustrate);
    return getdata;
}
float parameter::R_outputweight(){
    float getdata = Read(mfilename, R_side + outputweight);
    return getdata;
}
float parameter::R_limit_Aper(){
    float getdata = Read(mfilename, R_side + limit_Aper);
    return getdata;
}
float parameter::R_limit_CVper(){
    float getdata = Read(mfilename, R_side + limit_CVper);
    return getdata;
}
//end Normal Parameter

//EE Parameter
int parameter::CR_diameter(){
    int getdata = Read(EEfilename, CR_diameter_EE);
    return getdata;
}
int parameter::DetectGear(){
    int getdata = Read(EEfilename, DetectGear_EE);
    return getdata;
}
int parameter::Filter_1(){
    int getdata = Read(EEfilename, Filter_1_EE);
    return getdata;
}
int parameter::Filter_2(){
    int getdata = Read(EEfilename, Filter_2_EE);
    return getdata;
}
int parameter::BiasAdjust(){
    int getdata = Read(EEfilename, BiasAdjust_EE);
    return getdata;
}
//end EE Parameter
