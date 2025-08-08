#ifndef _MAGPIE_H_
#define _MAGPIE_H_

#include <stdint.h>
#include <stddef.h>

#include "elk.h"

#pragma warning(push)

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                         Memory
 *---------------------------------------------------------------------------------------------------------------------------
 * Request big chunks of memory from the OS, bypassing the CRT. The system may round up your requested memory size, but it
 * will return an error instead of rounding down if there isn't enough memory.
 */
typedef struct
{
    byte *mem;
    size size;
    u8 flags; /* bit field, 0x1 = valid, 0x2 = is owned */
} MagMemoryBlock;

static inline MagMemoryBlock mag_sys_memory_allocate(size minimum_num_bytes);
static inline void mag_sys_memory_free(MagMemoryBlock *mem);
static inline MagMemoryBlock mag_wrap_memory(size buf_size, void *buffer);

#define MAG_MEM_IS_VALID(mem_block) (((mem_block).flags & 0x01u) > 0)
#define MAG_MEM_IS_OWNED(mem_block) (((mem_block).flags & 0x02u) > 0)
#define MAG_MEM_IS_VALID_AND_OWNED(mem_block) (((mem_block).flags & (0x01u | 0x02u)) == 0x03u)

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Static Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A statically sized, non-growable arena allocator that works on top of a user supplied buffer.
 *
 * Always use a pointer to the arena to collect the correct usage statistics. Utilize a save-point if you want to pass it
 * in to a temporary function and then restore to the save point when you get it back.
 */

typedef struct 
{
    MagMemoryBlock buf;
    size buf_offset;

    /* Keep track of the previous allocation for realloc and free. */
    void *prev_ptr;
    size prev_offset;

    size max_offset;
    b32 failed_allocation;
} MagStaticArena;

typedef struct
{
    MagStaticArena *origin;
    size buf_offset;
    void *prev_ptr;
    size prev_offset;
} MagStaticArenaSavePoint;

static inline MagStaticArena mag_static_arena_create(size buf_size, byte buffer[]);
static inline MagStaticArena mag_static_arena_allocate_and_create(size num_bytes);
static inline void mag_static_arena_destroy(MagStaticArena *arena);
static inline void mag_static_arena_reset(MagStaticArena *arena);                                           /* Set offset to 0, invalidates all previous allocations */
static inline void *mag_static_arena_alloc(MagStaticArena *arena, size num_bytes, size alignment);          /* ret NULL if out of memory (OOM)                       */
static inline void *mag_static_arena_realloc(MagStaticArena *arena, void *ptr, size asize, size alignment); /* ret NULL if ptr is not most recent allocation         */
static inline void mag_static_arena_free(MagStaticArena *arena, void *ptr);                                 /* Undo if it was last allocation, otherwise no-op       */

static inline MagStaticArenaSavePoint mag_static_arena_create_save_point(MagStaticArena *arena);
static inline void mag_static_arena_restore_save_point(MagStaticArenaSavePoint *save_point);

/* Also see eco_arena_malloc and related macros below. */
#define mag_static_arena_malloc(arena, type) (type *)mag_static_arena_alloc((arena), sizeof(type), _Alignof(type))
#define mag_static_arena_nmalloc(arena, count, type) (type *)mag_static_arena_alloc((arena), (count) * sizeof(type), _Alignof(type))
#define mag_static_arena_nrealloc(arena, ptr, count, type) (type *) mag_static_arena_realloc((arena), (ptr), sizeof(type) * (count), _Alignof(type))

static inline f64 mag_static_arena_max_ratio(MagStaticArena *arena);
static inline b32 mag_static_arena_over_allocated(MagStaticArena *arena);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                Dynamic Arena Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * A dynamically growable arena allocator that works with the system allocator to grow indefinitely.
 *
 * Always use a pointer to the arena to collect the correct usage statistics. Utilize a save-point if you want to pass it
 * in to a temporary function and then restore to the save point when you get it back.
 */

/* A dynamically size arena. */
typedef struct MagDynArenaBlock MagDynArenaBlock;
typedef struct
{
    MagDynArenaBlock *head_block;
    size default_block_size;

    /* Keep track of the previous allocation for realloc and free. */
    void *prev_ptr;
    size prev_offset;

    /* Extra blocks from restoring save points. */
    MagDynArenaBlock *orphaned_blocks_head;

    b32 failed_allocation;
    size max_total_allocations; /* Update during reset, save-point restore, and when directly queried.  */

} MagDynArena;

typedef struct
{
    MagDynArena *origin;

    MagDynArenaBlock *head;
    size block_buf_offset;

    void *prev_ptr;
    size prev_offset;
} MagDynArenaSavePoint;

static inline MagDynArena mag_dyn_arena_create(size default_block_size);
static inline void mag_dyn_arena_destroy(MagDynArena *arena);
static inline void mag_dyn_arena_reset(MagDynArena *arena, b32 coalesce); /* Coalesce, keep the same size but in single block, else frees all excess blocks. */
static inline void mag_dyn_arena_reset_default(MagDynArena *arena);       /* Assumes Coalesce is true.                                                       */
static inline void *mag_dyn_arena_alloc(MagDynArena *arena, size num_bytes, size alignment);
static inline void *mag_dyn_arena_realloc(MagDynArena *arena, void *ptr, size num_bytes, size alignment);
static inline void mag_dyn_arena_free(MagDynArena *arena, void *ptr);

static inline MagDynArenaSavePoint mag_dyn_arena_create_save_point(MagDynArena *arena);
static inline void mag_dyn_arena_restore_save_point(MagDynArenaSavePoint *save_point);

#define mag_dyn_arena_malloc(arena, type)               (type *)mag_dyn_arena_alloc((arena),           sizeof(type),           _Alignof(type))
#define mag_dyn_arena_nmalloc(arena, count, type)       (type *)mag_dyn_arena_alloc((arena), (count) * sizeof(type),           _Alignof(type))
#define mag_dyn_arena_nrealloc(arena, ptr, count, type) (type *)mag_dyn_arena_realloc((arena), (ptr),  sizeof(type) * (count), _Alignof(type))

static inline f64 mag_dyn_arena_max_ratio(MagDynArena *arena);
static inline b32 mag_dyn_arena_over_allocated(MagDynArena *arena);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                  Static Pool Allocator
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * This is an error prone and brittle type. If you get it all working, a refactor or code edit later is likely to break it.
 *
 * WARNING: It is the user's responsibility to make sure that there is at least object_size * num_objects bytes in the
 * backing buffer. If that isn't true, you'll probably get a seg-fault during initialization.
 *
 * WARNING: Object_size must be a multiple of sizeof(void*) to ensure the buffer is aligned to hold pointers also.
 *
 * WARNING: It is the user's responsibility to make sure the buffer is correctly aligned for the type of objects they will
 * be storing in it. If using a stack allocated or static memory section, you should use an _Alignas to force the alignment.
 */
typedef struct 
{
    size object_size;    /* The size of each object                                 */
    size num_objects;    /* The capacity, or number of objects storable in the pool */
    void *free;          /* The head of a free list of available slots for objects  */
    MagMemoryBlock buf;  /* The buffer we actually store the data in                */
} MagStaticPool;

static inline MagStaticPool mag_static_pool_create(size object_size, size num_objects, byte buffer[]);
static inline void mag_static_pool_destroy(MagStaticPool *pool);
static inline void mag_static_pool_reset(MagStaticPool *pool);
static inline void mag_static_pool_free(MagStaticPool *pool, void *ptr);
static inline void *mag_static_pool_alloc(MagStaticPool *pool); /* returns NULL if there's no more space available. */
/* no mag_static_pool_realloc because that doesn't make sense! */

#define mag_static_pool_malloc(alloc, type) (type *)mag_static_pool_alloc(alloc)

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

typedef struct
{
    MagAllocatorType type;
    union
    {
        MagStaticArenaSavePoint static_sp;
        MagDynArenaSavePoint dyn_sp;
    };
} MagAllocatorSavePoint;

static inline MagAllocator mag_allocator_dyn_arena_create(size default_block_size);
static inline MagAllocator mag_allocator_static_arena_create(size buf_size, byte buffer[]);
static inline MagAllocator mag_allocator_from_dyn_arena(MagDynArena arena);       /* Takes ownership of arena. */
static inline MagAllocator mag_allocator_from_static_arena(MagStaticArena arena); /* Takes ownership of arena. */

static inline void mag_allocator_destroy(MagAllocator *arena);
static inline void mag_allocator_reset(MagAllocator *arena); /* Clear all previous allocations. */
static inline void *mag_allocator_alloc(MagAllocator *alloc, size num_bytes, size alignment);
static inline void *mag_allocator_realloc(MagAllocator *alloc, void *ptr, size num_bytes, size alignment);
static inline void mag_allocator_free(MagAllocator *alloc, void *ptr);

static inline MagAllocatorSavePoint mag_allocator_create_save_point(MagAllocator *alloc);
static inline void mag_allocator_restore_save_point(MagAllocatorSavePoint *save_point);

#define mag_allocator_malloc(arena, type)              (type *)mag_allocator_alloc((arena),           sizeof(type),           _Alignof(type))
#define mag_allocator_nmalloc(arena, count, type)      (type *)mag_allocator_alloc((arena), (count) * sizeof(type),           _Alignof(type))
#define mag_allocator_nrealloc(arena, ptr, count, type)(type *)mag_allocator_realloc((arena), (ptr),  sizeof(type) * (count), _Alignof(type))

static inline f64 mag_allocator_max_ratio(MagAllocator *alloc);
static inline b32 mag_allocator_over_allocated(MagAllocator *alloc);

#define eco_arena_malloc(alloc, type) (type *)         _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_alloc,                        \
                                                           MagDynArena *:    mag_dyn_arena_alloc,                           \
                                                           MagAllocator *:   mag_allocator_alloc                            \
                                                       )(alloc, sizeof(type), _Alignof(type))

#define eco_arena_nmalloc(alloc, count, type) (type *) _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_alloc,                        \
                                                           MagDynArena *:    mag_dyn_arena_alloc,                           \
                                                           MagAllocator *:   mag_allocator_alloc                            \
                                                       )(alloc, (count) * sizeof(type), _Alignof(type))

#define eco_arena_nrealloc(alloc, ptr, count, type)    _Generic((alloc),                                                    \
                                                            MagStaticArena *: mag_static_arena_realloc,                     \
                                                            MagDynArena *:    mag_dyn_arena_realloc,                        \
                                                            MagAllocator *:   mag_allocator_realloc                         \
                                                       )(alloc, ptr, sizeof(type) * (count), _Alignof(type))

#define eco_arena_destroy(alloc)                       _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_destroy,                      \
                                                           MagDynArena *:    mag_dyn_arena_destroy,                         \
                                                           MagAllocator *:   mag_allocator_destroy                          \
                                                       )(alloc)

#define eco_arena_free(alloc, ptr)                     _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_free,                         \
                                                           MagDynArena *:    mag_dyn_arena_free,                            \
                                                           MagAllocator *:   mag_allocator_free                             \
                                                       )(alloc, ptr)

#define eco_arena_reset(alloc)                         _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_reset,                        \
                                                           MagDynArena *:    mag_dyn_arena_reset_default,                   \
                                                           MagAllocator *:   mag_allocator_reset                            \
                                                       )(alloc)

#define eco_arena_create_save_point(alloc)            _Generic((alloc),                                                     \
                                                           MagStaticArena *: mag_static_arena_create_save_point,            \
                                                           MagDynArena *:    mag_dyn_arena_create_save_point,               \
                                                           MagAllocator *:   mag_allocator_create_save_point                \
                                                       )(alloc)

#define eco_arena_max_ratio(alloc)                     _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_max_ratio,                    \
                                                           MagDynArena *:    mag_dyn_arena_max_ratio,                       \
                                                           MagAllocator *:   mag_allocator_max_ratio                        \
                                                       )(alloc)

#define eco_arena_over_allocated(alloc)                _Generic((alloc),                                                    \
                                                           MagStaticArena *: mag_static_arena_over_allocated,               \
                                                           MagDynArena *:    mag_dyn_arena_over_allocated,                  \
                                                           MagAllocator *:   mag_allocator_over_allocated                   \
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

#define eco_str_append(dest, src, alloc) _Generic((alloc),                                                                  \
                                             MagStaticArena *: mag_str_append_static,                                       \
                                             MagDynArena *:    mag_str_append_dyn,                                          \
                                             MagAllocator *:   mag_str_append_alloc                                         \
                                         )(dest, src, alloc)

#define eco_str_alloc_copy(src, alloc) _Generic((alloc),                                                                    \
                                             MagStaticArena *: mag_str_alloc_copy_static,                                   \
                                             MagDynArena *:    mag_str_alloc_copy_dyn,                                      \
                                             MagAllocator *:   mag_str_alloc_copy_alloc                                     \
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
    uptr mod = ptr & (a - 1); /* Same as (ptr % a) but faster as 'a' is a power of 2 */

    if (mod != 0)
    {
        /* push the address forward to the next value which is aligned               */
        ptr += a - mod;
    }

    return ptr;
}

static inline MagMemoryBlock 
mag_wrap_memory(size buf_size, void *buffer)
{
    return (MagMemoryBlock){ .mem = buffer, .size = buf_size, .flags = 0x01u | 0x00u };
}

static inline MagStaticArena
mag_static_arena_create_internal(MagMemoryBlock mem)
{
    return (MagStaticArena) 
        {
            .buf = mem,
            .buf_offset = 0,
            .prev_ptr = NULL,
            .prev_offset = 0,
            .max_offset = 0,
            .failed_allocation = false
        };
}

static inline MagStaticArena
mag_static_arena_create(size buf_size, byte buffer[])
{
    Assert(buffer && buf_size > 0);
    MagMemoryBlock mblk = mag_wrap_memory(buf_size, buffer);
    return mag_static_arena_create_internal(mblk);
}

static inline MagStaticArena 
mag_static_arena_allocate_and_create(size num_bytes)
{
    Assert(num_bytes > 0);

    MagMemoryBlock mem = mag_sys_memory_allocate(num_bytes);
    MagStaticArena arena = {0};

    if(MAG_MEM_IS_VALID(mem))
    {
        arena = mag_static_arena_create_internal(mem);
    }

    return arena;
}

static inline void
mag_static_arena_destroy(MagStaticArena *arena)
{
    MagMemoryBlock mem = arena->buf;
    *arena = (MagStaticArena){0};
    if(MAG_MEM_IS_OWNED(mem))
    {
        mag_sys_memory_free(&mem);
    }
    return;
}

static inline void
mag_static_arena_reset(MagStaticArena *arena)
{
    Assert(arena->buf.mem);
    arena->buf_offset = 0;
    arena->prev_ptr = NULL;
    arena->prev_offset = 0;
    return;
}

static inline void *
mag_static_arena_alloc(MagStaticArena *arena, size num_bytes, size alignment)
{
    Assert(num_bytes > 0 && alignment > 0);

    /* Align 'curr_offset' forward to the specified alignment */
    uptr curr_ptr = (uptr)arena->buf.mem + (uptr)arena->buf_offset;
    uptr offset = mag_align_pointer(curr_ptr, alignment);
    offset -= (uptr)arena->buf.mem; /* change to relative offset */

    /* Check to see if there is enough space left */
    if ((size)(offset + num_bytes) <= arena->buf.size)
    {
        void *ptr = &arena->buf.mem[offset];
        memset(ptr, 0, num_bytes);
        
        arena->prev_offset = arena->buf_offset;
        arena->prev_ptr = ptr;

        arena->buf_offset = offset + num_bytes;
        arena->max_offset = arena->max_offset < arena->buf_offset ? arena->buf_offset : arena->max_offset;

        return ptr;
    }

    arena->failed_allocation = true;
    return NULL;
}

static inline void * 
mag_static_arena_realloc(MagStaticArena *arena, void *ptr, size asize, size alignment)
{
    Assert(asize > 0);

    if(ptr == arena->prev_ptr)
    {
        /* Get previous extra offset due to alignment */
        uptr offset = (uptr)ptr - (uptr)arena->buf.mem; /* relative offset accounting for alignment */

        /* Check to see if there is enough space left */
        if ((size)(offset + asize) <= arena->buf.size)
        {
            arena->buf_offset = offset + asize;
            arena->max_offset = arena->max_offset < arena->buf_offset ? arena->buf_offset : arena->max_offset;

            return ptr;
        }
        else
        {
            arena->failed_allocation = true;
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

static inline MagStaticArenaSavePoint 
mag_static_arena_create_save_point(MagStaticArena *arena)
{
    return (MagStaticArenaSavePoint)
        {
            .origin = arena,
            .buf_offset = arena->buf_offset,
            .prev_ptr = arena->prev_ptr,
            .prev_offset = arena->prev_offset
        };
}

static inline void 
mag_static_arena_restore_save_point(MagStaticArenaSavePoint *save_point)
{
    MagStaticArena *origin = save_point->origin;
    origin->buf_offset = save_point->buf_offset;
    origin->prev_ptr = save_point->prev_ptr;
    origin->prev_offset = save_point->prev_offset;
}

static inline f64 
mag_static_arena_max_ratio(MagStaticArena *arena)
{
    return (f64)arena->max_offset / (f64)arena->buf.size;
}

static inline b32 
mag_static_arena_over_allocated(MagStaticArena *arena)
{
    return arena->failed_allocation;
}

struct MagDynArenaBlock
{
    MagMemoryBlock buf;
    size buf_offset;
    struct MagDynArenaBlock *next;

    size max_buf_offset;

};

static inline MagDynArenaBlock *
mag_dyn_arena_block_create(size block_size)
{
    MagMemoryBlock mem = mag_sys_memory_allocate(block_size + sizeof(MagDynArenaBlock));
    if(MAG_MEM_IS_VALID(mem))
    {
        /* Place the block metadata at the beginning of the memory block. */
        MagDynArenaBlock *block = (void *)mem.mem;
        block->buf_offset = sizeof(MagDynArenaBlock);
        block->buf = mem;
        block->next = NULL;
        block->max_buf_offset = sizeof(MagDynArenaBlock);
        return block;
    }

    return NULL;
}

static inline void *
mag_dyn_arena_block_alloc(MagDynArenaBlock *block, size num_bytes, size alignment, void **prev_ptr, size *prev_offset)
{
    Assert(num_bytes > 0 && alignment > 0);

    /* Align 'curr_offset' forward to the specified alignment */
    uptr curr_ptr = (uptr)block->buf.mem + (uptr)block->buf_offset;
    uptr offset = mag_align_pointer(curr_ptr, alignment);
    offset -= (uptr)block->buf.mem; /* change to relative offset */

    /* Check to see if there is enough space left */
    if ((size)(offset + num_bytes) <= block->buf.size)
    {
        void *ptr = &block->buf.mem[offset];
        memset(ptr, 0, num_bytes);

        *prev_offset = block->buf_offset;
        *prev_ptr = ptr;

        block->buf_offset = offset + num_bytes;

        if(block->buf_offset > block->max_buf_offset) { block->max_buf_offset = block->buf_offset; }

        return ptr;
    }

    return NULL;
}

static inline MagDynArena 
mag_dyn_arena_create(size default_block_size)
{
    MagDynArena arena = {0};

    MagDynArenaBlock *block = mag_dyn_arena_block_create(default_block_size);
    if(!block) { return arena; }

    arena.head_block = block;
    arena.default_block_size = default_block_size;

    arena.prev_offset = 0;
    arena.prev_ptr = block;

    arena.orphaned_blocks_head = NULL;

    arena.failed_allocation = false;
    arena.max_total_allocations = 0;

    return arena;
}

static inline void 
mag_dyn_arena_destroy(MagDynArena *arena)
{
    /* Iterate through linked list and free all the blocks. */
    MagDynArenaBlock *curr = arena->head_block;
    while(curr)
    {
        MagDynArenaBlock *next = curr->next;
        mag_sys_memory_free(&curr->buf);
        curr = next;
    }

    curr = arena->orphaned_blocks_head;
    while(curr)
    {
        MagDynArenaBlock *next = curr->next;
        mag_sys_memory_free(&curr->buf);
        curr = next;
    }

    *arena = (MagDynArena){0};

    return;
}

static inline size
mag_dyn_arena_internal_calc_total_allocations(MagDynArena *arena)
{
    size total_allocation_size = 0;

    MagDynArenaBlock *curr = arena->head_block;
    while(curr)
    {
        total_allocation_size += curr->max_buf_offset;
        curr = curr->next;
    }

    curr = arena->orphaned_blocks_head;
    while(curr)
    {
        total_allocation_size += curr->max_buf_offset;
        curr = curr->next;
    }

    if(total_allocation_size > arena->max_total_allocations) 
    {
        arena->max_total_allocations = total_allocation_size;
    }
    else
    {
        total_allocation_size = arena->max_total_allocations;
    }

    return total_allocation_size;
}

static inline void 
mag_dyn_arena_reset(MagDynArena *arena, b32 coalesce)
{
    /* FIXME: To be more robust, free all old allocations first before creating one big new one. */

    /* Only actually do the coalesce if there is more than 1 block. */
    b32 do_coalesce = coalesce && (arena->head_block->next || arena->orphaned_blocks_head);

    mag_dyn_arena_internal_calc_total_allocations(arena);

    MagDynArenaBlock *curr = NULL;
    if(do_coalesce)
    {
        /* Queue up all of the blocks to be deleted. */
        curr = arena->head_block;

        /* Create a block large enough to hold ALL the data from last time in a single block. */
        MagDynArenaBlock *block = mag_dyn_arena_block_create(arena->max_total_allocations);
        if(!block)
        {
            arena->failed_allocation = true;
        }

        arena->head_block = block;
        /* arena->default_block_size = ...doesn't need changed; */

        arena->prev_offset = 0;
        arena->prev_ptr = block;
    }
    else
    {
        /* Reset the first block. */
        arena->head_block->buf_offset = sizeof(MagDynArenaBlock); /* Not 0! Don't overwrite the block metadata. */

        arena->prev_offset = 0;
        arena->prev_ptr = arena->head_block;

        /* Queue up the rest of the blocks to be deleted. */
        curr = arena->head_block->next;
        arena->head_block->next = NULL; /* Make sure the linked list is properly terminated. */
    }

    /* Free the unneeded blocks. */
    while(curr)
    {
        MagDynArenaBlock *next = curr->next;
        mag_sys_memory_free(&curr->buf);
        curr = next;
    }

    curr = arena->orphaned_blocks_head;
    while(curr)
    {
        MagDynArenaBlock *next = curr->next;
        mag_sys_memory_free(&curr->buf);
        curr = next;
    }
    arena->orphaned_blocks_head = NULL;

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
    MagDynArenaBlock *block = NULL;

    /* Take the first orphaned block that will do. */
    MagDynArenaBlock *candidate = arena->orphaned_blocks_head;
    MagDynArenaBlock **prev_candidate_next = &arena->orphaned_blocks_head;
    while(candidate)
    {
        if(candidate->buf.size >= min_bytes)
        {
            block = candidate;
            *prev_candidate_next = candidate->next;
            block->buf_offset = 0;
            break;
        }

        prev_candidate_next = &candidate->next;
        candidate = candidate->next;
    }

    /* If we couldn't find an acceptable one among the orphans, then allocate a new block from the system. */
    if(!block) { block = mag_dyn_arena_block_create(min_bytes); }

    if(block)
    {
        block->next = arena->head_block;
        arena->head_block = block;
    }

    return block;
}

static inline void *
mag_dyn_arena_alloc(MagDynArena *arena, size num_bytes, size alignment)
{
    Assert(num_bytes > 0 && alignment > 0);

    /* Look for space in the current block. */
    MagDynArenaBlock *block = arena->head_block;
    void *ptr = mag_dyn_arena_block_alloc(block, num_bytes, alignment, &arena->prev_ptr, &arena->prev_offset);
    if(ptr) { return ptr; }

    /* FIXME: ? I could also search all the other blocks in the linked list for space this size. This is a memory efficiency
     * vs. speed trade off. I probably will not find space in the other blocks, so this will usually be a waste of time. It
     * could help save memory waste though if my default_block_size is poorly tuned to the application.
     */

    /* We need to add another block. */
    size block_size = num_bytes + alignment > arena->default_block_size ? num_bytes + alignment : arena->default_block_size;
    block = mag_dyn_arena_add_block(arena, block_size);
    if(block)
    {
        ptr = mag_dyn_arena_block_alloc(block, num_bytes, alignment, &arena->prev_ptr, &arena->prev_offset);
    }

    /* Give up. */
    if(!ptr) {arena->failed_allocation = true; }
    return ptr;
}

static inline void *
mag_dyn_arena_realloc(MagDynArena *arena, void *ptr, size num_bytes, size alignment)
{
    Assert(num_bytes > 0);

    if(ptr == arena->prev_ptr)
    {
        /* Get previous extra offset due to alignment */
        uptr offset = (uptr)ptr - (uptr)arena->head_block->buf.mem; /* relative offset accounting for alignment */

        /* Check to see if there is enough space left */
        if ((size)(offset + num_bytes) <= arena->head_block->buf.size)
        {
            arena->head_block->buf_offset = offset + num_bytes;
            return ptr;
        }
    }

    /* Get the previous allocation size, or at least a ceiling for it. */
    size prev_alloc_size_ceiling = arena->head_block->buf_offset + (uptr)arena->head_block->buf.mem - (uptr)ptr; 
    prev_alloc_size_ceiling = prev_alloc_size_ceiling < num_bytes ? prev_alloc_size_ceiling : num_bytes;

    void *new = mag_dyn_arena_alloc(arena, num_bytes, alignment);
    if(new) { memcpy(new, ptr, prev_alloc_size_ceiling); }

    /* Failed allocations are tracked in mag_dyn_arena_alloc */

    return new;
}

static inline void 
mag_dyn_arena_free(MagDynArena *arena, void *ptr)
{
    if(ptr == arena->prev_ptr)
    {
        arena->head_block->buf_offset = arena->prev_offset;
    }

    return;
}

static inline f64 
mag_dyn_arena_max_ratio(MagDynArena *arena)
{
    size used = mag_dyn_arena_internal_calc_total_allocations(arena);

    return (f64)used / (f64)arena->default_block_size;
}

static inline MagDynArenaSavePoint 
mag_dyn_arena_create_save_point(MagDynArena *arena)
{
    return (MagDynArenaSavePoint)
        {
            .origin = arena,
            .head = arena->head_block,
            .block_buf_offset = arena->head_block->buf_offset,
            .prev_ptr = arena->prev_ptr,
            .prev_offset = arena->prev_offset
        };
}

static inline void 
mag_dyn_arena_restore_save_point(MagDynArenaSavePoint *save_point)
{
    MagDynArena *origin = save_point->origin;
    MagDynArenaBlock *head = origin->head_block;

    if(head != save_point->head)
    {
        MagDynArenaBlock *tail = origin->orphaned_blocks_head; /* Preserve any blocks already on the list. */
        origin->orphaned_blocks_head = head;

        MagDynArenaBlock *curr = head;
        while(curr->next != save_point->head)
        {
            curr = curr->next;
        }

        Assert(curr->next == save_point->head);

        curr->next = tail;
    }

    origin->head_block = save_point->head;
    origin->head_block->buf_offset = save_point->block_buf_offset;
    origin->prev_ptr = save_point->prev_ptr;
    origin->prev_offset = save_point->prev_offset;
    return;
}

static inline b32 
mag_dyn_arena_over_allocated(MagDynArena *arena)
{
    return arena->failed_allocation;
}

static inline void
mag_static_pool_initialize_linked_list(byte *buffer, size object_size, size num_objects)
{

    /* Initialize the free list to a linked list. */

    /* start by pointing to last element and assigning it NULL */
    size offset = object_size * (num_objects - 1);
    uptr *ptr = (uptr *)&buffer[offset];
    *ptr = (uptr)NULL;

    /* Then work backwards to the front of the list. */
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
    Assert(pool && pool->buf.mem && pool->num_objects && pool->object_size);

    /* Initialize the free list to a linked list. */
    mag_static_pool_initialize_linked_list(pool->buf.mem, pool->object_size, pool->num_objects);
    pool->free = &pool->buf.mem[0];
}

static inline MagStaticPool
mag_static_pool_create(size object_size, size num_objects, byte buffer[])
{
    Assert(object_size >= sizeof(void *));       /* Need to be able to fit at least a pointer! */
    Assert(object_size % _Alignof(void *) == 0); /* Need for alignment of pointers.            */
    Assert(num_objects > 0);

    MagMemoryBlock mblk = mag_wrap_memory(num_objects * object_size, buffer);
    MagStaticPool pool = { .buf = mblk, .object_size = object_size, .num_objects = num_objects };

    mag_static_pool_reset(&pool);

    return pool;
}

static inline void
mag_static_pool_destroy(MagStaticPool *pool)
{
    mag_sys_memory_free(&pool->buf);
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
        default: { Panic(); }
    }
}

static inline void 
mag_allocator_reset(MagAllocator *alloc)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: mag_static_arena_reset(&alloc->static_arena);   break;
        case MAG_ALLOC_T_DYN_ARENA:    mag_dyn_arena_reset_default(&alloc->dyn_arena); break;
        default: { Panic(); }
    }
}

static inline void *
mag_allocator_alloc(MagAllocator *alloc, size num_bytes, size alignment)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: return mag_static_arena_alloc(&alloc->static_arena, num_bytes, alignment);
        case MAG_ALLOC_T_DYN_ARENA:    return mag_dyn_arena_alloc(&alloc->dyn_arena, num_bytes, alignment);
        default: { Panic(); }
    }

    return NULL;
}

static inline void *
mag_allocator_realloc(MagAllocator *alloc, void *ptr, size num_bytes, size alignment)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: return mag_static_arena_realloc(&alloc->static_arena, ptr, num_bytes, alignment);
        case MAG_ALLOC_T_DYN_ARENA:    return mag_dyn_arena_realloc(&alloc->dyn_arena, ptr, num_bytes, alignment);
        default: { Panic(); }
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
        default: { Panic(); }
    }
}

static inline MagAllocatorSavePoint 
mag_allocator_create_save_point(MagAllocator *alloc)
{
    MagAllocatorSavePoint sp = { .type = alloc->type };
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: sp.static_sp = mag_static_arena_create_save_point(&alloc->static_arena); break;
        case MAG_ALLOC_T_DYN_ARENA:    sp.dyn_sp =    mag_dyn_arena_create_save_point(&alloc->dyn_arena);       break;
        default: { Panic(); }
    }

    return sp;
}

static inline void 
mag_allocator_restore_save_point(MagAllocatorSavePoint *save_point)
{
    switch(save_point->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: mag_static_arena_restore_save_point(&save_point->static_sp); break;
        case MAG_ALLOC_T_DYN_ARENA:    mag_dyn_arena_restore_save_point(&save_point->dyn_sp);       break;
        default: { Panic(); }
    }

}

static inline f64 
mag_allocator_max_ratio(MagAllocator *alloc)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: return mag_static_arena_max_ratio(&alloc->static_arena);
        case MAG_ALLOC_T_DYN_ARENA:    return mag_dyn_arena_max_ratio(&alloc->dyn_arena);
        default: { Panic(); }
    }

    return 0.0; /* Unreachable. */
}

static inline b32 
mag_allocator_over_allocated(MagAllocator *alloc)
{
    switch(alloc->type)
    {
        case MAG_ALLOC_T_STATIC_ARENA: return mag_static_arena_over_allocated(&alloc->static_arena);
        case MAG_ALLOC_T_DYN_ARENA:    return mag_dyn_arena_over_allocated(&alloc->dyn_arena);
        default: { Panic(); }
    }

    return true; /* Unreachable. */
}

static inline ElkStr 
mag_str_alloc_copy_static(ElkStr src, MagStaticArena *arena)
{
    ElkStr ret_val = {0};

    size copy_len = src.len + 1; /* Add room for terminating zero. */
    char *buffer = mag_static_arena_nmalloc(arena, copy_len, char);
    StopIf(!buffer, return ret_val); /* Return NULL string if out of memory */

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

    size copy_len = src.len + 1; /* Add room for terminating zero. */
    char *buffer = mag_dyn_arena_nmalloc(arena, copy_len, char);
    StopIf(!buffer, return ret_val); /* Return NULL string if out of memory */

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

    size copy_len = src.len + 1; /* Add room for terminating zero. */
    char *buffer = mag_allocator_nmalloc(alloc, copy_len, char);
    StopIf(!buffer, return ret_val); /* Return NULL string if out of memory. */

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
