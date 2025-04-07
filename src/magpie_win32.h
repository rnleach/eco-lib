#ifndef _MAGPIE_WIN32_H_
#define _MAGPIE_WIN32_H_

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Windows Implementation
 *-------------------------------------------------------------------------------------------------------------------------*/
#include <windows.h>

_Static_assert(UINT32_MAX < INTPTR_MAX, "DWORD cannot be cast to intptr_t safely.");

static inline MagMemoryBlock 
mag_sys_memory_allocate(size minimum_num_bytes)
{
    if(minimum_num_bytes <= 0)
    {
         return (MagMemoryBlock){.mem = 0, .size = minimum_num_bytes, .valid = false };
    }

    SYSTEM_INFO info = {0};
    GetSystemInfo(&info);
    uptr page_size = info.dwPageSize;
    uptr alloc_gran = info.dwAllocationGranularity;

    uptr target_granularity = (uptr)minimum_num_bytes > alloc_gran ? alloc_gran : page_size;

    uptr allocation_size = minimum_num_bytes;
    if(minimum_num_bytes % target_granularity)
    {
        allocation_size += target_granularity - (minimum_num_bytes % target_granularity);
    }

    if(allocation_size > INTPTR_MAX)
    {
         return (MagMemoryBlock){.mem = 0, .size = INTPTR_MAX, .valid = false };
    }
    size a_size = (size)allocation_size;

    void *mem = VirtualAlloc(NULL, allocation_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!mem)
    {
         return (MagMemoryBlock){.mem = 0, .size = 0, .valid = false };
    }

    return (MagMemoryBlock){.mem = mem, .size = a_size, .valid = true };
}

static inline void
mag_sys_memory_free(MagMemoryBlock *mem)
{
    if(mem->valid)
    {
         /*BOOL success =*/ VirtualFree(mem->mem, 0, MEM_RELEASE);
         memset(mem, 0, sizeof(*mem));
    }

    return;
}

#endif
