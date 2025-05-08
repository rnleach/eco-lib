#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                               Tests for the Memory Pool
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
#define TEST_BUF_COUNT 10

static void
test_full_pool(void)
{
    _Alignas(_Alignof(f64)) byte buffer[TEST_BUF_COUNT * sizeof(f64)] = {0};
    MagStaticPool pool_obj = mag_static_pool_create(sizeof(f64), TEST_BUF_COUNT, buffer);
    MagStaticPool *pool = &pool_obj;

    f64 *dubs[TEST_BUF_COUNT] = {0};

    for (i32 i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        dubs[i] = mag_static_pool_alloc(pool);
        Assert(dubs[i]);

        *dubs[i] = (f64)i;
    }

    for (i32 i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        Assert(*dubs[i] == (f64)i);
    }

    // Test that it's full!
    for (i32 i = TEST_BUF_COUNT; i < 2 * TEST_BUF_COUNT; i++) 
    {
        f64 *no_dub = mag_static_pool_alloc(pool);
        Assert(!no_dub);
    }

    mag_static_pool_destroy(pool);
}

static void
test_pool_freeing(void)
{
    _Alignas(_Alignof(f64)) byte buffer[TEST_BUF_COUNT * sizeof(f64)] = {0};
    MagStaticPool pool_obj = mag_static_pool_create(sizeof(f64), TEST_BUF_COUNT, buffer);
    MagStaticPool *pool = &pool_obj;

    f64 *dubs[TEST_BUF_COUNT] = {0};

    for (i32 i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        dubs[i] = mag_static_pool_alloc(pool);
        Assert(dubs[i]);

        *dubs[i] = (f64)i;
    }

    for (i32 i = 0; i < TEST_BUF_COUNT; ++i) 
    {
        Assert(*dubs[i] == (f64)i);
    }

    // Half empty it!
    for (i32 i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        mag_static_pool_free(pool, dubs[2 * i]);
        dubs[2 * i] = NULL;
    }

    for (i32 i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        dubs[2 * i] = mag_static_pool_alloc(pool);
        Assert(dubs[2 * i]);

        *dubs[2 * i] = (f64)i;
    }

    for (i32 i = 0; i < TEST_BUF_COUNT / 2; i++) 
    {
        Assert(*dubs[2 * i] == (f64)i);
    }

    mag_static_pool_destroy(pool);
}

/*----------------------------------------------------------------------------------------------------------------------------
 *                                                 All Memory Pool Tests
 *--------------------------------------------------------------------------------------------------------------------------*/
void
magpie_pool_tests(void)
{
    test_full_pool();
    test_pool_freeing();
}
