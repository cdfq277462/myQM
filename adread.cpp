#include "adread.h"
#include "pigpio.h"

//SPI config
#define CE0     8
#define CE1     7
#define MISO    9
#define MOSI    10
#define SCLK    11
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


ADread::ADread()
{
    gpioInitialise();
}

void ADread::run()
{
    /************************i2c**********************************/
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
        int adcData = ((i2cReadWordData(sigHandle, 0x00) & 0xff00) >> 8) \
                               | ((i2cReadWordData(sigHandle, 0x00) & 0x000f)  << 8);
            //in address 0x50 result reg 0x00
            //on recevie SDA
            //
            // D7 D6 D5 D4 D3 D2 D1 D0  | D15 D14 D13 D12 D11 D10  D9 D8
            // _  _  _  _  _  _  _   _  |  _  _   _   _   _   _   _   _
            //
            //
        //printf("ADC: %d\n", adcData);
        end_t = clock();
        //qDebug() << difftime(end_t, start_t);
        emit emit_AD_value(adcData);
    }
    /************************i2c**********************************/


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
