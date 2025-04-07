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

    coy_channel_register_sender(outbound);
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

    coy_channel_register_receiver(inbound);
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
    Assert(success);

    ConsumerThreadData cdata = { .inbound = &chan, .num_received = 0 };
    CoyThread consumer_thread = {0};
    success = coy_thread_create(&consumer_thread, consumer, &cdata);
    Assert(success);

    success = coy_thread_join(&producer_thread);
    Assert(success);

    success = coy_thread_join(&consumer_thread);
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
    Assert(success);

    ConsumerThreadData cdata[4] = {0};
    CoyThread consumer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        cdata[i] = (ConsumerThreadData){ .inbound = &chan, .num_received = 0 };
        success = coy_thread_create(&consumer_threads[i], consumer, &cdata[i]);
        Assert(success);
    }

    success = coy_thread_join(&producer_thread);
    Assert(success);

    for(i32 i = 0; i < 4; ++i)
    {
        success = coy_thread_join(&consumer_threads[i]);
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
        Assert(success);
    }

    ConsumerThreadData cdata = { .inbound = &chan, .num_received = 0 };
    CoyThread consumer_thread = {0};
    success = coy_thread_create(&consumer_thread, consumer, &cdata);
    Assert(success);

    for(i32 i = 0; i < 4; ++i)
    {
        success = coy_thread_join(&producer_threads[i]);
        Assert(success);
    }

    success = coy_thread_join(&consumer_thread);
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
        Assert(success);
    }

    ConsumerThreadData cdata[4] = {0};
    CoyThread consumer_threads[4] = {0};
    for(i32 i = 0; i < 4; ++i)
    {
        cdata[i] = (ConsumerThreadData){ .inbound = &chan, .num_received = 0 };
        success = coy_thread_create(&consumer_threads[i], consumer, &cdata[i]);
        Assert(success);
    }

    for(i32 i = 0; i < 4; ++i)
    {
        success = coy_thread_join(&producer_threads[i]);
        Assert(success);
    }

    for(i32 i = 0; i < 4; ++i)
    {
        success = coy_thread_join(&consumer_threads[i]);
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
}

