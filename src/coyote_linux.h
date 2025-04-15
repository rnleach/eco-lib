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
