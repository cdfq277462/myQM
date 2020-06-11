
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "fft.h"

#ifndef FFT_N
#define FFT_N   4096
#endif // FFT_N

#ifndef PI
#define PI      3.1415926535897932384626433832795028841971
#endif // PI

//#define FILE_NAME "SPG_test.txt"
struct compx {float real,imag;};                                        //定義一個復數結構
struct compx s[FFT_N];
//int datain[FFT_N] = {0};

float x_ticks[FFT_N / 2] = {0};


struct compx EE(struct compx a,struct compx b)
{
    struct compx c;

    c.real=a.real*b.real-a.imag*b.imag;
    c.imag=a.real*b.imag+a.imag*b.real;

    return(c);
}

void FFT(struct compx *xin)
{
    int f , m, nv2, nm1, i, k, l, j = 0;
    struct compx u,w,t;

    nv2 = FFT_N / 2;                   //變址運算，即把自然順序變成倒位序，采用雷德算法
    nm1 = FFT_N - 1;
    for(i = 0; i < nm1; i++)
    {
        if(i < j)                      //如果i<j,即進行變址
        {
            t = xin[j];
            xin[j] = xin[i];
            xin[i] = t;
        }
        k = nv2;                       //求j的下一個倒位序
        while( k <= j)                 //如果k<=j,表示j的最高位為1
        {
            j = j - k;                 //把最高位變成0
            k = k / 2;                 //k/2，比較次高位，依次類推，逐個比較，直到某個位為0
        }
        j = j + k;                     //把0改為1
    }
    {
        int le , lei, ip;                            //FFT運算核，使用蝶形運算完成FFT運算

        f = FFT_N;
        for(l = 1; (f=f/2)!=1; l++)                  //計算l的值，即計算蝶形級數
            ;
        for( m = 1; m <= l; m++)                         // 控制蝶形結級數
        {                                        //m表示第m級蝶形，l為蝶形級總數l=log（2）N
            le = 2 << (m - 1);                            //le蝶形結距離，即第m級蝶形的蝶形結相距le點
            lei = le / 2;                               //同一蝶形結中參加運算的兩點的距離
            u.real = 1.0;                             //u為蝶形結運算系數，初始值為1
            u.imag = 0.0;
            w.real = cos(PI / lei);                     //w為系數商，即當前系數與前一個系數的商
            w.imag = -sin(PI / lei);
            for(j = 0;j <= lei - 1; j++)                   //控制計算不同種蝶形結，即計算系數不同的蝶形結
            {
                for(i = j; i <= FFT_N - 1; i = i + le)            //控制同一蝶形結運算，即計算系數相同蝶形結
                {
                    ip = i + lei;                           //i，ip分別表示參加蝶形運算的兩個節點
                    t = EE(xin[ip], u);                    //蝶形運算，詳見公式
                    xin[ip].real = xin[i].real - t.real;
                    xin[ip].imag = xin[i].imag - t.imag;
                    xin[i].real = xin[i].real + t.real;
                    xin[i].imag = xin[i].imag + t.imag;
                }
                u = EE(u, w);                           //改變系數，進行下一個蝶形運算
            }
        }
    }
}

void SPG(int *datainput)
{
    float mean_s = 0;				                                            //mean of s[].real
    float interval[51] = {0};
    float sn[FFT_N/2] = {0};
    float y[FFT_N] = {0};
    int chw = 5*log2(FFT_N/4)-4;
//set channel

for(int i = 0; i < FFT_N; i++)											//給結構體賦值
    {
    //data input
        float tmp = datainput[i];
        s[i].real = tmp;
        s[i].imag = 0;//虛部為0
        printf("%1f\t" , s[i].real);
    }
//means
    for(int i = 0; i < FFT_N; i++)
        mean_s = mean_s + (s[i].real / FFT_N);

    for(int i = 0; i < FFT_N; i++)
        s[i].real = (s[i].real - mean_s);


    FFT(s);

    for(int i = 0; i < FFT_N/2; i++)
    {
        s[i].real = sqrt(s[i].real * s[i].real + s[i].imag * s[i].imag) ;    //求變換後結果，存入復數的實部部分
//x_axis degree
        float tp = (i + 1);
        x_ticks[FFT_N/2 - i] = (FFT_N / tp);                          // length(x_ticks[i]) = FFT_N/2 - 1 ; x_ticks[FFT_N/2 -1] = 0

//inverse s[i] to sn[i]
        float tmp = s[i].real / FFT_N *2;
        sn[FFT_N/2 - i] = tmp;
    }
    //writeEXCEL(x_ticks, sn);

    //for(int i = 0; i < FFT_N/2; i++)
        //printf("%d\t %f\t %f\n", i, x_ticks[i], sn[i]);
        //printf("%1f\t %1f\n",x_ticks[i], sn[i]);
        //printf("%d %1f\n",i , sn[i]);

//set channel
    for(int i= 0; i <= chw; i++){                //set how many chs
        interval[i] = pow(2 , 0.2*(i+5));       //pow()用來求 x 的 y 次方
        //printf("%f\n", interval[i]);
    }

    int j = 0;
    for(int i= 1; i <= chw; i++)
    {
        float max = 0;
        int tmp = j;
        //find max btw interval[i] from interval[i + 1]
        if(x_ticks[j] > interval[i])
            max = sn[j];
        else
            while(x_ticks[j] <= interval[i])
            {
                if(sn[j] > max)
                    max = sn[j];
                j++;
            }
        y[i] = max;
        //printf("%d\t", j);
        printf("%1f\n", y[i]);
    }
}
