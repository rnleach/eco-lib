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
    Assert(minimum_num_bytes > 0);
    StopIf(minimum_num_bytes <= 0, goto ERR_RETURN);

    SYSTEM_INFO info = {0};
    GetSystemInfo(&info);
    uptr page_size = info.dwPageSize;
    uptr alloc_gran = info.dwAllocationGranularity;

    uptr target_granularity = (uptr)minimum_num_bytes > alloc_gran ? alloc_gran : page_size;

    uptr allocation_size = minimum_num_bytes + target_granularity - (minimum_num_bytes % target_granularity);

    Assert(allocation_size <= INTPTR_MAX);
    StopIf(allocation_size > INTPTR_MAX, goto ERR_RETURN);

    size a_size = (size)allocation_size;

    void *mem = VirtualAlloc(NULL, allocation_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    Assert(mem);
    StopIf(!mem, goto ERR_RETURN);

    return (MagMemoryBlock){.mem = mem, .size = a_size, .flags = 0x01u | 0x02u };

ERR_RETURN:
    return (MagMemoryBlock) { 0 };
}

static inline void
mag_sys_memory_free(MagMemoryBlock *mem)
{
    if(MAG_MEM_IS_VALID_AND_OWNED(*mem))
    {
        /* Copy them out in case the MagMemoryBlock is stored in memory it allocated! It happens. */
        void *smem = mem->mem;
         memset(mem, 0, sizeof(*mem));
         /*BOOL success =*/ VirtualFree(smem, 0, MEM_RELEASE);
    }

    return;
}

#endif
