#ifndef _COYOTE_LINUX_H_
#define _COYOTE_LINUX_H_
/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Linux Implementation
 *-------------------------------------------------------------------------------------------------------------------------*/
// Linux specific implementation goes here - things NOT in common with Apple / BSD
#include <fcntl.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>

/* The reason for a seperate Linux and Apple impelementation is on Linux I can use the MAP_POPULATE flag, but I cannot on 
 * Apple. This flag pre-faults all the pages, so you don't have to worry about page faults slowing down the program.
 */

static inline CoyMemoryBlock 
coy_memory_allocate(size minimum_num_bytes)
{
    Assert(minimum_num_bytes > 0);

    long page_size = sysconf(_SC_PAGESIZE);
    StopIf(page_size == -1, goto ERR_RETURN);
    usize nbytes = minimum_num_bytes + page_size - (minimum_num_bytes % page_size);

    void *ptr = mmap(NULL,                                    // the starting address, NULL = don't care
                     nbytes,                                  // the amount of memory to allocate
                     PROT_READ | PROT_WRITE,                  // we should have read and write access to the memory
                     MAP_PRIVATE | MAP_ANON | MAP_POPULATE,   // not backed by a file, this is pure memory
                     -1,                                      // recommended file descriptor for portability
                     0);                                      // offset into what? this isn't a file, so should be zero

    StopIf(ptr == MAP_FAILED, goto ERR_RETURN);

    return (CoyMemoryBlock){ .mem = ptr, .size = nbytes, .valid = true };

ERR_RETURN:
    return (CoyMemoryBlock){ .mem = NULL, .size = 0, .valid = false};
}

static inline void
coy_memory_free(CoyMemoryBlock *mem)
{
    /* int success = */ munmap(mem->mem, (size_t)mem->size);
    mem->valid = false;
    return;
}

static inline void 
coy_profile_initialize_os_metrics(void)
{
    if(!coy_global_os_metrics.initialized)
    {
        int fd = -1;
        struct perf_event_attr pe = {0};
        pe.type = PERF_TYPE_SOFTWARE;
        pe.size = sizeof(pe);
        pe.config = PERF_COUNT_SW_PAGE_FAULTS;
        pe.disabled = 1;
        pe.exclude_kernel = 0; /* Turns out most page faults happen here */

        fd = syscall(SYS_perf_event_open, &pe, 0, -1, -1, 0);
        if (fd == -1) {
            /* Might need to run with sudo, crash! */
            (*(int volatile*)0);
        }

        ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);

        coy_global_os_metrics.handle = (uptr)fd;
        coy_global_os_metrics.initialized = true;
    }
}

static inline void 
coy_profile_finalize_os_metrics(void)
{
    close((int)coy_global_os_metrics.handle);
}

static inline u64
coy_profile_read_os_page_fault_count(void)
{
    u64 count = (u64)-1;
    ioctl((int)coy_global_os_metrics.handle, PERF_EVENT_IOC_DISABLE, 0);
    int size_read = read((int)coy_global_os_metrics.handle, &count, sizeof(count));
    if(size_read != sizeof(count))
    {
        count = (u64)-1; 
    }
    ioctl((int)coy_global_os_metrics.handle, PERF_EVENT_IOC_ENABLE, 0);
    return count;
}

#endif
