
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
struct compx {float real,imag;};                                        //�w�q�@�Ӵ_�Ƶ��c
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

    nv2 = FFT_N / 2;                   //�ܧ}�B��A�Y��۵M�����ܦ��˦�ǡA���ιp�w��k
    nm1 = FFT_N - 1;
    for(i = 0; i < nm1; i++)
    {
        if(i < j)                      //�p�Gi<j,�Y�i���ܧ}
        {
            t = xin[j];
            xin[j] = xin[i];
            xin[i] = t;
        }
        k = nv2;                       //�Dj���U�@�ӭ˦��
        while( k <= j)                 //�p�Gk<=j,���j���̰��쬰1
        {
            j = j - k;                 //��̰����ܦ�0
            k = k / 2;                 //k/2�A���������A�̦������A�v�Ӥ���A����Y�Ӧ쬰0
        }
        j = j + k;                     //��0�אּ1
    }
    {
        int le , lei, ip;                            //FFT�B��֡A�ϥν��ιB�⧹��FFT�B��

        f = FFT_N;
        for(l = 1; (f=f/2)!=1; l++)                  //�p��l���ȡA�Y�p�⽺�ίż�
            ;
        for( m = 1; m <= l; m++)                         // ����ε��ż�
        {                                        //m��ܲ�m�Ž��ΡAl�����ί��`��l=log�]2�^N
            le = 2 << (m - 1);                            //le���ε��Z���A�Y��m�Ž��Ϊ����ε��۶Zle�I
            lei = le / 2;                               //�P�@���ε����ѥ[�B�⪺���I���Z��
            u.real = 1.0;                             //u�����ε��B��t�ơA��l�Ȭ�1
            u.imag = 0.0;
            w.real = cos(PI / lei);                     //w���t�ưӡA�Y��e�t�ƻP�e�@�Өt�ƪ���
            w.imag = -sin(PI / lei);
            for(j = 0;j <= lei - 1; j++)                   //����p�⤣�P�ؽ��ε��A�Y�p��t�Ƥ��P�����ε�
            {
                for(i = j; i <= FFT_N - 1; i = i + le)            //����P�@���ε��B��A�Y�p��t�ƬۦP���ε�
                {
                    ip = i + lei;                           //i�Aip���O��ܰѥ[���ιB�⪺��Ӹ`�I
                    t = EE(xin[ip], u);                    //���ιB��A�Ԩ�����
                    xin[ip].real = xin[i].real - t.real;
                    xin[ip].imag = xin[i].imag - t.imag;
                    xin[i].real = xin[i].real + t.real;
                    xin[i].imag = xin[i].imag + t.imag;
                }
                u = EE(u, w);                           //���ܨt�ơA�i��U�@�ӽ��ιB��
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

for(int i = 0; i < FFT_N; i++)											//�����c����
    {
    //data input
        float tmp = datainput[i];
        s[i].real = tmp;
        s[i].imag = 0;//�곡��0
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
        s[i].real = sqrt(s[i].real * s[i].real + s[i].imag * s[i].imag) ;    //�D�ܴ��ᵲ�G�A�s�J�_�ƪ��곡����
//x_axis degree
        float tp = (i + 1);
        x_ticks[FFT_N/2 - i] = (FFT_N / tp);                                     // length(x_ticks[i]) = FFT_N/2 - 1 ; x_ticks[FFT_N/2 -1] = 0

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
        interval[i] = pow(2 , 0.2*(i+5));       //pow() ��?�ΨӨD x �� y ��?�]����)
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
/*
void read()
{
    FILE *fpr;
//open file
    fpr=fopen(FILE_NAME,"r");
    //fpr=fopen("d:\\SPG_test.txt","r");
//read file
    for (int i = 0; i < FFT_N; i++){
        fscanf(fpr,"%d",&datain[i]);
        //printf("%d\t" , in[i]);
    }
//close file
    fclose(fpr);
}

void write(float x[], float data[])
{
    FILE *fpr;
//open file
    fpr=fopen("data_output.txt","w");
    //fpr=fopen("d:\\SPG_test.txt","r");
//write file
    fprintf(fpr,"x_ticks\t sn\n");
    for (int i = 0; i < FFT_N/2; i++){
        fprintf(fpr,"%f\t %f\n", x[i], data[i]);
        //printf("%d\t" , in[i]);
    }
//close file
    fclose(fpr);
}
*/
