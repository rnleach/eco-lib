#ifndef _MAGPIE_LINUX_H_
#define _MAGPIE_LINUX_H_
/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Linux Implementation
 *---------------------------------------------------------------------------------------------------------------------------
 * Linux specific implementation goes here - things NOT in common with Apple / BSD
 */
#include <stdlib.h>

static inline MagMemoryBlock 
mag_sys_memory_allocate(size minimum_num_bytes)
{
    Assert(minimum_num_bytes > 0);

    void *ptr = malloc(minimum_num_bytes);

    StopIf(ptr == NULL, goto ERR_RETURN);

    return (MagMemoryBlock){ .mem = ptr, .size = minimum_num_bytes, .flags = 0x01u | 0x02u };

ERR_RETURN:
    return (MagMemoryBlock){ 0 };
}

static inline void
mag_sys_memory_free(MagMemoryBlock *mem)
{
    if(MAG_MEM_IS_VALID_AND_OWNED(*mem))
    {
        /* Copy them out in case the MagMemoryBlock is stored in memory it allocated! It happens. */
        void *smem = mem->mem;
        free(smem);
    }

    return;
}

#endif
