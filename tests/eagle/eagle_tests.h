#ifndef _EAGLE_TESTS_H_
#define _EAGLE_TESTS_H_

#include <math.h>

#define COUNT_TRIALS (8 * 100000000)

#ifdef __AVX2__ 

typedef union
{
    __m256d vec;
    f64 arr[4];
} Vec256Pun;

#endif

#if ELK_AVX_512

typedef union
{
    __m512d vec;
    f64 arr[8];
} Vec512Pun;

#endif

#endif

