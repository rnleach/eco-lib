#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                               Tests for the Queue Ledger
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
#define TEST_BUF_QUEUE_LEDGER_CNT 10

static void
test_empty_full_queue(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    i32 ibuf[TEST_BUF_QUEUE_LEDGER_CNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    PakQueueLedger queue = pak_queue_ledger_create(TEST_BUF_QUEUE_LEDGER_CNT);
    PakQueueLedger *qp = &queue;

    // It should be empty now - for as many calls as I make.
    Assert(pak_queue_ledger_empty(qp));
    Assert(!pak_queue_ledger_full(qp));
    for (i32 i = 0; i < 5; ++i) 
    {
        Assert(pak_queue_ledger_empty(qp));
        Assert(!pak_queue_ledger_full(qp));
        Assert(pak_queue_ledger_pop_front_index(qp) == PAK_COLLECTION_EMPTY);
    }

    // Let's fill it up!
    for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT; ++i) 
    {
        Assert(!pak_queue_ledger_full(qp)); // Should never be full in this loop

        size push_idx = pak_queue_ledger_push_back_index(qp);
        ibuf[push_idx] = i;

        Assert(!pak_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
    }

    Assert(pak_queue_ledger_full(qp));

    // All the rest of these should fail
    for (i32 i = 0; i < 5; ++i) 
    {
        Assert(pak_queue_ledger_full(qp));
        Assert(!pak_queue_ledger_empty(qp));
        Assert(pak_queue_ledger_push_back_index(qp) == PAK_COLLECTION_FULL);
    }

    // Let's empty it out.
    for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT; ++i) 
    {
        Assert(!pak_queue_ledger_empty(qp)); // Should never be empty at the start of this loop.

        size pop_idx = pak_queue_ledger_pop_front_index(qp);
        Assert(ibuf[pop_idx] == i); // They should come out in the order we put them in.

        Assert(!pak_queue_ledger_full(qp)); // Should never be full after we pop
    }

    // It should be empty now - for as many calls as I make.
    Assert(pak_queue_ledger_empty(qp));
    Assert(!pak_queue_ledger_full(qp));
    for (i32 i = 0; i < 5; ++i) 
    {
        Assert(pak_queue_ledger_empty(qp));
        Assert(!pak_queue_ledger_full(qp));
        Assert(pak_queue_ledger_pop_front_index(qp) == PAK_COLLECTION_EMPTY);
    }
}

static void
test_lots_of_throughput(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    i32 ibuf[TEST_BUF_QUEUE_LEDGER_CNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    PakQueueLedger queue = pak_queue_ledger_create(TEST_BUF_QUEUE_LEDGER_CNT);
    PakQueueLedger *qp = &queue;

    // Let's put a few in there.
    for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT / 2; ++i) 
    {
        Assert(!pak_queue_ledger_full(qp)); // Should never be full in this loop

        size push_idx = pak_queue_ledger_push_back_index(qp);
        ibuf[push_idx] = i;

        Assert(!pak_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
    }

    // Cycle through adding and removing from the queue
    i32 const reps = 100;
    for (i32 i = 0; i < reps; ++i) 
    {

        // Let's put a few more in there.
        for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT / 2; ++i) 
        {
            Assert(!pak_queue_ledger_full(qp)); // Should never be full in this loop

            size push_idx = pak_queue_ledger_push_back_index(qp);
            ibuf[push_idx] = i;

            Assert(!pak_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
        }

        // Let's pull a few out.
        for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT / 2; ++i) 
        {
            Assert(!pak_queue_ledger_empty(qp)); // Should never be empty in this loop

            size pop_idx = pak_queue_ledger_pop_front_index(qp);
            Assert(ibuf[pop_idx] == i);

            Assert(!pak_queue_ledger_full(qp)); // Should never be empty after we've pushed.
        }
    }
}

static void
test_test_peek(void)
{
    // Set up backing memory, in this case it's just an array on the stack.
    i32 ibuf[TEST_BUF_QUEUE_LEDGER_CNT] = {0};

    // Set up the ledger to track the capacity of the buffer.
    PakQueueLedger queue = pak_queue_ledger_create(TEST_BUF_QUEUE_LEDGER_CNT);
    PakQueueLedger *qp = &queue;

    // It should be empty now - for as many calls as I make.
    Assert(pak_queue_ledger_empty(qp));
    Assert(!pak_queue_ledger_full(qp));
    for (i32 i = 0; i < 5; ++i) {
        Assert(pak_queue_ledger_peek_front_index(qp) == PAK_COLLECTION_EMPTY);
    }

    // Let's fill it up!
    for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT; ++i) 
    {
        Assert(!pak_queue_ledger_full(qp)); // Should never be full in this loop

        size push_idx = pak_queue_ledger_push_back_index(qp);
        ibuf[push_idx] = i;

        Assert(!pak_queue_ledger_empty(qp)); // Should never be empty after we've pushed.
    }

    // Let's empty it out.
    for (i32 i = 0; i < TEST_BUF_QUEUE_LEDGER_CNT; ++i) 
    {
        Assert(!pak_queue_ledger_empty(qp)); // Should never be empty at the start of this loop.

        size peek_idx = pak_queue_ledger_peek_front_index(qp);
        Assert(ibuf[peek_idx] == i); // They should come out in the order we put them in.

        size pop_idx = pak_queue_ledger_pop_front_index(qp);
        Assert(ibuf[pop_idx] == i); // They should come out in the order we put them in.

        Assert(peek_idx == pop_idx);

        Assert(!pak_queue_ledger_full(qp)); // Should never be full after we pop
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    All Queue Ledger Tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
pak_queue_ledger_tests(void)
{
    test_empty_full_queue();
    test_lots_of_throughput();
    test_test_peek();
}
