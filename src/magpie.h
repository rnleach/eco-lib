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
 *                                                 Static Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Compile with -D_MAG_TRACK_MEM_USAGE to include the maximum memory tracking variables & features.
 *
 * A statically sized, non-growable arena allocator that works on top of a user supplied buffer.
 */

#ifdef _MAG_TRACK_MEM_USAGE
typedef struct
{
    size max_offset;
    b32 over_allocation_attempted;
} MagStaticArenaAllocationMetrics;
#endif

typedef struct 
{
    size buf_size;
    size buf_offset;
    byte *buffer;
    void *prev_ptr;
    size prev_offset;
#ifdef _MAG_TRACK_MEM_USAGE
    MagStaticArenaAllocationMetrics *metrics_ptr;
#endif
} MagStaticArena;

static inline MagStaticArena mag_static_arena_create(size buf_size, byte buffer[]);
static inline void mag_static_arena_destroy(MagStaticArena *arena);
static inline void mag_static_arena_reset(MagStaticArena *arena);                                  // Set offset to 0, invalidates all previous allocations
static inline void *mag_static_arena_alloc(MagStaticArena *arena, size num_bytes, size alignment); // ret NULL if OOM
static inline void *mag_static_arena_realloc(MagStaticArena *arena, void *ptr, size asize);        // ret NULL if ptr is not most recent allocation
static inline void mag_static_arena_free(MagStaticArena *arena, void *ptr);                        // Undo if it was last allocation, otherwise no-op

/* Also see eco_arena_malloc and related macros below. */
#define mag_static_arena_malloc(arena, type) (type *)mag_static_arena_alloc((arena), sizeof(type), _Alignof(type))
#define mag_static_arena_nmalloc(arena, count, type) (type *)mag_static_arena_alloc((arena), (count) * sizeof(type), _Alignof(type))
#define mag_static_arena_nrealloc(arena, ptr, count, type) (type *) mag_static_arena_realloc((arena), (ptr), sizeof(type) * (count))

#ifdef _MAG_TRACK_MEM_USAGE
static MagStaticArenaAllocationMetrics mag_static_arena_metrics[128] = {0};
static size mag_static_arena_metrics_next = 0;
static inline f64 mag_static_arena_max_ratio(MagStaticArena *arena);
static inline b32 mag_static_arena_over_allocated(MagStaticArena *arena);
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Static Pool Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * This is an error prone and brittle type. If you get it all working, a refactor or code edit later is likely to break it.
 *
 * WARNING: It is the user's responsibility to make sure that there is at least object_size * num_objects bytes in the
 * backing buffer. If that isn't true, you'll probably get a seg-fault during initialization.
 *
 * WARNING: Object_size must be a multiple of sizeof(void*) to ensure the buffer is aligned to hold pointers also. That also
 * means object_size must be at least sizeof(void*).
 *
 * WARNING: It is the user's responsibility to make sure the buffer is correctly aligned for the type of objects they will
 * be storing in it. This isn't a concern if the memory came from malloc() et al as they return memory with the most
 * pessimistic alignment. However, if using a stack allocated or static memory section, you should use an _Alignas to force
 * the alignment.
 */
typedef struct 
{
    size object_size;    // The size of each object
    size num_objects;    // The capacity, or number of objects storable in the pool
    void *free;          // The head of a free list of available slots for objects
    byte *buffer;        // The buffer we actually store the data in
} MagStaticPool;

static inline MagStaticPool mag_static_pool_create(size object_size, size num_objects, byte buffer[]);
static inline void mag_static_pool_destroy(MagStaticPool *pool);
static inline void mag_static_pool_reset(MagStaticPool *pool);
static inline void mag_static_pool_free(MagStaticPool *pool, void *ptr);
static inline void * mag_static_pool_alloc(MagStaticPool *pool); // returns NULL if there's no more space available.
// no mag_static_pool_realloc because that doesn't make sense!

#define mag_static_pool_malloc(alloc, type) (type *)mag_static_pool_alloc(alloc)

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

static inline MagMemoryBlock mag_sys_memory_allocate(size minimum_num_bytes);
static inline void mag_sys_memory_free(MagMemoryBlock *mem);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Static Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 * Allocate some memory from the heap and manage it with an MagStaticArena.
 */
static inline MagStaticArena mag_static_arena_allocate_and_create(size num_bytes);
static inline void mag_static_arena_destroy_and_deallocate(MagStaticArena *arena);

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

static inline MagDynArena mag_dyn_arena_create(size default_block_size);
static inline void mag_dyn_arena_destroy(MagDynArena *arena);
static inline void mag_dyn_arena_reset(MagDynArena *arena, b32 coalesce); /* Coalesce, keep the same size but in single block, else frees all excess blocks. */
static inline void mag_dyn_arena_reset_default(MagDynArena *arena);
static inline void *mag_dyn_arena_alloc(MagDynArena *arena, size num_bytes, size alignment);
static inline void *mag_dyn_arena_realloc(MagDynArena *arena, void *ptr, size num_bytes);
static inline void mag_dyn_arena_free(MagDynArena *arena, void *ptr);

#define mag_dyn_arena_malloc(arena, type) (type *)mag_dyn_arena_alloc((arena), sizeof(type), _Alignof(type))
#define mag_dyn_arena_nmalloc(arena, count, type) (type *)mag_dyn_arena_alloc((arena), (count) * sizeof(type), _Alignof(type))
#define mag_dyn_arena_nrealloc(arena, ptr, count, type) (type *) mag_dyn_arena_realloc((arena), (ptr), sizeof(type) * (count))

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Generalized Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A generalized interface to allocators.
 */

typedef enum { MAG_ALLOC_T_STATIC_ARENA, MAG_ALLOC_T_DYN_ARENA } MagAllocatorType;

typedef struct
{
    MagAllocatorType type;
    union
    {
        MagStaticArena static_arena;
        MagDynArena dyn_arena;
    };
} MagAllocator;


static inline MagAllocator mag_allocator_dyn_arena_create(size default_block_size);
static inline MagAllocator mag_allocator_static_arena_create(size buf_size, byte buffer[]);
static inline MagAllocator mag_allocator_from_dyn_arena(MagDynArena arena);
static inline MagAllocator mag_allocator_from_static_arena(MagStaticArena arena);

static inline void mag_allocator_destroy(MagAllocator *arena);
static inline void mag_allocator_reset(MagAllocator *arena); /* Clear all previous allocations. */
static inline void *mag_allocator_alloc(MagAllocator *alloc, size num_bytes, size alignment);
static inline void *mag_allocator_realloc(MagAllocator *alloc, void *ptr, size num_bytes);
static inline void mag_allocator_free(MagAllocator *alloc, void *ptr);

#define mag_allocator_malloc(arena, type) (type *)mag_allocator_alloc((arena), sizeof(type), _Alignof(type))
#define mag_allocator_nmalloc(arena, count, type) (type *)mag_allocator_alloc((arena), (count) * sizeof(type), _Alignof(type))
#define mag_allocator_nrealloc(arena, ptr, count, type) (type *) mag_allocator_realloc((arena), (ptr), sizeof(type) * (count))

#define eco_malloc(alloc, type) (type *)         _Generic((alloc),                                                          \
                                                     MagStaticArena *: mag_static_arena_alloc,                              \
                                                     MagDynArena *:    mag_dyn_arena_alloc,                                 \
                                                     MagAllocator *:   mag_allocator_alloc                                  \
                                                 )(alloc, sizeof(type), _Alignof(type))

#define eco_nmalloc(alloc, count, type) (type *) _Generic((alloc),                                                          \
                                                     MagStaticArena *: mag_static_arena_alloc,                              \
                                                     MagDynArena *:    mag_dyn_arena_alloc,                                 \
                                                     MagAllocator *:   mag_allocator_alloc                                  \
                                                 )(alloc, (count) * sizeof(type), _Alignof(type))

#define eco_nrealloc(alloc, ptr, count, type)    _Generic((alloc),                                                          \
                                                      MagStaticArena *: mag_static_arena_realloc,                           \
                                                      MagDynArena *:    mag_dyn_arena_realloc,                              \
                                                      MagAllocator *:   mag_allocator_realloc                               \
                                                 )(alloc, ptr, sizeof(type) * (count))

#define eco_destroy(alloc)                       _Generic((alloc),                                                          \
                                                     MagStaticArena *: mag_static_arena_destroy,                            \
                                                     MagDynArena *:    mag_dyn_arena_destroy,                               \
                                                     MagAllocator *:   mag_allocator_destroy                                \
                                                 )(alloc)

#define eco_free(alloc, ptr)                     _Generic((alloc),                                                          \
                                                     MagStaticArena *: mag_static_arena_free,                               \
                                                     MagDynArena *:    mag_dyn_arena_free,                                  \
                                                     MagAllocator *:   mag_allocator_free                                   \
                                                 )(alloc, ptr)

#define eco_reset(alloc)                         _Generic((alloc),                                                          \
                                                     MagStaticArena *: mag_static_arena_reset,                              \
                                                     MagDynArena *:    mag_dyn_arena_reset_default,                         \
                                                     MagAllocator *:   mag_allocator_reset                                  \
                                                 )(alloc)

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                      String Slice
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * Add some allocator enabled functionality to ElkStr.
 */


static inline ElkStr mag_str_alloc_copy_static(ElkStr src, MagStaticArena *arena);          /* Allocate space and create a copy.                                                        */
static inline ElkStr mag_str_append_static(ElkStr dest, ElkStr src, MagStaticArena *arena); /* If dest wasn't the last thing allocated on arena, this fails and returns an empty string */

static inline ElkStr mag_str_append_dyn(ElkStr dest, ElkStr src, MagDynArena *arena); 
static inline ElkStr mag_str_alloc_copy_dyn(ElkStr src, MagDynArena *arena);

static inline ElkStr mag_str_append_alloc(ElkStr dest, ElkStr src, MagAllocator *alloc); 
static inline ElkStr mag_str_alloc_copy_alloc(ElkStr src, MagAllocator *alloc);

/* Convenience functions. These may may be extended with _Generic in Magpie. */
#define eco_str_append(dest, src, alloc) _Generic((alloc),                                                                  \
                                         MagStaticArena *: mag_str_append_static,                                           \
                                         MagDynArena *:    mag_str_append_dyn,                                              \
                                         MagAllocator *:   mag_str_append_alloc                                             \
                                         )(dest, src, alloc)

#define eco_str_alloc_copy(src, alloc) _Generic((alloc),                                                                    \
                                         MagStaticArena *: mag_str_alloc_copy_static,                                       \
                                         MagDynArena *:    mag_str_alloc_copy_dyn,                                          \
                                         MagAllocator *:   mag_str_alloc_copy_alloc                                         \
                                         )(src, alloc)

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
static inline b32
mag_is_power_of_2(uptr p)
{
    return (p & (p - 1)) == 0;
}

static inline uptr
mag_align_pointer(uptr ptr, usize align)
{

    Assert(mag_is_power_of_2(align));

    uptr a = (uptr)align;
    uptr mod = ptr & (a - 1); // Same as (ptr % a) but faster as 'a' is a power of 2

    if (mod != 0)
    {
        // push the address forward to the next value which is aligned
        ptr += a - mod;
    }

    return ptr;
}

static inline MagStaticArena
mag_static_arena_create(size buf_size, byte buffer[])
{
    Assert(buffer && buf_size > 0);

    MagStaticArena arena = { .buf_size = buf_size, .buf_offset = 0, .buffer = buffer, .prev_ptr = NULL, .prev_offset = 0, };

#ifdef _MAG_TRACK_MEM_USAGE
    size idx = mag_static_arena_metrics_next++;
    Assert(idx < (sizeof(mag_static_arena_metrics) / sizeof(mag_static_arena_metrics[0])));
    arena.metrics_ptr = &mag_static_arena_metrics[idx];
    *arena.metrics_ptr = (MagStaticArenaAllocationMetrics){0};
#endif

    return arena;
}

static inline void
mag_static_arena_destroy(MagStaticArena *arena)
{
    *arena = (MagStaticArena){0};
    return;
}

static inline void
mag_static_arena_reset(MagStaticArena *arena)
{
    Assert(arena->buffer);
    arena->buf_offset = 0;
    arena->prev_ptr = NULL;
    arena->prev_offset = 0;
    return;
}

static inline void *
mag_static_arena_alloc(MagStaticArena *arena, size num_bytes, size alignment)
{
    Assert(num_bytes > 0 && alignment > 0);

    // Align 'curr_offset' forward to the specified alignment
    uptr curr_ptr = (uptr)arena->buffer + (uptr)arena->buf_offset;
    uptr offset = mag_align_pointer(curr_ptr, alignment);
    offset -= (uptr)arena->buffer; // change to relative offset

    // Check to see if there is enough space left
    if ((size)(offset + num_bytes) <= arena->buf_size)
    {
        void *ptr = &arena->buffer[offset];
        memset(ptr, 0, num_bytes);
        arena->prev_offset = arena->buf_offset;
        arena->buf_offset = offset + num_bytes;
        arena->prev_ptr = ptr;

#ifdef _MAG_TRACK_MEM_USAGE
        arena->metrics_ptr->max_offset = arena->buf_offset > (arena->metrics_ptr->max_offset) ?
            arena->buf_offset : (arena->metrics_ptr->max_offset);
#endif

        return ptr;
    }
    else
    {
#ifdef _MAG_TRACK_MEM_USAGE
        arena->metrics_ptr->over_allocation_attempted = true;
#endif
        return NULL;
    }
}

static inline void * 
mag_static_arena_realloc(MagStaticArena *arena, void *ptr, size asize)
{
    Assert(asize > 0);

    if(ptr == arena->prev_ptr)
    {
        // Get previous extra offset due to alignment
        uptr offset = (uptr)ptr - (uptr)arena->buffer; // relative offset accounting for alignment

        // Check to see if there is enough space left
        if ((size)(offset + asize) <= arena->buf_size)
        {
            arena->buf_offset = offset + asize;

#ifdef _MAG_TRACK_MEM_USAGE
            arena->metrics_ptr->max_offset = arena->buf_offset > (arena->metrics_ptr->max_offset) ?
                arena->buf_offset : (arena->metrics_ptr->max_offset);
#endif

            return ptr;
        }
        else
        {
#ifdef _MAG_TRACK_MEM_USAGE
            arena->metrics_ptr->over_allocation_attempted = true;
#endif
        }
    }

    return NULL;
}

static inline void
mag_static_arena_free(MagStaticArena *arena, void *ptr)
{
    if(ptr == arena->prev_ptr)
    {
        arena->buf_offset = arena->prev_offset;
    }

    return;
}

#ifdef _MAG_TRACK_MEM_USAGE

static inline f64 
mag_static_arena_max_ratio(MagStaticArena *arena)
{
    return (f64)arena->metrics_ptr->max_offset / (f64)arena->buf_size;
}

static inline b32 
mag_static_arena_over_allocated(MagStaticArena *arena)
{
    return arena->metrics_ptr->over_allocation_attempted;
}

#endif

static inline void
mag_static_pool_initialize_linked_list(byte *buffer, size object_size, size num_objects)
{

    // Initialize the free list to a linked list.

    // start by pointing to last element and assigning it NULL
    size offset = object_size * (num_objects - 1);
    uptr *ptr = (uptr *)&buffer[offset];
    *ptr = (uptr)NULL;

    // Then work backwards to the front of the list.
    while (offset) 
    {
        size next_offset = offset;
        offset -= object_size;
        ptr = (uptr *)&buffer[offset];
        uptr next = (uptr)&buffer[next_offset];
        *ptr = next;
    }

    return;
}

static inline void
mag_static_pool_reset(MagStaticPool *pool)
{
    Assert(pool && pool->buffer && pool->num_objects && pool->object_size);

    // Initialize the free list to a linked list.
    mag_static_pool_initialize_linked_list(pool->buffer, pool->object_size, pool->num_objects);
    pool->free = &pool->buffer[0];
}

static inline MagStaticPool
mag_static_pool_create(size object_size, size num_objects, byte buffer[])
{
    Assert(object_size >= sizeof(void *));       // Need to be able to fit at least a pointer!
    Assert(object_size % _Alignof(void *) == 0); // Need for alignment of pointers.
    Assert(num_objects > 0);

    MagStaticPool pool = { .buffer = buffer, .object_size = object_size, .num_objects = num_objects };

    mag_static_pool_reset(&pool);

    return pool;
}

static inline void
mag_static_pool_destroy(MagStaticPool *pool)
{
    memset(pool, 0, sizeof(*pool));
}

static inline void
mag_static_pool_free(MagStaticPool *pool, void *ptr)
{
    uptr *next = ptr;
    *next = (uptr)pool->free;
    pool->free = ptr;
}

static inline void *
mag_static_pool_alloc(MagStaticPool *pool)
{
    void *ptr = pool->free;
    uptr *next = pool->free;

    if (ptr) 
    {
        pool->free = (void *)*next;
        memset(ptr, 0, pool->object_size);
    }

    return ptr;
}

static inline MagStaticArena 
mag_static_arena_allocate_and_create(size num_bytes)
{
    MagStaticArena arena = {0};
    MagMemoryBlock mem = mag_sys_memory_allocate(num_bytes);

    if(mem.valid)
    {
        arena = mag_static_arena_create(mem.size, mem.mem);
    }

    return arena;
}

static inline void 
mag_static_arena_destroy_and_deallocate(MagStaticArena *arena)
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
    uptr offset = mag_align_pointer(curr_ptr, alignment);
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

static inline MagDynArena 
mag_dyn_arena_create(size default_block_size)
{
    MagDynArena arena = {0};

    MagDynArenaBlock *block = mag_dyn_arena_block_create(default_block_size);
    if(!block)
    {
        return arena;
    }

    arena.head_block = block;
    arena.num_blocks = 1;
    arena.num_total_bytes = block->buf_size;
    arena.default_block_size = default_block_size;

    arena.prev_block = block;
    arena.prev_offset = 0;
    arena.prev_ptr = block;

    return arena;
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

static inline void 
mag_dyn_arena_reset_default(MagDynArena *arena)
{
    mag_dyn_arena_reset(arena, true);
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

static inline MagAllocator 
mag_allocator_dyn_arena_create(size default_block_size)
{
    MagAllocator alloc = { .type = MAG_ALLOC_T_DYN_ARENA };
    alloc.dyn_arena = mag_dyn_arena_create(default_block_size);
    return alloc;
}

static inline MagAllocator 
mag_allocator_static_arena_create(size buf_size, byte buffer[])
{
    MagAllocator alloc = { .type = MAG_ALLOC_T_STATIC_ARENA };
    alloc.static_arena = mag_static_arena_create(buf_size, buffer);
    return alloc;
}

static inline MagAllocator 
mag_allocator_from_dyn_arena(MagDynArena arena)
{
    MagAllocator alloc = { .type = MAG_ALLOC_T_DYN_ARENA, .dyn_arena = arena };
    return alloc;
}

static inline MagAllocator 
mag_allocator_from_static_arena(MagStaticArena arena)
{
    MagAllocator alloc = { .type = MAG_ALLOC_T_STATIC_ARENA, .static_arena = arena };
    return alloc;
}

static inline void 
mag_allocator_destroy(MagAllocator *alloc)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: mag_static_arena_destroy(&alloc->static_arena); break;
        case MAG_ALLOC_T_DYN_ARENA:    mag_dyn_arena_destroy(&alloc->dyn_arena);       break;
    }
}

static inline void 
mag_allocator_reset(MagAllocator *alloc)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: mag_static_arena_reset(&alloc->static_arena);   break;
        case MAG_ALLOC_T_DYN_ARENA:    mag_dyn_arena_reset_default(&alloc->dyn_arena); break;
    }
}

static inline void *
mag_allocator_alloc(MagAllocator *alloc, size num_bytes, size alignment)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: return mag_static_arena_alloc(&alloc->static_arena, num_bytes, alignment);
        case MAG_ALLOC_T_DYN_ARENA:    return mag_dyn_arena_alloc(&alloc->dyn_arena, num_bytes, alignment);
    }

    return NULL;
}

static inline void *
mag_allocator_realloc(MagAllocator *alloc, void *ptr, size num_bytes)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: return mag_static_arena_realloc(&alloc->static_arena, ptr, num_bytes);
        case MAG_ALLOC_T_DYN_ARENA:    return mag_dyn_arena_realloc(&alloc->dyn_arena, ptr, num_bytes);
    }

    return NULL;
}

static inline void 
mag_allocator_free(MagAllocator *alloc, void *ptr)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: mag_static_arena_free(&alloc->static_arena, ptr); break;
        case MAG_ALLOC_T_DYN_ARENA:    mag_dyn_arena_free(&alloc->dyn_arena, ptr);       break;
    }
}

static inline ElkStr 
mag_str_alloc_copy_static(ElkStr src, MagStaticArena *arena)
{
    ElkStr ret_val = {0};

    size copy_len = src.len + 1; // Add room for terminating zero.
    char *buffer = mag_static_arena_nmalloc(arena, copy_len, char);
    StopIf(!buffer, return ret_val); // Return NULL string if out of memory

    ret_val = elk_str_copy(copy_len, buffer, src);

    return ret_val;
}

static inline ElkStr 
mag_str_append_static(ElkStr dest, ElkStr src, MagStaticArena *arena)
{
    ElkStr result = {0};

    if(src.len <= 0) { return result; }

    size new_len = dest.len + src.len;
    char *buf = dest.start;
    buf = mag_static_arena_nrealloc(arena, buf, new_len, char);

    if(!buf) { return result; }

    result.start = buf;
    result.len = new_len;
    char *dest_ptr = dest.start + dest.len;
    memcpy(dest_ptr, src.start, src.len);

    return result;
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

static inline ElkStr 
mag_str_append_alloc(ElkStr dest, ElkStr src, MagAllocator *alloc)
{
    ElkStr result = {0};

    if(src.len <= 0) { return result; }

    size new_len = dest.len + src.len;
    char *buf = dest.start;
    buf = mag_allocator_nrealloc(alloc, buf, new_len, char);

    if(!buf)
    { 
        /* Failed to grow in place. */
        buf = mag_allocator_nmalloc(alloc, new_len, char);
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
mag_str_alloc_copy_alloc(ElkStr src, MagAllocator *alloc)
{
    ElkStr ret_val = {0};

    size copy_len = src.len + 1; // Add room for terminating zero.
    char *buffer = mag_allocator_nmalloc(alloc, copy_len, char);
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
