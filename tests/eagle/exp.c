#include "eagle_tests.h"

static f64 const min_exp_arg = -720.0;
static f64 const max_exp_arg = 720.0;

void
test_exp_scalar(void)
{
    srand(11);

    f64 min = min_exp_arg;
    f64 max = max_exp_arg;

    int64_t count = 0;
    int64_t count1 = 0;

    for(int64_t i = 0; i < COUNT_TRIALS; ++i)
    {
        f64 x = min + (f64)rand() / RAND_MAX * (max - min);
        f64 std_math = exp(x);
        f64 eagle_math = eagle_exp(x);

        int64_t diff = f64_ulp_distance(std_math, eagle_math, 10);
        ++count;

        if(diff > 0) { ++count1; }

        if(diff > 1)
        {
            printf("scalar input: %a stdlib exp: %a custom exp: %a difference in ulps %ld\n",
                    x, std_math, eagle_math, diff);
        }

        Assert(diff <= 1);
    }

    /* printf("exp - Scalar |  %.2lf%% of cases were 1 ULP off\n", (f64)count1 / (f64)count * 100.0); */
}

#ifdef __AVX2__ 

void
test_exp_avx2(void)
{
    srand(11);

    f64 min = min_exp_arg;
    f64 max = max_exp_arg;

    int64_t count = 0;
    int64_t count1 = 0;

    for(int64_t i = 0; i < COUNT_TRIALS; i += 4)
    {
        Vec256Pun x = {0};
        Vec256Pun eagle_math = {0};
        f64 std_math[4] ={0};

        for(int j = 0; j < 4; ++j)
        {
            x.arr[j] = min + (f64)rand() / RAND_MAX * (max - min);
            std_math[j] = exp(x.arr[j]);
        }

        eagle_math.vec = eagle_avx2_exp_pd(x.vec);

        for(int j = 0; j < 4; ++j)
        {
            int64_t diff = f64_ulp_distance(std_math[j], eagle_math.arr[j], 10);
            ++count;

            if(diff > 0) { ++count1; }

            if(diff > 1)
            {
                printf("avx2 input: %a stdlib exp: %a custom exp: %a difference in ulps %ld\n",
                        x.arr[j], std_math[j], eagle_math.arr[j], diff);
            }

            Assert(diff <= 1);
        }
    }

    /* printf("exp - AVX2   |  %.2lf%% of cases were 1 ULP off\n", (f64)count1 / (f64)count * 100.0); */
}

#endif

#if ELK_AVX_512

void
test_exp_avx512(void)
{
    srand(11);

    f64 min = min_exp_arg;
    f64 max = max_exp_arg;

    int64_t count = 0;
    int64_t count1 = 0;

    for(int64_t i = 0; i < COUNT_TRIALS; i += 8)
    {

        Vec512Pun x = {0};
        Vec512Pun eagle_math = {0};
        f64 std_math[8];

        for(int j = 0; j < 8; ++j)
        {
            f64 val = min + (f64)rand() / RAND_MAX * (max - min);
            x.arr[j] = val;
            std_math[j] = exp(val);
        }

        eagle_math.vec = eagle_avx512_exp_pd(x.vec);

        for(int j = 0; j < 8; ++j)
        {
            int64_t diff = f64_ulp_distance(std_math[j], eagle_math.arr[j], 10);
            ++count;

            if(diff > 0) { ++count1; }

            if(diff > 1)
            {
                printf("AVX512 input: %a stdlib exp: %a custom exp: %a difference in ulps %ld\n",
                        x.arr[j], std_math[j], eagle_math.arr[j], diff);
            }

            Assert(diff <= 1);
        }
    }

    /* printf("exp - AVX512 |  %.2lf%% of cases were 1 ULP off\n", (f64)count1 / (f64)count * 100.0); */
}

#endif

void
eagle_exp_tests(void)
{
    CoyProfileAnchor ap;

    ap = COY_START_PROFILE_BLOCK("exp-scalar-test");
    test_exp_scalar();
    COY_END_PROFILE(ap);

#ifdef __AVX2__
    ap = COY_START_PROFILE_BLOCK("exp-avx2-test");
    test_exp_avx2();
    COY_END_PROFILE(ap);
#endif

#if ELK_AVX_512
    ap = COY_START_PROFILE_BLOCK("exp-avx512-test");
    test_exp_avx512();
    COY_END_PROFILE(ap);
#endif
}

