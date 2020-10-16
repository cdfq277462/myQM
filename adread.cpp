#include "adread.h"
#include "pigpio.h"
#include <QtCore>

//SPI config
#define CE0     8
#define CE1     7
#define MISO    9
#define MOSI    10
#define SCLK    11
#define Busy    6
#define CVAB    12
#define RST     5

bool enable = true;

//i2c config
#define i2cBus      1
#define i2cAddres   0x50
#define i2cFlag     0

#define REG_ADDR_RESULT         0x00
#define REG_ADDR_ALERT          0x01
#define REG_ADDR_CONFIG         0x02
#define REG_ADDR_LIMITL         0x03
#define REG_ADDR_LIMITH         0x04
#define REG_ADDR_HYST           0x05
#define REG_ADDR_CONVL          0x06
#define REG_ADDR_CONVH          0x07
int ADflag = 0;

ADread::ADread()
{
    gpioInitialise();
    AD7606_IOSet();
    AD7606_RST();
    ADC_value = 0;
    ADC_value_R = 0;
}
void ADread::BUSY_ISR(int gpio, int level, uint32_t tick)
{
    ADflag = 1;
    qDebug() <<"ISR" << tick << ADflag;
}

void ADread::run()
{
    /************************i2c**********************************/
    /*
    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed.\n");
        //return 1;
    }

    int sigHandle = i2cOpen(i2cBus, i2cAddres, i2cFlag);
    i2cWriteByte(REG_ADDR_CONFIG, 0x20);
    time_t start_t, end_t;
    while(enable)
    {
        start_t = clock();
        int ADC_value = ((i2cReadWordData(sigHandle, 0x00) & 0xff00) >> 8) \
                               | ((i2cReadWordData(sigHandle, 0x00) & 0x000f)  << 8);
            //in address 0x50 result reg 0x00
            //on recevie SDA
            //
            // D7 D6 D5 D4 D3 D2 D1 D0  | D15 D14 D13 D12 D11 D10  D9 D8
            // _  _  _  _  _  _  _   _  |  _  _   _   _   _   _   _   _
            //
            //
        //printf("ADC: %d\n", ADC_value);
        end_t = clock();
        //qDebug() << difftime(end_t, start_t);
        emit emit_AD_value(ADC_value);
    }
    */
    /************************i2c**********************************/

    /************************AD7606 SPI***************************/
/*
    int count;
    int spiSigHandle;
    char rxBuf[64];

    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed.\n");
        //return;
    }    
    spiSigHandle = spiOpen(0, 10000000, 2);
    gpioHardwarePWM(CVAB, 100000, 950000);

    while(enable)
    {
        //qDebug() << gpioRead(Busy);
        //qDebug() << "ADflag : " <<ADflag;
        if(ADflag == 1)
        while(gpioRead(Busy) < 1){
            count = spiRead(spiSigHandle, (char *)rxBuf, 2);
            //qDebug() << "count : " << count;
            if(count == 2){
                ADC_value = ((rxBuf[0] & 0x7f) << 8) | rxBuf[1];
                qDebug() << ADC_value;
                //AD7606_startConv();

                ADflag = 0;
                time_sleep(0.002);
                //emit emit_AD_value(ADC_value);
                while(gpioRead(Busy) == 0);
            }
        }
    }
    spiClose(spiSigHandle);
*/
    /*************************************************************/

    int count;
    int spiSigHandle;
    char rxBuf[64];

    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed.\n");
        //return;
    }
    spiSigHandle = spiOpen(0, 10000000, 2);
    //gpioHardwarePWM(CVAB, 100000, 950000);

    while(enable)
    {
        AD7606_startConv();
        gpioDelay(10);   //while BUSY falling

        count = spiRead(spiSigHandle, (char *)rxBuf, 4);
        //qDebug() << "count : " << count;
        if(count == 4){
            //ADC_value[0] = ADC_value[1];
            //ADC_value_R[0] = ADC_value_R[1];

            ADC_value = ((rxBuf[0] & 0x7f) << 8) | rxBuf[1];
            ADC_value_R = ((rxBuf[2] & 0x7f) << 8) | rxBuf[3];
            //qDebug() << real_val;
            //AD7606_startConv();
            //emit emit_AD_value(real_val);
            gpioDelay(5);
            //time_sleep(0.001);
            //while(gpioRead(Busy) == 0);
        }
    }
    spiClose(spiSigHandle);

    /************************SPI**********************************
    //qDebug() << thread()->currentThreadId();

    int count, read_val;
    unsigned char inBuf[3];
    //char cmd1[] = {0, 0};
    //char cmd2[] = {12, 0};
    char cmd3[] = {1, 128, 0};

    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed.\n");
        //return 1;
    }

    bbSPIOpen(CE0, MISO, MOSI, SCLK, 20000, 3); // MCP3008 ADC

    //for (i=0; i<25600; i++)

    while(enable)
    {
        count = bbSPIXfer(CE0, cmd3, (char *)inBuf, 3); // < ADC

        if (count == 3)
        {
          read_val = ((inBuf[1]&3)<<8) | inBuf[2];

          //printf("%d\n", read_val);
          emit emit_AD_value(read_val);
        }
    }

    bbSPIClose(CE0);
    bbSPIClose(CE1);
    *******************************************************************/

    gpioTerminate();

    //return 0;

}

void ADread::ADC_enable()
{
    enable = false;
}

void ADread::AD7606_IOSet()
{
    gpioSetMode(Busy, PI_INPUT);
    gpioSetPullUpDown(Busy, PI_PUD_DOWN);
    gpioSetMode(CVAB, PI_OUTPUT);
    gpioSetMode(RST, PI_OUTPUT);

    gpioWrite(RST, PI_LOW);
    gpioWrite(CVAB, PI_HIGH);

    //gpioSetISRFunc(Busy, FALLING_EDGE, 0, BUSY_ISR);

    //gpioHardwarePWM(CVAB, 200000, 993750);
}

void ADread::AD7606_RST()
{
    gpioWrite(RST, PI_LOW);
    gpioWrite(RST, PI_HIGH);
    gpioWrite(RST, PI_LOW);
}

void ADread::AD7606_startConv()
{
    gpioWrite(CVAB, PI_LOW);
    gpioDelay(1);
    gpioWrite(CVAB, PI_HIGH);
}

uint ADread::readADCvalue()
{
    // combine L & R value


    return (ADC_value << 16) | ADC_value_R;
}
