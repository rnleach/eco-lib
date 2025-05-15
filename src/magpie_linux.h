#ifndef _MAGPIE_LINUX_H_
#define _MAGPIE_LINUX_H_
/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Linux Implementation
 *---------------------------------------------------------------------------------------------------------------------------
 * Linux specific implementation goes here - things NOT in common with Apple / BSD
 */
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

static inline MagMemoryBlock 
mag_sys_memory_allocate(size minimum_num_bytes)
{
    Assert(minimum_num_bytes > 0);

    long page_size = sysconf(_SC_PAGESIZE);
    StopIf(page_size == -1, goto ERR_RETURN);
    usize nbytes = minimum_num_bytes + page_size - (minimum_num_bytes % page_size);

    void *ptr = mmap(NULL,                                    /* the starting address, NULL = don't care                */
                     nbytes,                                  /* the amount of memory to allocate                       */
                     PROT_READ | PROT_WRITE,                  /* we should have read and write access to the memory     */
                     MAP_PRIVATE | MAP_ANON | MAP_POPULATE,   /* not backed by a file, this is pure memory              */
                     -1,                                      /* recommended file descriptor for portability            */
                     0);                                      /* offset into what? this isn't a file, so should be zero */

    StopIf(ptr == MAP_FAILED, goto ERR_RETURN);

    return (MagMemoryBlock){ .mem = ptr, .size = nbytes, .flags = 0x01u | 0x02u };

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
        size_t sz = mem->size;
        memset(mem, 0, sizeof(*mem));
        /* int success = */ munmap(smem, sz);
    }

    return;
}

#endif
