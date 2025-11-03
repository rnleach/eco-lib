#ifndef _COYOTE_H_
#define _COYOTE_H_

#include <stdint.h>
#include <stddef.h>

#include "elk.h"
#include "magpie.h"

#include <immintrin.h>

#pragma warning(push)

/*---------------------------------------------------------------------------------------------------------------------------
 * Declare parts of the standard C library I use. These should almost always be implemented as compiler intrinsics anyway.
 *-------------------------------------------------------------------------------------------------------------------------*/

void *memset(void *buffer, int val, size_t num_bytes);
void *memcpy(void *dest, void const *src, size_t num_bytes);
void *memmove(void *dest, void const *src, size_t num_bytes);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Date and Time
 *-------------------------------------------------------------------------------------------------------------------------*/
static inline u64 coy_time_now(void); /* Get the current system time in seconds since midnight, Jan. 1 1970. */

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     Files & Paths
 *---------------------------------------------------------------------------------------------------------------------------
 * Check the 'valid' member of the structs to check for errors!
 */

typedef struct
{
    char *start; /* NULL indicates not a valid path element. */
    size len;    /* 0 indicates not a valid path element.    */
} CoyPathStr;

typedef struct
{
    char *full_path;       /* Pointer to null terminated string. (Non-owning!!!)        */
    CoyPathStr dir;        /* Directory, not including terminating directory separator. */
    CoyPathStr base;       /* Last element in the path, if it is a file.                */
    CoyPathStr extension;  /* File extension, if this path is a file.                   */
    b32 exists;            /* Does this path already exist on the system?               */
    b32 is_file;           /* Is this a file? If not, it's a directory.                 */
} CoyPathInfo;

/* If path exists and is a file, assumes file, if path exists and is a directory, assumes not file. If path does not exist,
 * assumes file IFF it has an extension. You can override any assumptions via the assume_file argument. In the event that
 * assume_file conflicts with the system information about an existing file, the system information is used.               */
static inline CoyPathInfo coy_path_info(char *path, b32 assume_file);

/* Append new path to the path in path_buffer, return true on success or false on error. path_buffer must be 0 terminated. */
static inline b32 coy_path_append(size buf_len, char path_buffer[], char const *new_path);

static inline size coy_file_size(char const *filename); /* size of a file in bytes, -1 on error. */

#define COY_FILE_READER_BUF_SIZE ECO_KiB(32)
typedef struct
{
    iptr handle; /* posix returns an int and windows a HANDLE (e.g. void*), this should work for all of them. */
    byte buffer[COY_FILE_READER_BUF_SIZE];
    size buf_cursor;
    size bytes_remaining;
    b32 valid;   /* error indicator */
} CoyFileReader;

static inline CoyFileReader coy_file_open_read(char const *filename);
static inline size coy_file_read(CoyFileReader *file, size buf_size, byte *buffer); /* return nbytes read or -1 on error                           */
static inline b32 coy_file_read_f64(CoyFileReader *file, f64 *val);
static inline b32 coy_file_read_f32(CoyFileReader *file, f32 *val);
static inline b32 coy_file_read_i8(CoyFileReader *file, i8 *val);
static inline b32 coy_file_read_i16(CoyFileReader *file, i16 *val);
static inline b32 coy_file_read_i32(CoyFileReader *file, i32 *val);
static inline b32 coy_file_read_i64(CoyFileReader *file, i64 *val);
static inline b32 coy_file_read_u8(CoyFileReader *file, u8 *val);
static inline b32 coy_file_read_u16(CoyFileReader *file, u16 *val);
static inline b32 coy_file_read_u32(CoyFileReader *file, u32 *val);
static inline b32 coy_file_read_u64(CoyFileReader *file, u64 *val);
static inline b32 coy_file_read_str(CoyFileReader *file, size *len, char *str);     /* set len to buffer length, updated to actual size on return. */
static inline void coy_file_reader_close(CoyFileReader *file);                      /* Must set valid member to false on success or failure!       */

/* Convenient loading of files. */
static inline size coy_file_slurp(char const *filename, byte **out, MagAllocator *alloc);   
static inline ElkStr coy_file_slurp_text_static(char const *filename, MagStaticArena *arena);
static inline ElkStr coy_file_slurp_text_dyn(char const *filename, MagDynArena *arena);
static inline ElkStr coy_file_slurp_text_allocator(char const *filename, MagAllocator *alloc);

#define eco_file_slurp_text(fname, alloc) _Generic((alloc),                                                                 \
                                         MagStaticArena *: coy_file_slurp_text_static,                                      \
                                         MagDynArena *:    coy_file_slurp_text_dyn,                                         \
                                         MagAllocator *:   coy_file_slurp_text_allocator                                    \
                                         )(fname, alloc)


#define COY_FILE_WRITER_BUF_SIZE ECO_KiB(32)
typedef struct
{
    iptr handle; /* posix returns an int and windows a HANDLE (e.g. void*), this should work for all of them. */
    byte buffer[COY_FILE_WRITER_BUF_SIZE];
    size buf_cursor;
    b32 valid;   /* error indicator */
} CoyFileWriter;

static inline CoyFileWriter coy_file_create(char const *filename); /* Truncate if it already exists, otherwise create it.        */
static inline CoyFileWriter coy_file_append(char const *filename); /* Create file if it doesn't exist yet, otherwise append.     */
static inline size coy_file_writer_flush(CoyFileWriter *file);     /* Close will also do this, only use if ALL you need is flush */
static inline void coy_file_writer_close(CoyFileWriter *file);     /* Must set valid member to false on success or failure!      */

static inline size coy_file_write(CoyFileWriter *file, size nbytes_write, byte const *buffer); /* return nbytes written or -1 on error */
static inline b32 coy_file_write_f64(CoyFileWriter *file, f64 val);
static inline b32 coy_file_write_f32(CoyFileWriter *file, f32 val);
static inline b32 coy_file_write_i8(CoyFileWriter *file, i8 val);
static inline b32 coy_file_write_i16(CoyFileWriter *file, i16 val);
static inline b32 coy_file_write_i32(CoyFileWriter *file, i32 val);
static inline b32 coy_file_write_i64(CoyFileWriter *file, i64 val);
static inline b32 coy_file_write_u8(CoyFileWriter *file, u8 val);
static inline b32 coy_file_write_u16(CoyFileWriter *file, u16 val);
static inline b32 coy_file_write_u32(CoyFileWriter *file, u32 val);
static inline b32 coy_file_write_u64(CoyFileWriter *file, u64 val);
static inline b32 coy_file_write_str(CoyFileWriter *file, size len, char *str);

typedef struct
{
    size size_in_bytes;     /* size of the file */
    byte const *data; 
    iptr _internal[2];      /* implementation specific data */
    b32 valid;              /* error indicator */
} CoyMemMappedFile;

static inline CoyMemMappedFile coy_memmap_read_only(char const *filename);
static inline void coy_memmap_close(CoyMemMappedFile *file);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                File System Interactions
 *---------------------------------------------------------------------------------------------------------------------------
 * Check the 'valid' member of the structs to check for errors!
 *
 * WARNING: NONE OF THESE ARE THREADSAFE.
 */

typedef struct
{
    iptr os_handle;         /* for internal use only */
    char const *file_extension;
    b32 valid;
} CoyFileNameIter;

/* Create an iterator. file_extension can be NULL if you want all files. Does not list directories. NOT THREADSAFE. */
static inline CoyFileNameIter coy_file_name_iterator_open(char const *directory_path, char const *file_extension);

/* Returns NULL when done. Copy the string if you need it, it will be overwritten on the next call. NOT THREADSAFE. */
static inline char const *coy_file_name_iterator_next(CoyFileNameIter *cfni);
static inline void coy_file_name_iterator_close(CoyFileNameIter *cfin); /* should leave the argument zeroed. NOT THREADSAFE. */

/*---------------------------------------------------------------------------------------------------------------------------
 *                                         Dynamically Loading Shared Libraries
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * WARNING: NONE OF THESE ARE THREADSAFE.
 */
typedef struct
{
    void *handle;
} CoySharedLibHandle;

static inline CoySharedLibHandle coy_shared_lib_load(char const *lib_name);
static inline void coy_shared_lib_unload(CoySharedLibHandle handle);
static inline void *coy_share_lib_load_symbol(CoySharedLibHandle handle, char const *symbol_name);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     Terminal Info
 *-------------------------------------------------------------------------------------------------------------------------*/
typedef struct
{
    i32 columns;
    i32 rows;
} CoyTerminalSize;

static inline CoyTerminalSize coy_get_terminal_size(void);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                           Multi-threading & Syncronization
 *---------------------------------------------------------------------------------------------------------------------------
 * Basic threading and syncronization.
 */

/* Windows requires a u32 return type while Linux (pthreads) requires a void*. Just return 0 or 1 to indicate success
 * and Coyote will cast it to the correct type for the API.
 */

typedef void (*CoyThreadFunc)(void *thread_data);

typedef struct
{
    _Alignas(16) byte handle[32];
    CoyThreadFunc func;
    void *thread_data;
} CoyThread;

typedef struct 
{
    _Alignas(16) byte mutex[64];
    b32 valid;
} CoyMutex;

typedef struct
{
    _Alignas(16) byte cond_var[64];
    b32 valid;
} CoyCondVar;

static inline b32 coy_thread_create(CoyThread *thrd, CoyThreadFunc func, void *thread_data);  /* Returns false on failure. */
static inline b32 coy_thread_join(CoyThread *thread);                              /* Returns false if there was an error. */
static inline void coy_thread_destroy(CoyThread *thread);

static inline CoyMutex coy_mutex_create(void);
static inline b32 coy_mutex_lock(CoyMutex *mutex);     /* Block, return false on failure. */
static inline b32 coy_mutex_unlock(CoyMutex *mutex);   /* Return false on failure.        */
static inline void coy_mutex_destroy(CoyMutex *mutex); /* Must set valid member to false. */

static inline CoyCondVar coy_condvar_create(void);
static inline b32 coy_condvar_sleep(CoyCondVar *cv, CoyMutex *mtx);
static inline b32 coy_condvar_wake(CoyCondVar *cv);
static inline b32 coy_condvar_wake_all(CoyCondVar *cv);
static inline void coy_condvar_destroy(CoyCondVar *cv); /* Must set valid member to false. */

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Thread Safe Channel
 *---------------------------------------------------------------------------------------------------------------------------
 * Threadsafe channel for sending / receiving pointers. Multiple Producer / Multiple Consumer (mpmc)
 */
#define COYOTE_CHANNEL_SIZE 64
typedef struct
{
    size head;
    size tail;
    size count;
    CoyMutex mtx;
    CoyCondVar space_available;
    CoyCondVar data_available;
    i32 num_producers_started;
    i32 num_producers_finished;
    i32 num_consumers_started;
    i32 num_consumers_finished;
    void *buf[COYOTE_CHANNEL_SIZE];
} CoyChannel;

static inline CoyChannel coy_channel_create(void);
static inline void coy_channel_destroy(CoyChannel *chan, void(*free_func)(void *ptr, void *ctx), void *free_ctx);
static inline void coy_channel_wait_until_ready_to_receive(CoyChannel *chan);
static inline void coy_channel_wait_until_ready_to_send(CoyChannel *chan);
static inline void coy_channel_register_sender(CoyChannel *chan);   /* Call from thread that created channel, not the one using it. */
static inline void coy_channel_register_receiver(CoyChannel *chan); /* Call from thread that created channel, not the one using it. */
static inline void coy_channel_done_sending(CoyChannel *chan);      /* Call from the thread using the channel.                      */
static inline void coy_channel_done_receiving(CoyChannel *chan);    /* Call from the thread using the channel.                      */
static inline b32 coy_channel_send(CoyChannel *chan, void *data);
static inline b32 coy_channel_receive(CoyChannel *chan, void **out);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Task Thread
 *---------------------------------------------------------------------------------------------------------------------------
 * A thread paired with input and output channels. The channels need to be set up separately. This is useful for building
 * pipeline style concurrency. Since CoyChannel is multi-producer, multi-consumer channel, each stage of a pipeline can
 * fork out for parallelism or fork in for aggregation.
 *
 * The create function takes care of calling register sender / receiver on the channels for the user, but the user must
 * must still call the *done* sending / receiving functions from inside the CoyTaskThreadFunc.
 */

typedef void (*CoyTaskThreadFunc)(void *thread_data, CoyChannel *input, CoyChannel *output);

typedef struct
{
    _Alignas(16) byte handle[32];
    CoyTaskThreadFunc func;
    CoyChannel *input;
    CoyChannel *output;
    void *thread_data;
} CoyTaskThread;

static inline b32 coy_task_thread_create(CoyTaskThread *thread, CoyTaskThreadFunc func, CoyChannel *in, CoyChannel *out, void *thread_data);
static inline b32 coy_task_thread_join(CoyTaskThread *thread);
static inline void coy_task_thread_destroy(CoyTaskThread *thread);

#define eco_thread_join(thrd) _Generic((thrd),                                                                              \
                                CoyThread *: coy_thread_join,                                                               \
                                CoyTaskThread *: coy_task_thread_join                                                       \
                              )(thrd)

#define eco_thread_destroy(thrd) _Generic((thrd),                                                                           \
                                   CoyThread *: coy_thread_destroy,                                                         \
                                   CoyTaskThread *: coy_task_thread_destroy                                                 \
                                 )(thrd)

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      Thread Pool
 *---------------------------------------------------------------------------------------------------------------------------
 * A pool of worker threads.
 */

typedef enum 
{ 
    COY_FUTURE_STATE_ERROR,     /* Initialization is an error! If zero initialized.                                        */
    COY_FUTURE_STATE_CREATED,   /* Created but not yet submitted.                                                          */
    COY_FUTURE_STATE_PENDING,   /* It's been added to the queue, but not started.                                          */
    COY_FUTURE_STATE_RUNNING,   /* It's currently running.                                                                 */
    COY_FUTURE_STATE_COMPLETE,  /* The task is done and ready to consume.                                                  */
    COY_FUTURE_STATE_CONSUMED   /* Let the client mark that they have consumed the result; the future is no longer needed. */
} CoyTaskState;

typedef struct
{
    volatile CoyTaskState state;
    CoyThreadFunc function;
    void *future_data;                /* function arguments and return values go in here. */
} CoyFuture;

static inline CoyFuture coy_future_create(CoyThreadFunc function, void *future_data);
static inline CoyTaskState coy_future_get_task_state(CoyFuture *fut);
static inline b32 coy_future_is_complete(CoyFuture *fut);
static inline void coy_future_mark_consumed(CoyFuture *fut);
static inline b32 coy_future_is_consumed(CoyFuture *fut);

#define COY_MAX_THREAD_POOL_SIZE 32
typedef struct
{
    CoyChannel queue;
    size nthreads;
    CoyThread threads[COY_MAX_THREAD_POOL_SIZE];
} CoyThreadPool;

static inline CoyThreadPool coy_threadpool_create(size nthreads);
static inline void coy_threadpool_destroy(CoyThreadPool *pool);    /* Finish pending tasks and shut down. */
static inline void coy_threadpool_submit(CoyThreadPool *pool, CoyFuture *fut);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    Profiling Tools
 *---------------------------------------------------------------------------------------------------------------------------
 * Macro enabled tools for profiling code.
 */

#ifndef COY_PROFILE
#define COY_PROFILE 0
#endif

#if COY_PROFILE
#define COY_PROFILE_NUM_BLOCKS 64
#else
#define COY_PROFILE_NUM_BLOCKS 1
#endif

typedef struct
{
    u64 tsc_elapsed_inclusive;
    u64 tsc_elapsed_exclusive;

    u64 hit_count;
    i32 ref_count;

    u64 bytes;

    char const *label;

    double exclusive_pct;
    double inclusive_pct;
    double gibibytes_per_second;
} CoyBlockProfiler;

#if COY_PROFILE
typedef struct
{
    u64 start;
    u64 old_tsc_elapsed_inclusive;
    i32 index;
    i32 parent_index;
} CoyProfileAnchor;
#else
typedef u32 CoyProfileAnchor;
#endif

typedef struct
{
    CoyBlockProfiler blocks[COY_PROFILE_NUM_BLOCKS];
    i32 current_block;

    u64 start;

    double total_elapsed;
    u64 freq;
} GlobalProfiler;

static GlobalProfiler coy_global_profiler;

/* CPU & timing */
static inline void coy_profile_begin(void);
static inline void coy_profile_end(void);
static inline u64 coy_profile_read_cpu_timer(void);
static inline u64 coy_profile_estimate_cpu_timer_freq(void);

/* OS Counters (page faults, etc.) */
static inline void coy_profile_initialize_os_metrics(void);
static inline void coy_profile_finalize_os_metrics(void);
static inline u64 coy_profile_read_os_page_fault_count(void);

#define COY_START_PROFILE_FUNCTION_BYTES(bytes) coy_profile_start_block(__func__, __COUNTER__, bytes)
#define COY_START_PROFILE_FUNCTION coy_profile_start_block(__func__, __COUNTER__, 0)
#define COY_START_PROFILE_BLOCK_BYTES(name, bytes) coy_profile_start_block(name, __COUNTER__, bytes)
#define COY_START_PROFILE_BLOCK(name) coy_profile_start_block(name, __COUNTER__, 0)
#define COY_END_PROFILE(ap) coy_profile_end_block(&ap)

#if COY_PROFILE
#define COY_PROFILE_STATIC_CHECK _Static_assert(__COUNTER__ <= ECO_ARRAY_SIZE(coy_global_profiler.blocks), "not enough profiler blocks")
#else
#define COY_PROFILE_STATIC_CHECK
#endif
/*---------------------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *                                          Implementation of `inline` functions.
 *                                                      Internal Only
 *
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/* assumes zero terminated string returned from OS - not for general use. */
static inline char const *
coy_file_extension(char const *path)
{
    char const *extension = path;
    char const *next_char = path;
    while(*next_char)
    {
        if(*next_char == '.')
        {
            extension = next_char + 1;
        }
        ++next_char;
    }
    return extension;
}

/* assumes zero terminated string returned from OS - not for general use. */
static inline b32
coy_null_term_strings_equal(char const *left, char const *right)
{
    char const *l = left;
    char const *r = right;

    while(*l || *r)
    {
        if(*l != *r)
        {
            return false;
        }

        ++l;
        ++r;
    }

    return true;
}

static inline b32 
coy_file_write_f64(CoyFileWriter *file, f64 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_f32(CoyFileWriter *file, f32 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_i8(CoyFileWriter *file, i8 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_i16(CoyFileWriter *file, i16 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_i32(CoyFileWriter *file, i32 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_i64(CoyFileWriter *file, i64 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_u8(CoyFileWriter *file, u8 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_u16(CoyFileWriter *file, u16 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_u32(CoyFileWriter *file, u32 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_u64(CoyFileWriter *file, u64 val)
{
    size nbytes = coy_file_write(file, sizeof(val), (byte *)&val);
    if(nbytes != sizeof(val)) { return false; }
    return true;
}

static inline b32 
coy_file_write_str(CoyFileWriter *file, size len, char *str)
{
    _Static_assert(sizeof(size) == sizeof(i64), "must not be on 64 bit!");
    b32 success = coy_file_write_i64(file, len);
    StopIf(!success, return false);
    if(len > 0)
    {
        size nbytes = coy_file_write(file, len, (byte *)str);
        if(nbytes != len) { return false; }
    }
    return true;
}

static inline b32 
coy_file_read_f64(CoyFileReader *file, f64 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_f32(CoyFileReader *file, f32 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_i8(CoyFileReader *file, i8 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_i16(CoyFileReader *file, i16 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_i32(CoyFileReader *file, i32 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_i64(CoyFileReader *file, i64 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_u8(CoyFileReader *file, u8 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_u16(CoyFileReader *file, u16 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_u32(CoyFileReader *file, u32 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_u64(CoyFileReader *file, u64 *val)
{
    size nbytes = coy_file_read(file, sizeof(*val), (byte *)val);
    if(nbytes != sizeof(*val)) { return false; }
    return true;
}

static inline b32 
coy_file_read_str(CoyFileReader *file, size *len, char *str)
{
    i64 str_len = 0;
    b32 success = coy_file_read_i64(file, &str_len);
    StopIf(!success || str_len > *len, return false);

    if(str_len > 0)
    {
        success = coy_file_read(file, str_len, (byte *)str) > 0;
        StopIf(!success, return false);
    }
    else
    {
        /* Clear the provided buffer. */
        memset(str, 0, *len);
    }

    *len = str_len;
    return true;
}

static inline CoyChannel 
coy_channel_create(void)
{
    CoyChannel chan = 
        {
            .head = 0,
            .tail = 0,
            .count = 0,
            .mtx = coy_mutex_create(),
            .space_available = coy_condvar_create(),
            .data_available = coy_condvar_create(),
            .num_producers_started = 0,
            .num_producers_finished = 0,
            .num_consumers_started = 0,
            .num_consumers_finished = 0,
            .buf = {0},
        };

    Assert(chan.mtx.valid);
    Assert(chan.space_available.valid);
    Assert(chan.data_available.valid);

    return chan;
}

static inline void 
coy_channel_destroy(CoyChannel *chan, void(*free_func)(void *ptr, void *ctx), void *free_ctx)
{
    Assert(chan->num_producers_started == chan->num_producers_finished);
    Assert(chan->num_consumers_started == chan->num_consumers_finished);
    if(free_func)
    {
        while(chan->count > 0)
        {
            free_func(chan->buf[chan->head % COYOTE_CHANNEL_SIZE], free_ctx);
            chan->head += 1;
            chan->count -= 1;
        }

        Assert(chan->head == chan->tail && chan->count == 0);
    }

    coy_mutex_destroy(&chan->mtx);
    coy_condvar_destroy(&chan->space_available);
    coy_condvar_destroy(&chan->data_available);
    *chan = (CoyChannel){0};
}

static inline void 
coy_channel_wait_until_ready_to_receive(CoyChannel *chan)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);

    while(chan->num_producers_started == 0) 
    {
        coy_condvar_sleep(&chan->data_available, &chan->mtx); 
    }

    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);
}

static inline void 
coy_channel_wait_until_ready_to_send(CoyChannel *chan)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);
    while(chan->num_consumers_started == 0) { coy_condvar_sleep(&chan->space_available, &chan->mtx); }
    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);
}

static inline void 
coy_channel_register_sender(CoyChannel *chan)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);
    chan->num_producers_started += 1;

    if(chan->num_producers_started == 1)
    {
        /* Broadcast here so any threads blocked on coy_channel_wait_until_ready_to_receive can progress. If the number of
         * producers is greater than 1, then this was already sent.
         */
        success = coy_condvar_wake_all(&chan->data_available);
        Assert(success);
    }

    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);
}

static inline void 
coy_channel_register_receiver(CoyChannel *chan)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);

    chan->num_consumers_started += 1;

    if(chan->num_consumers_started == 1)
    {
        /* Broadcast here so any threads blocked on coy_channel_wait_until_ready_to_send can progress. If the number of
         * consumers is greater than 1, then this was already sent.
         */
        success = coy_condvar_wake_all(&chan->space_available);
        Assert(success);
    }

    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);
}

static inline void 
coy_channel_done_sending(CoyChannel *chan)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);
    Assert(chan->num_producers_started > 0);

    chan->num_producers_finished += 1;

    if(chan->num_producers_started == chan->num_producers_finished)
    {
        /* Broadcast in case any thread is waiting for data to become available that won't because there are no more
         * producers running. This will let them check the number of producers started/finished and realize nothing is
         * coming so they too can quit.
         */
        success = coy_condvar_wake_all(&chan->data_available);
        Assert(success);
    }
    else
    {
        /* Deadlock may occur if I don't do this. This thread got signaled when others should have. */
        success = coy_condvar_wake_all(&chan->space_available);
        Assert(success);
    }

    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);
}

static inline void 
coy_channel_done_receiving(CoyChannel *chan)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);
    Assert(chan->num_consumers_started > 0);

    chan->num_consumers_finished += 1;

    if(chan->num_consumers_started == chan->num_consumers_finished)
    {
        /* Broadcast in case any thread is waiting to send data that they will never be able to send because there are no
         * consumers to receive it! This will let them check the number of consumers started/finished and realize space 
         * will never become available.
         */
        success = coy_condvar_wake_all(&chan->space_available);
        Assert(success);
    }
    else
    {
        /* Deadlock may occur if I don't do this. This thread got signaled when others should have. */
        success = coy_condvar_wake_all(&chan->data_available);
        Assert(success);
    }

    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);
}

static inline b32 
coy_channel_send(CoyChannel *chan, void *data)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);
    Assert(chan->num_producers_started > 0);

    while(chan->count == COYOTE_CHANNEL_SIZE && chan->num_consumers_started != chan->num_consumers_finished)
    {
        success = coy_condvar_sleep(&chan->space_available, &chan->mtx);
        Assert(success);
    }

    Assert(chan->count < COYOTE_CHANNEL_SIZE || chan->num_consumers_started == chan->num_consumers_finished);

    if(chan->num_consumers_started > chan->num_consumers_finished)
    {
        chan->buf[(chan->tail) % COYOTE_CHANNEL_SIZE] = data;
        chan->tail += 1;
        chan->count += 1;

        /* If the count was increased to 1, then someone may have been waiting to be notified! */
        if(chan->count == 1)
        {
            success = coy_condvar_wake_all(&chan->data_available);
            Assert(success);
        }

        success = coy_mutex_unlock(&chan->mtx);
        Assert(success);
        return true;
    }
    else
    {
        /* Space will never become available. */
        success = coy_mutex_unlock(&chan->mtx);
        Assert(success);
        return false;
    }
}

static inline b32 
coy_channel_receive(CoyChannel *chan, void **out)
{
    b32 success = coy_mutex_lock(&chan->mtx);
    Assert(success);

    while(chan->count == 0 && chan->num_producers_started > chan->num_producers_finished)
    {
        success = coy_condvar_sleep(&chan->data_available, &chan->mtx);
        Assert(success);
    }

    Assert(chan->count > 0 || chan->num_producers_started == chan->num_producers_finished);

    *out = NULL;
    if(chan->count > 0)
    {
        *out = chan->buf[(chan->head) % COYOTE_CHANNEL_SIZE];
        chan->head += 1;
        chan->count -= 1;
    }
    else
    {
        /* Nothing more is coming, to get here num_producers_started must num_producers_finished. */
        success = coy_mutex_unlock(&chan->mtx);
        Assert(success);
        return false;
    }

    /* If the queue was full before, send a signal to let other's know there is room now. */
    if( chan->count + 1 == COYOTE_CHANNEL_SIZE)
    {
        success = coy_condvar_wake_all(&chan->space_available);
        Assert(success);
    }
    
    success = coy_mutex_unlock(&chan->mtx);
    Assert(success);

    return true;
}

typedef struct
{
    b32 initialized;
    uptr handle;
} CoyOsMetrics;

static CoyOsMetrics coy_global_os_metrics;

/* NOTE: force the __COUNTER__ to start at 1, so I can offset and NOT use position 0 of global_profiler.blocks array */
i64 dummy =  __COUNTER__; 

static inline u64 coy_profile_get_os_timer_freq(void);
static inline u64 coy_profile_read_os_timer(void);

static inline u64
coy_profile_estimate_cpu_timer_freq(void)
{
	u64 milliseconds_to_wait = 100;
	u64 os_freq = coy_profile_get_os_timer_freq();

	u64 cpu_start = coy_profile_read_cpu_timer();
	u64 os_start = coy_profile_read_os_timer();
	u64 os_end = 0;
	u64 os_elapsed = 0;
	u64 os_wait_time = os_freq * milliseconds_to_wait / 1000;
	while(os_elapsed < os_wait_time)
	{
		os_end = coy_profile_read_os_timer();
		os_elapsed = os_end - os_start;
	}
	
	u64 cpu_end = coy_profile_read_cpu_timer();
	u64 cpu_elapsed = cpu_end - cpu_start;
	
	u64 cpu_freq = 0;
	if(os_elapsed)
	{
		cpu_freq = os_freq * cpu_elapsed / os_elapsed;
	}
	
	return cpu_freq;
}

static inline void
coy_profile_begin(void)
{
    coy_global_profiler.start = coy_profile_read_cpu_timer();
    coy_global_profiler.blocks[0].label = "Global";
    coy_global_profiler.blocks[0].hit_count++;
}

#pragma warning(disable : 4723)
static inline void
coy_profile_end(void)
{
    u64 end = coy_profile_read_cpu_timer();
    u64 total_elapsed = end - coy_global_profiler.start;

    CoyBlockProfiler *gp = coy_global_profiler.blocks;
    gp->tsc_elapsed_inclusive = total_elapsed;
    gp->tsc_elapsed_exclusive += total_elapsed;

    u64 freq = coy_profile_estimate_cpu_timer_freq();
    f64 const ZERO = 0.0;
    f64 const A_NAN = 0.0 / ZERO;
    if(freq)
    {
        coy_global_profiler.total_elapsed = (double)total_elapsed / (double)freq;
        coy_global_profiler.freq = freq;
    }
    else
    {
        coy_global_profiler.total_elapsed = A_NAN;
        coy_global_profiler.freq = 0;
    }

    for(i32 i = 0; i < ECO_ARRAY_SIZE(coy_global_profiler.blocks); ++i)
    {
        CoyBlockProfiler *block = coy_global_profiler.blocks + i;
        if(block->tsc_elapsed_inclusive)
        {
            block->exclusive_pct = (double)block->tsc_elapsed_exclusive / (double) total_elapsed * 100.0;
            block->inclusive_pct = (double)block->tsc_elapsed_inclusive / (double) total_elapsed * 100.0;
            if(block->bytes && freq)
            {
                double gib = (double)block->bytes / (1024 * 1024 * 1024);
                block->gibibytes_per_second = gib * (double) freq / (double)block->tsc_elapsed_inclusive;
            }
            else
            {
                block->gibibytes_per_second = A_NAN;
            }
        }
        else
        {
            block->exclusive_pct = A_NAN;
            block->inclusive_pct = A_NAN;
            block->gibibytes_per_second = A_NAN;
        }
    }
}
#pragma warning(default : 4723)

static inline CoyProfileAnchor 
coy_profile_start_block(char const *label, i32 index, u64 bytes_processed)
{
#if COY_PROFILE
    /* Update global state */
    i32 parent_index = coy_global_profiler.current_block;
    coy_global_profiler.current_block = index;

    /* update block profiler for this block */
    CoyBlockProfiler *block = coy_global_profiler.blocks + index;
    block->hit_count++;
    block->ref_count++;
    block->bytes += bytes_processed;
    block->label = label; /* gotta do it every time */

    /* build anchor to track this block */
    u64 start = coy_profile_read_cpu_timer();
    return (CoyProfileAnchor)
        { 
            .index = index, 
            .parent_index = parent_index, 
            .start=start, 
            .old_tsc_elapsed_inclusive = block->tsc_elapsed_inclusive 
        };
#else
    return -1;
#endif
}

static inline void 
coy_profile_end_block(CoyProfileAnchor *anchor)
{
#if COY_PROFILE
    /* read the end time, hopefully before the rest of this stuff */
    u64 end = coy_profile_read_cpu_timer();
    u64 elapsed = end - anchor->start;

    /* update global state */
    coy_global_profiler.current_block = anchor->parent_index;

    /* update the parent block profilers state */
    CoyBlockProfiler *parent = coy_global_profiler.blocks + anchor->parent_index;
    parent->tsc_elapsed_exclusive -= elapsed;

    /* update block profiler state */
    CoyBlockProfiler *block = coy_global_profiler.blocks + anchor->index;
    block->tsc_elapsed_exclusive += elapsed;
    block->tsc_elapsed_inclusive = anchor->old_tsc_elapsed_inclusive + elapsed;
    block->ref_count--;
#endif
}

/* return size in bytes of the loaded data or -1 on error. If buffer is too small, load nothing and return -1 */
static inline size coy_file_slurp_internal(char const *filename, size buf_size, byte *buffer);

static inline size 
coy_file_slurp(char const *filename, byte **out, MagAllocator *alloc)
{
    size fsize = coy_file_size(filename);
    StopIf(fsize < 0, goto ERR_RETURN);

    *out = mag_allocator_nmalloc(alloc, fsize, byte);
    StopIf(!*out, goto ERR_RETURN);

    size size_read = coy_file_slurp_internal(filename, fsize, *out);
    StopIf(fsize != size_read, goto ERR_RETURN);

    return fsize;

ERR_RETURN:
    *out = NULL;
    return -1;
}

static inline ElkStr 
coy_file_slurp_text_static(char const *filename, MagStaticArena *arena)
{
    size fsize = coy_file_size(filename);
    StopIf(fsize < 0, goto ERR_RETURN);

    byte *out = mag_static_arena_nmalloc(arena, fsize, byte);
    StopIf(!out, goto ERR_RETURN);

    size size_read = coy_file_slurp_internal(filename, fsize, out);
    StopIf(fsize != size_read, goto ERR_RETURN);

    return (ElkStr){ .start = out, .len = fsize };

ERR_RETURN:
    return (ElkStr){ .start = NULL, .len = 0 };
}

static inline ElkStr 
coy_file_slurp_text_dyn(char const *filename, MagDynArena *arena)
{
    size fsize = coy_file_size(filename);
    StopIf(fsize < 0, goto ERR_RETURN);

    byte *out = mag_dyn_arena_nmalloc(arena, fsize, byte);
    StopIf(!out, goto ERR_RETURN);

    size size_read = coy_file_slurp_internal(filename, fsize, out);
    StopIf(fsize != size_read, goto ERR_RETURN);

    return (ElkStr){ .start = out, .len = fsize };

ERR_RETURN:
    return (ElkStr){ .start = NULL, .len = 0 };
}

static inline ElkStr 
coy_file_slurp_text_allocator(char const *filename, MagAllocator *alloc)
{
    size fsize = coy_file_size(filename);
    StopIf(fsize < 0, goto ERR_RETURN);

    byte *out = mag_allocator_nmalloc(alloc, fsize, byte);
    StopIf(!out, goto ERR_RETURN);

    size size_read = coy_file_slurp_internal(filename, fsize, out);
    StopIf(fsize != size_read, goto ERR_RETURN);

    return (ElkStr){ .start = out, .len = fsize };

ERR_RETURN:
    return (ElkStr){ .start = NULL, .len = 0 };
}


static inline CoyFuture 
coy_future_create(CoyThreadFunc function, void *future_data)
{
    CoyFuture fut = { .state = COY_FUTURE_STATE_CREATED, .function = function, .future_data = future_data };
    return fut;
}

static inline CoyTaskState 
coy_future_get_task_state(CoyFuture *fut)
{
    return fut->state;
}

static inline b32 
coy_future_is_complete(CoyFuture *fut)
{
    return fut->state == COY_FUTURE_STATE_COMPLETE;
}

static inline void coy_future_mark_consumed(CoyFuture *fut)
{
    Assert(fut->state == COY_FUTURE_STATE_COMPLETE);
    fut->state = COY_FUTURE_STATE_CONSUMED;
}

static inline b32 
coy_future_is_consumed(CoyFuture *fut)
{
    return fut->state == COY_FUTURE_STATE_CONSUMED;
}

static inline void 
coy_thread_pool_executor_internal(void *input_channel)
{
    CoyChannel *tasks = input_channel;
    coy_channel_wait_until_ready_to_receive(tasks);

    void *void_task;
    while(coy_channel_receive(tasks, &void_task))
    {
        CoyFuture *fut = void_task;
        Assert(fut->state == COY_FUTURE_STATE_PENDING);
        fut->state = COY_FUTURE_STATE_RUNNING;
        _mm_mfence();
        fut->function(fut->future_data);
        _mm_mfence();
        fut->state = COY_FUTURE_STATE_COMPLETE;
    }

    coy_channel_done_receiving(tasks);
}

static inline CoyThreadPool 
coy_threadpool_create(size nthreads)
{
    Assert(nthreads <= COY_MAX_THREAD_POOL_SIZE);

    CoyThreadPool pool = { .nthreads = nthreads };
    pool.queue = coy_channel_create();
    coy_channel_register_sender(&pool.queue);

    for(size i = 0; i < nthreads; ++i)
    {
        coy_thread_create(&pool.threads[i], coy_thread_pool_executor_internal, &pool.queue);
        coy_channel_register_receiver(&pool.queue);
    }
    coy_channel_wait_until_ready_to_send(&pool.queue);

    return pool;
}

static inline void 
coy_threadpool_destroy(CoyThreadPool *pool)
{
    coy_channel_done_sending(&pool->queue);
    for(size i = 0; i < pool->nthreads; ++i)
    {
        coy_thread_join(&pool->threads[i]);
        coy_thread_destroy(&pool->threads[i]);
    }
    coy_channel_destroy(&pool->queue, NULL, NULL);
    memset(pool, 0, sizeof(*pool));
}

static inline void 
coy_threadpool_submit(CoyThreadPool *pool, CoyFuture *fut)
{
    fut->state = COY_FUTURE_STATE_PENDING;
    coy_channel_send(&pool->queue, fut);
}

#if defined(_WIN32) || defined(_WIN64)

#pragma warning(disable: 4142)
#include "coyote_win32.h"
#pragma warning(default: 4142)

#elif defined(__linux__)

#include "coyote_linux.h"

#elif defined(__APPLE__)

#include "coyote_apple_osx.h"

#else
#error "Platform not supported by Coyote Library"
#endif

#if defined(__linux__) || defined(__APPLE__)

#include "coyote_linux_apple_common.h"

#endif

#pragma warning(pop)

#endif
