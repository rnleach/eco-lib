#include "test.h"

#include <inttypes.h>

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                    Tests for Date
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

static void
test_date_addition()
{
    ElkDate epoch = elk_date_from_ymd(1970, 1, 1);
    ElkDate d1 = elk_date_from_ymd(1970, 1, 2);
    ElkDate d2 = elk_date_from_ymd(1970, 2, 1);

    Assert((epoch + 1) == d1);
    Assert((epoch + 31) == d2);
}

static void
test_date_parsing()
{
    char *test_vals[3] = {"01/01/0001", "01/01/1970", "04/15/1981"};
    ElkDate tgt_vals[3] = {0};
    tgt_vals[1] = elk_date_from_ymd(1970, 1, 1);
    tgt_vals[2] = elk_date_from_ymd(1981, 4, 15);

    size const CNT = sizeof(test_vals) / sizeof(test_vals[0]);

    for(size i = 0; i < CNT; ++i)
    {
        ElkDate parsed = {0};
        ElkStr tst = elk_str_from_cstring(test_vals[i]);
        b32 success = elk_str_parse_usa_date(tst, &parsed);
        Assert(success);
        Assert(parsed == tgt_vals[i]);
    }
}

static void
test_date_and_unixtime(void)
{
    i64 unix_time = 0;
    ElkDate converted_date = elk_date_from_unix_timestamp(unix_time);
    ElkDate date = elk_date_from_ymd(1970, 1, 1);
    Assert(date == converted_date);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     All date tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
elk_date_tests(void)
{
    test_date_addition();
    test_date_parsing();
    test_date_and_unixtime();
}

