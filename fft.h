#ifndef _FFT_H
#define _FFT_H
#endif // _FFT_H

#include <math.h>

#ifdef __cplusplus

extern "C" {
#endif

struct compx EE(struct compx ,struct compx );


void FFT(struct compx *);

void read();
void write(float* , float* );

void SPG(int *);

#ifdef __cplusplus
}
#endif
