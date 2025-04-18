#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                             Tests for the Array Ledger
 *
 *--------------------------------------------------------------------------------------------------------------------------*/
#define TEST_BUF_ARRAY_LEDGER_CNT 10

static void
test_empty_full_array(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    i32 ibuf[TEST_BUF_ARRAY_LEDGER_CNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    PakArrayLedger array = pak_array_ledger_create(TEST_BUF_ARRAY_LEDGER_CNT);
    PakArrayLedger *ap = &array;

    // It should be empty now - for as many calls as I make.
    Assert(pak_array_ledger_empty(ap));
    Assert(!pak_array_ledger_full(ap));
    Assert(pak_array_ledger_len(ap) == 0);
    for (i32 i = 0; i < 5; ++i) 
    {
        Assert(pak_array_ledger_empty(ap));
        Assert(!pak_array_ledger_full(ap));
        Assert(pak_array_ledger_len(ap) == 0);
    }

    // Let's fill it up!
    for (i32 i = 0; i < TEST_BUF_ARRAY_LEDGER_CNT; ++i) 
    {
        Assert(!pak_array_ledger_full(ap)); // Should never be full in this loop

        size push_idx = pak_array_ledger_push_back_index(ap);
        ibuf[push_idx] = i;

        Assert(!pak_array_ledger_empty(ap)); // Should never be empty after we've pushed.
    }

    for (i32 i = 0; i < TEST_BUF_ARRAY_LEDGER_CNT; ++i) 
    {
        Assert(ibuf[i] == i);
    }

    Assert(pak_array_ledger_full(ap));
    Assert(pak_array_ledger_len(ap) == TEST_BUF_ARRAY_LEDGER_CNT);

    // All the rest of the pushes should fail
    for (i32 i = 0; i < 5; ++i) 
    {
        Assert(pak_array_ledger_full(ap));
        Assert(!pak_array_ledger_empty(ap));
        Assert(pak_array_ledger_len(ap) == TEST_BUF_ARRAY_LEDGER_CNT);
        Assert(pak_array_ledger_push_back_index(ap) == PAK_COLLECTION_FULL);
    }
}

/*----------------------------------------------------------------------------------------------------------------------------
 *                                                 All Array Ledger Tests
 *--------------------------------------------------------------------------------------------------------------------------*/
void
pak_array_ledger_tests(void)
{
    test_empty_full_array();
}
