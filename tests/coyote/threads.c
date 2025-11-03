#include "test.h"

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                 Tests for Threads
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct
{
    CoyChannel *outbound;
    u64 num_to_send;
} ProducerThreadData;

typedef struct
{
    CoyChannel *inbound;
    u64 num_received;
} ConsumerThreadData;

static void
producer(void *data)
{
    ProducerThreadData *pdata = data;
    CoyChannel *outbound = pdata->outbound;
    u64 num_to_send = pdata->num_to_send;

    coy_channel_wait_until_ready_to_send(outbound);

    b32 success = true;
    for(u64 i = 0; i < num_to_send && success; ++i)
    {
        success = coy_channel_send(outbound, (void *) 1);
        Assert(success);
    }

    coy_channel_done_sending(outbound);
}

static void
consumer(void *data)
{
    ConsumerThreadData *cdata = data;
    CoyChannel *inbound = cdata->inbound;

    coy_channel_wait_until_ready_to_receive(inbound);

    void *val;
    while(coy_channel_receive(inbound, &val))
    {
        cdata->num_received += (u64)val;
    }

    coy_channel_done_receiving(inbound);
}

static void
test_single_producer_single_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    ProducerThreadData pdata = { .outbound = &chan, .num_to_send = num_to_send };
    CoyThread producer_thread = {0};
    b32 success = coy_thread_create(&producer_thread, producer, &pdata);
    coy_channel_register_sender(&chan);
    Assert(success);

    ConsumerThreadData cdata = { .inbound = &chan, .num_received = 0 };
    CoyThread consumer_thread = {0};
    success = coy_thread_create(&consumer_thread, consumer, &cdata);
    coy_channel_register_receiver(&chan);
    Assert(success);

    success = eco_thread_join(&producer_thread);
    eco_thread_destroy(&producer_thread);
    Assert(success);

    success = eco_thread_join(&consumer_thread);
    eco_thread_destroy(&consumer_thread);
    Assert(success);

    coy_channel_destroy(&chan, NULL, NULL);

    Assert(cdata.num_received == num_to_send);
}

static void
test_single_producer_multiple_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    ProducerThreadData pdata = { .outbound = &chan, .num_to_send = num_to_send };
    CoyThread producer_thread = {0};
    b32 success = coy_thread_create(&producer_thread, producer, &pdata);
    coy_channel_register_sender(&chan);
    Assert(success);

    ConsumerThreadData cdata[4] = {0};
    CoyThread consumer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        cdata[i] = (ConsumerThreadData){ .inbound = &chan, .num_received = 0 };
        success = coy_thread_create(&consumer_threads[i], consumer, &cdata[i]);
        coy_channel_register_receiver(&chan);
        Assert(success);
    }

    success = eco_thread_join(&producer_thread);
    eco_thread_destroy(&producer_thread);
    Assert(success);

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&consumer_threads[i]);
        eco_thread_destroy(&consumer_threads[i]);
        Assert(success);
    }

    coy_channel_destroy(&chan, NULL, NULL);

    u64 total = 0;
    for(i32 i = 0; i < 4; ++i)
    {
        total += cdata[i].num_received;
    }

    Assert(total == num_to_send);
}

static void
test_multiple_producer_single_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    b32 success = true;
    ProducerThreadData pdata[4] = {0};
    CoyThread producer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        pdata[i] = (ProducerThreadData){ .outbound = &chan, .num_to_send = num_to_send };
        success = coy_thread_create(&producer_threads[i], producer, &pdata[i]);
        coy_channel_register_sender(&chan);
        Assert(success);
    }

    ConsumerThreadData cdata = { .inbound = &chan, .num_received = 0 };
    CoyThread consumer_thread = {0};
    success = coy_thread_create(&consumer_thread, consumer, &cdata);
    coy_channel_register_receiver(&chan);
    Assert(success);

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&producer_threads[i]);
        eco_thread_destroy(&producer_threads[i]);
        Assert(success);
    }

    success = eco_thread_join(&consumer_thread);
    eco_thread_destroy(&consumer_thread);
    Assert(success);

    coy_channel_destroy(&chan, NULL, NULL);

    Assert(cdata.num_received == 4 * num_to_send);
}

static void
test_multiple_producer_multiple_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    b32 success = true;
    ProducerThreadData pdata[4] = {0};
    CoyThread producer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        pdata[i] = (ProducerThreadData){ .outbound = &chan, .num_to_send = num_to_send };
        success = coy_thread_create(&producer_threads[i], producer, &pdata[i]);
        coy_channel_register_sender(&chan);
        Assert(success);
    }

    ConsumerThreadData cdata[4] = {0};
    CoyThread consumer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        cdata[i] = (ConsumerThreadData){ .inbound = &chan, .num_received = 0 };
        success = coy_thread_create(&consumer_threads[i], consumer, &cdata[i]);
        coy_channel_register_receiver(&chan);
        Assert(success);
    }

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&producer_threads[i]);
        eco_thread_destroy(&producer_threads[i]);
        Assert(success);
    }

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&consumer_threads[i]);
        eco_thread_destroy(&consumer_threads[i]);
        Assert(success);
    }

    coy_channel_destroy(&chan, NULL, NULL);

    u64 total = 0;
    for(i32 i = 0; i < 4; ++i)
    {
        total += cdata[i].num_received;
    }

    Assert(total == 4 * num_to_send);
}

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                               Tests for Task Threads
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct
{
    u64 num_to_send;
} ProducerTaskThreadData;

typedef struct
{
    u64 num_received;
} ConsumerTaskThreadData;

static void
task_producer(void *data, CoyChannel *in, CoyChannel *outbound)
{
    ProducerTaskThreadData *pdata = data;
    u64 num_to_send = pdata->num_to_send;

    coy_channel_wait_until_ready_to_send(outbound);

    b32 success = true;
    for(u64 i = 0; i < num_to_send && success; ++i)
    {
        success = coy_channel_send(outbound, (void *) 1);
        Assert(success);
    }

    coy_channel_done_sending(outbound);
}

static void
task_consumer(void *data, CoyChannel *inbound, CoyChannel *out)
{
    ConsumerTaskThreadData *cdata = data;

    coy_channel_wait_until_ready_to_receive(inbound);

    void *val;
    while(coy_channel_receive(inbound, &val))
    {
        cdata->num_received += (u64)val;
    }

    coy_channel_done_receiving(inbound);
}

static void
test_task_single_producer_single_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    ProducerTaskThreadData pdata = { .num_to_send = num_to_send };
    CoyTaskThread producer_thread = {0};
    b32 success = coy_task_thread_create(&producer_thread, task_producer, NULL, &chan, &pdata);
    Assert(success);

    ConsumerTaskThreadData cdata = { .num_received = 0 };
    CoyTaskThread consumer_thread = {0};
    success = coy_task_thread_create(&consumer_thread, task_consumer, &chan, NULL, &cdata);
    Assert(success);

    success = eco_thread_join(&producer_thread);
    eco_thread_destroy(&producer_thread);
    Assert(success);

    success = eco_thread_join(&consumer_thread);
    eco_thread_destroy(&consumer_thread);
    Assert(success);

    coy_channel_destroy(&chan, NULL, NULL);

    Assert(cdata.num_received == num_to_send);
}

static void
test_task_single_producer_multiple_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    ProducerTaskThreadData pdata = { .num_to_send = num_to_send };
    CoyTaskThread producer_thread = {0};
    b32 success = coy_task_thread_create(&producer_thread, task_producer, NULL, &chan, &pdata);
    Assert(success);

    ConsumerTaskThreadData cdata[4] = {0};
    CoyTaskThread consumer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        cdata[i] = (ConsumerTaskThreadData){ .num_received = 0 };
        success = coy_task_thread_create(&consumer_threads[i], task_consumer, &chan, NULL, &cdata[i]);
        Assert(success);
    }

    success = eco_thread_join(&producer_thread);
    eco_thread_destroy(&producer_thread);
    Assert(success);

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&consumer_threads[i]);
        eco_thread_destroy(&consumer_threads[i]);
        Assert(success);
    }

    coy_channel_destroy(&chan, NULL, NULL);

    u64 total = 0;
    for(i32 i = 0; i < 4; ++i)
    {
        total += cdata[i].num_received;
    }

    Assert(total == num_to_send);
}

static void
test_task_multiple_producer_single_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    b32 success = true;
    ProducerTaskThreadData pdata[4] = {0};
    CoyTaskThread producer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        pdata[i] = (ProducerTaskThreadData){ .num_to_send = num_to_send };
        success = coy_task_thread_create(&producer_threads[i], task_producer, NULL, &chan, &pdata[i]);
        Assert(success);
    }

    ConsumerTaskThreadData cdata = { .num_received = 0 };
    CoyTaskThread consumer_thread = {0};
    success = coy_task_thread_create(&consumer_thread, task_consumer, &chan, NULL, &cdata);
    Assert(success);

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&producer_threads[i]);
        eco_thread_destroy(&producer_threads[i]);
        Assert(success);
    }

    success = eco_thread_join(&consumer_thread);
    eco_thread_destroy(&consumer_thread);
    Assert(success);

    coy_channel_destroy(&chan, NULL, NULL);

    Assert(cdata.num_received == 4 * num_to_send);
}

static void
test_task_multiple_producer_multiple_consumer(i32 num_to_send)
{
    CoyChannel chan = coy_channel_create();

    b32 success = true;
    ProducerTaskThreadData pdata[4] = {0};
    CoyTaskThread producer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        pdata[i] = (ProducerTaskThreadData){ .num_to_send = num_to_send };
        success = coy_task_thread_create(&producer_threads[i], task_producer, NULL, &chan, &pdata[i]);
        Assert(success);
    }

    ConsumerTaskThreadData cdata[4] = {0};
    CoyTaskThread consumer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        cdata[i] = (ConsumerTaskThreadData){ .num_received = 0 };
        success = coy_task_thread_create(&consumer_threads[i], task_consumer, &chan, NULL, &cdata[i]);
        Assert(success);
    }

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&producer_threads[i]);
        eco_thread_destroy(&producer_threads[i]);
        Assert(success);
    }

    for(i32 i = 0; i < 4; ++i)
    {
        success = eco_thread_join(&consumer_threads[i]);
        eco_thread_destroy(&consumer_threads[i]);
        Assert(success);
    }

    coy_channel_destroy(&chan, NULL, NULL);

    u64 total = 0;
    for(i32 i = 0; i < 4; ++i)
    {
        total += cdata[i].num_received;
    }

    Assert(total == 4 * num_to_send);
}

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                 Tests for ThreadPool
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct
{
    i32 x;
    i32 two_x;
} CoyThreadPoolTestTaskData;

static inline void
coy_thread_pool_test_task_function(void *data)
{
    CoyThreadPoolTestTaskData *td = data;
    td->two_x = 2 * td->x;
}

static void
test_thread_pool(void)
{
#define NUM_TEST_TASKS 5000
    CoyThreadPool pool_ = {0};
    CoyThreadPool *pool = &pool_;
    coy_threadpool_initialize(pool, 5);

    CoyThreadPoolTestTaskData td[NUM_TEST_TASKS] = {0};
    CoyFuture futures[NUM_TEST_TASKS] = {0};

    for(i32 i = 0; i < NUM_TEST_TASKS; ++i)
    {
        td[i].x = i;
        futures[i] = coy_future_create(coy_thread_pool_test_task_function, &td[i]);
        coy_threadpool_submit(pool, &futures[i]);
    }

    b32 complete = false;
    while(!complete)
    {
        complete = true;
        for(i32 i = 0; i < NUM_TEST_TASKS; ++i)
        {
            if(coy_future_is_complete(&futures[i]))
            {
                Assert(td[i].two_x == td[i].x * 2);
                coy_future_mark_consumed(&futures[i]);
            }

            complete &= coy_future_is_consumed(&futures[i]);
        }
    }

    coy_threadpool_destroy(pool);
#undef NUM_TEST_TASKS
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                   All threads tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
coyote_threads_tests(void)
{
    fprintf(stderr,".spsc..");
    test_single_producer_single_consumer(1000000);
    test_single_producer_single_consumer(10);
    fprintf(stderr,".spmc..");
    test_single_producer_multiple_consumer(1000000);
    test_single_producer_multiple_consumer(10);
    fprintf(stderr,".mpsc 1000000..");
    test_multiple_producer_single_consumer(1000000);
    fprintf(stderr,".mpsc 10..");
    test_multiple_producer_single_consumer(10);
    fprintf(stderr,".mpmc..");
    test_multiple_producer_multiple_consumer(1000000);
    test_multiple_producer_multiple_consumer(10);

    fprintf(stderr,".task spsc..");
    test_task_single_producer_single_consumer(1000000);
    test_task_single_producer_single_consumer(10);
    fprintf(stderr,".task spmc..");
    test_task_single_producer_multiple_consumer(1000000);
    test_task_single_producer_multiple_consumer(10);
    fprintf(stderr,".task mpsc 1000000..");
    test_task_multiple_producer_single_consumer(1000000);
    fprintf(stderr,".task mpsc 10..");
    test_task_multiple_producer_single_consumer(10);
    fprintf(stderr,".task mpmc..");
    test_task_multiple_producer_multiple_consumer(1000000);
    test_task_multiple_producer_multiple_consumer(10);

    fprintf(stderr,".thread pool..");
    test_thread_pool();

}

