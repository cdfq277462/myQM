#include "adread.h"

#define CE0     8
#define CE1     7
#define MISO    9
#define MOSI    10
#define SCLK    11

ADread::ADread()
{

}

void ADread::run()
{
    int i, count, set_val, read_val;
    unsigned char inBuf[3];
    char cmd1[] = {0, 0};
    char cmd2[] = {12, 0};
    char cmd3[] = {1, 128, 0};

    if (gpioInitialise() < 0)
    {
        fprintf(stderr, "pigpio initialisation failed.\n");
        //return 1;
    }

    bbSPIOpen(CE0, MISO, MOSI, SCLK, 200000, 3); // MCP3008 ADC

    for (i=0; i<256; i++)
    {
        cmd1[1] = i;

        count = bbSPIXfer(CE0, cmd1, (char *)inBuf, 2); // > DAC

     if (count == 2)
     {
        count = bbSPIXfer(CE0, cmd2, (char *)inBuf, 2); // < DAC

        if (count == 2)
        {
           set_val = inBuf[1];

           count = bbSPIXfer(CE0, cmd3, (char *)inBuf, 3); // < ADC

           if (count == 3)
           {
              read_val = ((inBuf[1]&3)<<8) | inBuf[2];
              emit emit_AD_value(read_val);
              printf("%d %d\n", i, read_val);
           }
        }
     }
    }

    bbSPIClose(CE0);
    bbSPIClose(CE1);

    gpioTerminate();

    //return 0;
}

