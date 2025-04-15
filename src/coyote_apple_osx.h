#ifndef _COYOTE_APPLE_OSX_H_
#define _COYOTE_APPLE_OSX_H_
/*---------------------------------------------------------------------------------------------------------------------------
 *                                               Apple/MacOSX Implementation
 *-------------------------------------------------------------------------------------------------------------------------*/
// Apple / BSD specific implementation goes here - things NOT in common with Linux
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syslimits.h>
#include <unistd.h>

static inline void 
coy_profile_initialize_os_metrics(void)
{
    if(!coy_global_os_metrics.initialized)
    {
        coy_global_os_metrics.handle = 0;
        coy_global_os_metrics.initialized = true;
    }
}

static inline void 
coy_profile_finalize_os_metrics(void)
{
    /* no op on mac */
}

static inline u64
coy_profile_read_os_page_fault_count(void)
{
    struct rusage usage = {0};
    getrusage(RUSAGE_SELF, &usage);
    return (u64)(usage.ru_minflt + usage.ru_majflt);
}

#endif
