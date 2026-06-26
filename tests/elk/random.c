#include "test.h"

#include <inttypes.h>
#include <string.h>

/* Output the random numbers into a file so I can inspect them. */
#define WRITE_TEST_OUTPUT 0

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                  Test Random Number Generators
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_test_random_f64(void)
{

#if WRITE_TEST_OUTPUT
    FILE *out = fopen("random_f64_test.dat", "wb");
#endif

    ElkRandomState state_ = elk_random_state_create(111);
    ElkRandomState *state = &state_;

    for(size i = 0; i < 10000000; ++i)
    {
        f64 val = elk_random_state_uniform_f64(state);
        Assert(0.0 <= val && val <= 1.0);

#if WRITE_TEST_OUTPUT
        fprintf(out, "%.18lf\n", val);
#endif

    }

#if WRITE_TEST_OUTPUT
    fclose(out);
#endif

}

#if __AVX2__

#if WRITE_TEST_OUTPUT

typedef union
{
    __m256d vec;
    f64 arr[4];
} ElkM256dPun;

#endif

void
elk_test_random_f64_avx2(void)
{

#if WRITE_TEST_OUTPUT
    FILE *out = fopen("random_avx2_f64_test.dat", "wb");
#endif

    ElkAVX2RandomState state_ = elk_avx2_random_state_create(111);
    ElkAVX2RandomState *state = &state_;

    for(size i = 0; i < (10000000 / 4) + 1; ++i)
    {
        __m256d val = elk_avx2_random_state_uniform_f64(state);
        __m256d gt0 = _mm256_cmp_pd( val, _mm256_setzero_pd(), _CMP_GE_OQ);
        __m256d lt1 = _mm256_cmp_pd(val,_mm256_set1_pd(1.0), _CMP_LE_OQ);
        __m256d mask = _mm256_and_pd(gt0, lt1);
        i32 mask_bits = _mm256_movemask_pd(mask);

        Assert(mask_bits == 0xF);

#if WRITE_TEST_OUTPUT
        ElkM256dPun p = { .vec = val };
        for(i32 i = 0; i < 4; ++i)
        {
            fprintf(out, "%.18lf\n", p.arr[i]);
        }
#endif

    }

#if WRITE_TEST_OUTPUT
    fclose(out);
#endif

}

#endif

#if ELK_AVX_512

#if WRITE_TEST_OUTPUT

typedef union
{
    __m512d vec;
    f64 arr[8];
} ElkM512dPun;

#endif

void
elk_test_random_f64_avx512(void)
{

#if WRITE_TEST_OUTPUT
    FILE *out = fopen("random_avx512_f64_test.dat", "wb");
#endif

    ElkAVX512RandomState state_ = elk_avx512_random_state_create(111);
    ElkAVX512RandomState *state = &state_;

    for(size i = 0; i < (10000000 / 8) + 1; ++i)
    {
        __m512d val = elk_avx512_random_state_uniform_f64(state);
        __mmask8 gt0 = _mm512_cmp_pd_mask( val, _mm512_setzero_pd(), _CMP_GE_OQ);
        __mmask8 lt1 = _mm512_cmp_pd_mask(val,_mm512_set1_pd(1.0), _CMP_LE_OQ);
        u8 mask = (u8) (gt0 & lt1);

        Assert(mask == 0xFF);

#if WRITE_TEST_OUTPUT
        ElkM512dPun p = { .vec = val };
        for(i32 i = 0; i < 8; ++i)
        {
            fprintf(out, "%.18lf\n", p.arr[i]);
        }
#endif

    }

#if WRITE_TEST_OUTPUT
    fclose(out);
#endif

}

#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_random_tests(void)
{
    elk_test_random_f64();

#if __AVX2__
    elk_test_random_f64_avx2();
#endif

#if ELK_AVX_512
    elk_test_random_f64_avx512();
#endif

}
