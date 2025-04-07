#ifndef _MAGPIE_APPLE_OSX_H_
#define _MAGPIE_APPLE_OSX_H_
/*---------------------------------------------------------------------------------------------------------------------------
 *                                               Apple/MacOSX Implementation
 *-------------------------------------------------------------------------------------------------------------------------*/
// Apple / BSD specific implementation goes here - things NOT in common with Linux
#include <sys/mman.h>
#include <sys/resource.h>
#include <unistd.h>

/* The reason for a seperate Linux and Apple impelementation is on Linux I can use the MAP_POPULATE flag, but I cannot on 
 * Apple. This flag pre-faults all the pages, so you don't have to worry about page faults slowing down the program. Apple
 * gives far LESS control over allocation.
 */

static inline MagMemoryBlock 
mag_sys_memory_allocate(size minimum_num_bytes)
{
    Assert(minimum_num_bytes > 0);

    long page_size = sysconf(_SC_PAGESIZE);
    StopIf(page_size == -1, goto ERR_RETURN);
    usize nbytes = minimum_num_bytes + page_size - (minimum_num_bytes % page_size);

    void *ptr = mmap(NULL,                     // the starting address, NULL = don't care
                     nbytes,                   // the amount of memory to allocate
                     PROT_READ | PROT_WRITE,   // we should have read and write access to the memory
                     MAP_PRIVATE | MAP_ANON,   // not backed by a file, this is pure memory
                     -1,                       // recommended file descriptor for portability
                     0);                       // offset into what? this isn't a file, so should be zero

    StopIf(ptr == MAP_FAILED, goto ERR_RETURN);

    return (MagMemoryBlock){ .mem = ptr, .size = nbytes, .valid = true };

ERR_RETURN:
    return (MagMemoryBlock){ .mem = NULL, .size = 0, .valid = false};
}

static inline void
mag_sys_memory_free(MagMemoryBlock *mem)
{
    /* int success = */ munmap(mem->mem, (size_t)mem->size);
    memset(mem, 0, sizeof(*mem));
    return;
}

#endif
