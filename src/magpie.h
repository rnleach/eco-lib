#ifndef _MAGPIE_H_
#define _MAGPIE_H_

#include <stdint.h>
#include <stddef.h>

#include "elk.h"

#pragma warning(push)

/*---------------------------------------------------------------------------------------------------------------------------
 * TODO: Things I'd like to add.
 *-------------------------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------------------------
 * Declare parts of the standard C library I use. These should almost always be implemented as compiler intrinsics anyway.
 *-------------------------------------------------------------------------------------------------------------------------*/

void *memset(void *buffer, int val, size_t num_bytes);
void *memcpy(void *dest, void const *src, size_t num_bytes);
void *memmove(void *dest, void const *src, size_t num_bytes);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                         Memory
 *---------------------------------------------------------------------------------------------------------------------------
 * Request big chunks of memory from the OS, bypassing the CRT. The system may round up your requested memory size, but it
 * will return an error instead of rounding down if there isn't enough memory.
 */
typedef struct
{
    void *mem;
    size size;
    b32 valid;
} MagMemoryBlock;

#define MAG_KB(a) ((a) * INT64_C(1000))
#define MAG_MB(a) (MAG_KB(a) * INT64_C(1000))
#define MAG_GB(a) (MAG_MB(a) * INT64_C(1000))
#define MAG_TB(a) (MAG_GB(a) * INT64_C(1000))

#define MAG_KiB(a) ((a) * (1024))
#define MAG_MiB(a) (MAG_KiB(a) * INT64_C(1024))
#define MAG_GiB(a) (MAG_MiB(a) * INT64_C(1024))
#define MAG_TiB(a) (MAG_GiB(a) * INT64_C(1024))

static inline MagMemoryBlock mag_sys_memory_allocate(size minimum_num_bytes);
static inline void mag_sys_memory_free(MagMemoryBlock *mem);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Static Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 * Allocate some memory from the heap and manage it with an ElkStaticArena.
 */
static inline ElkStaticArena mag_static_arena_allocate_and_create(size num_bytes);
static inline void mag_static_arena_destroy_and_deallocate(ElkStaticArena *arena);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Dynamic Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A dynamically growable arena allocator that works with the system allocator to grow indefinitely.
 */


/* A dynamically size arena. */
typedef struct MagDynArenaBlock MagDynArenaBlock;
typedef struct
{
    MagDynArenaBlock *head_block;
    size num_blocks;
    size num_total_bytes;
    size default_block_size;

    /* Keep track of the previous allocation for realloc. */
    MagDynArenaBlock *prev_block;
    void *prev_ptr;
    size prev_offset;
} MagDynArena;

static inline void mag_dyn_arena_create(MagDynArena *arena, size default_block_size);
static inline void mag_dyn_arena_destroy(MagDynArena *arena);
static inline void mag_dyn_arena_reset(MagDynArena *arena, b32 coalesce); /* Coalesce, keep the same size but in single block, else frees all excess blocks. */
static inline void *mag_dyn_arena_alloc(MagDynArena *arena, size num_bytes, size alignment);
static inline void *mag_dyn_arena_realloc(MagDynArena *arena, void *ptr, size num_bytes);
static inline void mag_dyn_arena_free(MagDynArena *arena, void *ptr);

#define mag_dyn_arena_malloc(arena, type) (type *)mag_dyn_arena_alloc((arena), sizeof(type), _Alignof(type))
#define mag_dyn_arena_nmalloc(arena, count, type) (type *)mag_dyn_arena_alloc((arena), (count) * sizeof(type), _Alignof(type))
#define mag_dyn_arena_nrealloc(arena, ptr, count, type) (type *) mag_dyn_arena_realloc((arena), (ptr), sizeof(type) * (count))

#define mag_dyn_arena_reset_default(arena) mag_dyn_arena_reset(arena, true)

#ifdef eco_arena_malloc

#undef eco_arena_malloc
#undef eco_arena_nmalloc
#undef eco_arena_nrealloc
#undef eco_arena_destroy
#undef eco_arena_free
#undef eco_arena_reset

#define eco_arena_malloc(arena, type) (type *) _Generic((arena),                                                            \
                                               ElkStaticArena *: elk_static_arena_alloc,                                    \
                                               MagDynArena *:    mag_dyn_arena_alloc                                        \
                                              )(arena, sizeof(type), _Alignof(type))

#define eco_arena_nmalloc(arena, count, type) (type *) _Generic((arena),                                                    \
                                                       ElkStaticArena *: elk_static_arena_alloc,                            \
                                                       MagDynArena *:    mag_dyn_arena_alloc                                \
                                                      )(arena, (count) * sizeof(type), _Alignof(type))

#define eco_arena_nrealloc(arena, ptr, count, type) _Generic((arena),                                                       \
                                                    ElkStaticArena *: elk_static_arena_realloc,                             \
                                                    MagDynArena *:    mag_dyn_arena_realloc                                 \
                                                  )(arena, ptr, sizeof(type) * (count))

#define eco_arena_destroy(arena) _Generic((arena),                                                                          \
                                 ElkStaticArena *: elk_static_arena_destroy,                                                \
                                 MagDynArena *:    mag_dyn_arena_destroy                                                    \
                                )(arena)

#define eco_arena_free(arena, ptr) _Generic((arena),                                                                        \
                                 ElkStaticArena *: elk_static_arena_free,                                                   \
                                 MagDynArena *:    mag_dyn_arena_free                                                       \
                                )(arena, ptr)

#define eco_arena_reset(arena) _Generic((arena),                                                                            \
                            ElkStaticArena *: elk_static_arena_reset,                                                       \
                            MagDynArena *:    mag_dyn_arena_reset_default                                                   \
                           )(arena)
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      String Slice
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Add some allocator enabled functionality to ElkStr.
 */


static inline ElkStr mag_str_append_dyn(ElkStr dest, ElkStr src, MagDynArena *arena); 
static inline ElkStr mag_str_alloc_copy_dyn(ElkStr src, MagDynArena *arena);

#ifdef eco_str_append

#undef eco_str_append
#undef eco_str_alloc_copy

#define eco_str_append(dest, src, arena) _Generic((arena),                                                                  \
                                         ElkStaticArena *: elk_str_append_static,                                           \
                                         MagDynArena *:    mag_str_append_dyn                                               \
                                         )(dest, src, arena)

#define eco_str_alloc_copy(src, arena) _Generic((arena),                                                              \
                                         ElkStaticArena *: elk_str_alloc_copy_static,                                       \
                                         MagDynArena *:    mag_str_alloc_copy_dyn                                           \
                                         )(src, arena)
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

static inline ElkStaticArena 
mag_static_arena_allocate_and_create(size num_bytes)
{
    ElkStaticArena arena = {0};
    MagMemoryBlock mem = mag_sys_memory_allocate(num_bytes);

    if(mem.valid)
    {
        elk_static_arena_create(&arena, mem.size, mem.mem);
    }

    return arena;
}

static inline void 
mag_static_arena_destroy_and_deallocate(ElkStaticArena *arena)
{
    MagMemoryBlock mem = { .mem = arena->buffer, .size = arena->buf_size, .valid = true };
    mag_sys_memory_free(&mem);
    arena->buffer = NULL;
    arena->buf_size = 0;
    return;
}

struct MagDynArenaBlock
{
    size buf_size;
    size buf_offset;
    byte *buffer;
    struct MagDynArenaBlock *next;
};

static inline MagDynArenaBlock *
mag_dyn_arena_block_create(size block_size)
{
    MagMemoryBlock mem = mag_sys_memory_allocate(block_size + sizeof(MagDynArenaBlock));
    if(mem.valid)
    {
        /* Place the block metadata at the beginning of the memory block. */
        MagDynArenaBlock *block = mem.mem;
        block->buf_size = mem.size;
        block->buf_offset = sizeof(MagDynArenaBlock);
        block->buffer = mem.mem;
        block->next = NULL;
        return block;
    }
    else
    {
        return NULL;
    }
}

static inline void *
mag_dyn_arena_block_alloc(MagDynArenaBlock *block, size num_bytes, size alignment, void **prev_ptr, size *prev_offset)
{
    Assert(num_bytes > 0 && alignment > 0);

    // Align 'curr_offset' forward to the specified alignment
    uptr curr_ptr = (uptr)block->buffer + (uptr)block->buf_offset;
    uptr offset = elk_align_pointer(curr_ptr, alignment);
    offset -= (uptr)block->buffer; // change to relative offset

    // Check to see if there is enough space left
    if ((size)(offset + num_bytes) <= block->buf_size)
    {
        void *ptr = &block->buffer[offset];
        memset(ptr, 0, num_bytes);
        *prev_offset = block->buf_offset;
        block->buf_offset = offset + num_bytes;
        *prev_ptr = ptr;

        return ptr;
    }
    else { return NULL; }
}

static inline void 
mag_dyn_arena_create(MagDynArena *arena, size default_block_size)
{
    MagDynArenaBlock *block = mag_dyn_arena_block_create(default_block_size);
    if(!block)
    {
        *arena = (MagDynArena){0};
        return;
    }

    arena->head_block = block;
    arena->num_blocks = 1;
    arena->num_total_bytes = block->buf_size;
    arena->default_block_size = default_block_size;

    arena->prev_block = block;
    arena->prev_offset = 0;
    arena->prev_ptr = block;

    return;
}

static inline void mag_dyn_arena_destroy(MagDynArena *arena)
{
    /* Iterate through linked list and free all the blocks. */
    MagDynArenaBlock *curr = arena->head_block;
    while(curr)
    {
        MagDynArenaBlock *next = curr->next;
        MagMemoryBlock mem = { .mem = curr, .size = curr->buf_size, .valid = true} ;
        mag_sys_memory_free(&mem);
        curr = next;
    }

    *arena = (MagDynArena){0};
    return;
}

static inline void 
mag_dyn_arena_reset(MagDynArena *arena, b32 coalesce)
{
    /* Only actually do the coalesce if we need to. */
    b32 do_coalesce = coalesce && arena->num_total_bytes > arena->default_block_size;

    MagDynArenaBlock *curr = NULL;
    if(do_coalesce)
    {
        /* Queue up all of the blocks to be deleted. */
        curr = arena->head_block;
    }
    else
    {
        /* Reset the first block. */
        arena->head_block->buf_offset = sizeof(MagDynArenaBlock); /* Not 0! Don't overwrite the block metadata. */

        arena->prev_block = arena->head_block;
        arena->prev_offset = 0;
        arena->prev_ptr = arena->head_block;

        /* Queue up the rest of the blocks to be deleted. */
        curr = arena->head_block->next;
    }

    /* Free the unneeded blocks. */
    while(curr)
    {
        MagDynArenaBlock *next = curr->next;
        MagMemoryBlock mem = { .mem = curr, .size = curr->buf_size, .valid = true} ;
        mag_sys_memory_free(&mem);
        curr = next;
    }

    /* Create a new head block if necesssary. */
    if(do_coalesce)
    {
        /* Create a block large enough to hold ALL the data from last time in a single block. */
        MagDynArenaBlock *block = mag_dyn_arena_block_create(arena->num_total_bytes);
        if(!block)
        {
            /* Not sure how this is possible because we JUST freed up this much memory! */
            *arena = (MagDynArena){0};
            return;
        }

        arena->head_block = block;
        arena->num_blocks = 1;
        arena->num_total_bytes = block->buf_size;
        /* arena->default_block_size = ...doesn't need changed; */

        arena->prev_block = block;
        arena->prev_offset = 0;
        arena->prev_ptr = block;
    }

    return;
}

static inline MagDynArenaBlock *
mag_dyn_arena_add_block(MagDynArena *arena, size min_bytes)
{
    MagDynArenaBlock *block = mag_dyn_arena_block_create(min_bytes);

    if(block)
    {
        MagDynArenaBlock *curr = arena->prev_block;
        MagDynArenaBlock *next = curr->next;
        while(next)
        {
            curr = next;
            next = curr->next;
        }

        curr->next = block;

        arena->num_blocks += 1;
        arena->num_total_bytes += block->buf_size;
    }

    return block;
}

static inline void *
mag_dyn_arena_alloc(MagDynArena *arena, size num_bytes, size alignment)
{
    Assert(num_bytes > 0 && alignment > 0);

    /* Find a block to use - start with the last block used. */
    MagDynArenaBlock *block = arena->prev_block;
    void *ptr = mag_dyn_arena_block_alloc(block, num_bytes, alignment, &arena->prev_ptr, &arena->prev_offset);
    if(ptr) { return ptr; }

    /* If that didn't work, look for space in any other block. */
    block = arena->head_block;
    while(block)
    {
        MagDynArenaBlock *next = block->next;
        ptr = mag_dyn_arena_block_alloc(block, num_bytes, alignment, &arena->prev_ptr, &arena->prev_offset);

        if(ptr)
        {
            /* Found one! */
            arena->prev_block = block;
            return ptr;
        }

        block = next;
    }

    /* We need to add another block. */
    size block_size = num_bytes + alignment > arena->default_block_size ? num_bytes + alignment : arena->default_block_size;
    block = mag_dyn_arena_add_block(arena, block_size);
    if(block)
    {
        ptr = mag_dyn_arena_block_alloc(block, num_bytes, alignment, &arena->prev_ptr, &arena->prev_offset);
        if(ptr)
        {
            arena->prev_block = block;
            return ptr;
        }
    }

    /* Give up. */
    return NULL;
}

static inline void *
mag_dyn_arena_realloc(MagDynArena *arena, void *ptr, size num_bytes)
{
    Assert(num_bytes > 0);

    if(ptr == arena->prev_ptr)
    {
        // Get previous extra offset due to alignment
        uptr offset = (uptr)ptr - (uptr)arena->prev_block->buffer; // relative offset accounting for alignment

        // Check to see if there is enough space left
        if ((size)(offset + num_bytes) <= arena->prev_block->buf_size)
        {
            arena->prev_block->buf_offset = offset + num_bytes;
            return ptr;
        }
    }

    return NULL;
}

static inline void 
mag_dyn_arena_free(MagDynArena *arena, void *ptr)
{
    if(ptr == arena->prev_ptr)
    {
        arena->prev_block->buf_offset = arena->prev_offset;
    }

    return;
}

static inline ElkStr 
mag_str_append_dyn(ElkStr dest, ElkStr src, MagDynArena *arena)
{
    ElkStr result = {0};

    if(src.len <= 0) { return result; }

    size new_len = dest.len + src.len;
    char *buf = dest.start;
    buf = mag_dyn_arena_nrealloc(arena, buf, new_len, char);

    if(!buf)
    { 
        /* Failed to grow in place. */
        buf = mag_dyn_arena_nmalloc(arena, new_len, char);
        if(buf)
        {
            char *dest_ptr = buf;
            memcpy(dest_ptr, dest.start, dest.len);
            dest_ptr = dest_ptr + dest.len;
            memcpy(dest_ptr, src.start, src.len);

            result.start = buf;
            result.len = new_len;
        }

        /* else leave result as empty - a null string. */
    }
    else
    {
        /* Grew in place! */
        result.start = buf;
        result.len = new_len;
        char *dest_ptr = dest.start + dest.len;
        memcpy(dest_ptr, src.start, src.len);
    }

    return result;
}

static inline ElkStr 
mag_str_alloc_copy_dyn(ElkStr src, MagDynArena *arena)
{
    ElkStr ret_val = {0};

    size copy_len = src.len + 1; // Add room for terminating zero.
    char *buffer = mag_dyn_arena_nmalloc(arena, copy_len, char);
    StopIf(!buffer, return ret_val); // Return NULL string if out of memory

    ret_val = elk_str_copy(copy_len, buffer, src);

    return ret_val;
}

#if defined(_WIN32) || defined(_WIN64)

#pragma warning(disable: 4142)
#include "magpie_win32.h"
#pragma warning(default: 4142)

#elif defined(__linux__)

#include "magpie_linux.h"

#elif defined(__APPLE__)

#include "magpie_apple_osx.h"

#else
#error "Platform not supported by Magpie Library"
#endif

#pragma warning(pop)

#endif
