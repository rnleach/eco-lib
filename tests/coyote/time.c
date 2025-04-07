#include "test.h"

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                    Tests for Time
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static void
test_system_time(void)
{
    i64 now = coy_time_now();
    Assert(now > INT64_MIN && now < INT64_MAX);
}

static void
test_time_epoch_is_same_as_linux_epoch(void)
{
    // I can't really test this because the current time is always changing!!
    // But I can write a test that won't fail (false negative) for a long time.
    i64 now = coy_time_now();

    Assert(now > INT64_C(1697500800)); // Midnight GMT on Oct 17, 2023
    Assert(now < INT64_C(4102444800)); // Midnight GMT on Jan 01, 2100
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     All time tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
coyote_time_tests(void)
{
    test_system_time();
    test_time_epoch_is_same_as_linux_epoch();
}

