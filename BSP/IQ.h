#ifndef __IQ_H
#define __IQ_H	 

#include "main.h"

// fs = 800kHz, IF = 200kHz ¡ú fs/IF = 4
// 2 * cos(2 * pi * IF / fs * n) = 2 * cos(pi * n / 2)
const float cos_table[4] = { 
    2.0f,   // n=0: 2 * cos(0)
    0.0f,   // n=1: 2 * cos(pi/2)
   -2.0f,   // n=2: 2 * cos(pi)
    0.0f    // n=3: 2 * cos(3pi/2)
};

// -2 * sin(2 * pi * IF / fs * n) = -2 * sin(pi * n / 2)
const float sin_table[4] = { 
    0.0f,   // n=0: -2 * sin(0)
   -2.0f,   // n=1: -2 * sin(pi/2)
    0.0f,   // n=2: -2 * sin(pi)
    2.0f    // n=3: -2 * sin(3pi/2)
};

void Run_IQ_Demodulation_800k(float* pRawData);

#endif


