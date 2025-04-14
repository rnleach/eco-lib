#ifndef _PACKRAT_H_
#define _PACKRAT_H_

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     String Interner
 *---------------------------------------------------------------------------------------------------------------------------
 *
 * size_exp - The interner is backed by a hash table with a capacity that is a power of 2. The size_exp is that power of two.
 *     This value is only used initially, if the table needs to expand, it will, so it's OK to start with small values here.
 *     However, if you know it will grow larger, it's better to start larger! For most reasonable use cases, it really
 *     probably shouldn't be smaller than 5, but no checks are done for this.
 *
 * NOTE: A cstring is a null terminated string of unknown length.
 *
 * NOTE: The interner copies any successfully interned string, so once it has been interned you can reclaim the memory that 
 * was holding it before.
 */
typedef struct // Internal only
{
    u64 hash;
    ElkStr str;
} ElkStringInternerHandle;

typedef struct 
{
    ElkStaticArena *storage;          // This is where to store the strings

    ElkStringInternerHandle *handles; // The hash table - handles index into storage
    u32 num_handles;                  // The number of handles
    i8 size_exp;                      // Used to keep track of the length of *handles
} ElkStringInterner;


static inline ElkStringInterner elk_string_interner_create(i8 size_exp, ElkStaticArena *storage);
static inline void elk_string_interner_destroy(ElkStringInterner *interner);
static inline ElkStr elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string);
static inline ElkStr elk_string_interner_intern(ElkStringInterner *interner, ElkStr str);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                       Collections
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                         
 *                                                   Ordered Collections
 *
 *---------------------------------------------------------------------------------------------------------------------------
 *
 *  Ordered collections are things like arrays (sometimes called vectors), queues, dequeues, and heaps. I'm taking a 
 *  different approach to how I implement them. They will be implemented in two parts, the second of which might not be
 *  necessary. 
 *
 *  The first part is the ledger, which just does all the bookkeeping about how full the collection is, which elements are
 *  valid, and the order of elements. A user can use these with their own supplied buffer for storing objects. The ledger
 *  types only track indexes.
 *
 *  One advantage of the ledger approach is that the user can manage their own memory, allowing them to use the most
 *  efficient allocation strategy. 
 *
 *  Another advantage of this is that if you have several parallel collections (e.g. parallel arrays), you can use a single
 *  instance of a bookkeeping type (e.g. ElkQueueLedger) to track the state of all the arrays that back it. Further,
 *  different collections in the parallel collections can have different sized objects.
 *
 *  Complicated memory management can be a disadvantage of the ledger approach. For instance, implementing growable
 *  collections can require shuffling objects around in the backing buffer during the resize operation. A queue, for 
 *  instance, is non-trivial to expand because you have to account for wrap-around (assuming it is backed by a circular
 *  buffer). So not all the ledger types APIs will directly support resizing, and even those that do will further require 
 *  the user to make sure that as they allocate more memory in the backing buffer, they also update the ledger. Finally, if
 *  you want to pass the collection as a whole to a function, you'll have to pass the ledger and buffer(s) separately or
 *  create your own composite type to package them together in.
 *
 *  The second part is an implementation that manages memory for the user. This gives the user less control over how memory
 *  is allocated / freed, but it also burdens them less with the responsibility of doing so. These types can reliably be
 *  expanded on demand. These types will internally use the ledger types.
 *
 *  The more full featured types that manage memory also require the use of void pointers for adding and retrieving data 
 *  from the collections. So they are less type safe, whereas when using just the ledger types they only deal in indexes, so
 *  you can make the backing buffer have any type you want.
 *
 *  Adjacent just means that the objects are stored adjacently in memory. All of the collection ledger types must be backed
 *  by an array or a block of contiguous memory.
 *
 *  NOTE: The resizable collections may not be implemented unless I need them. I've found that I need dynamically expandable
 *  collections much less frequently.
 */

static size const PAK_COLLECTION_EMPTY = ELK_COLLECTION_EMPTY;
static size const PAK_COLLECTION_FULL = ELK_COLLECTION_FULL;

/*---------------------------------------------------------------------------------------------------------------------------
 *                                         
 *                                                  Unordered Collections
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    Hash Map (Table)
 *---------------------------------------------------------------------------------------------------------------------------
 * The table size must be a power of two, so size_exp is used to calcualte the size of the table. If it needs to grow in 
 * size, it will grow the table, so this is only a starting point.
 *
 * The ELkHashMap does NOT copy any objects, so it only stores pointers. The user has to manage the memory for their own
 * objects.
 */
typedef struct // Internal Only
{
    u64 hash;
    void *key;
    void *value;
} ElkHashMapHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkHashMapHandle *handles;
    size num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
    i8 size_exp;
} ElkHashMap;

typedef size ElkHashMapKeyIter;

static inline ElkHashMap elk_hash_map_create(i8 size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq, ElkStaticArena *arena);
static inline void elk_hash_map_destroy(ElkHashMap *map);
static inline void *elk_hash_map_insert(ElkHashMap *map, void *key, void *value); // if return != value, key was already in the map
static inline void *elk_hash_map_lookup(ElkHashMap *map, void *key); // return NULL if not in map, otherwise return pointer to value
static inline size elk_hash_map_len(ElkHashMap *map);
static inline ElkHashMapKeyIter elk_hash_map_key_iter(ElkHashMap *map);

static inline void *elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter);

/*--------------------------------------------------------------------------------------------------------------------------
 *                                            Hash Map (Table, ElkStr as keys)
 *---------------------------------------------------------------------------------------------------------------------------
 * Designed for use when keys are strings.
 *
 * Values are not copied, they are stored as pointers, so the user must manage memory.
 *
 * Uses fnv1a hash.
 */
typedef struct
{
    u64 hash;
    ElkStr key;
    void *value;
} ElkStrMapHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkStrMapHandle *handles;
    size num_handles;
    i8 size_exp;
} ElkStrMap;

typedef size ElkStrMapKeyIter;
typedef size ElkStrMapHandleIter;

static inline ElkStrMap elk_str_map_create(i8 size_exp, ElkStaticArena *arena);
static inline void elk_str_map_destroy(ElkStrMap *map);
static inline void *elk_str_map_insert(ElkStrMap *map, ElkStr key, void *value); // if return != value, key was already in the map
static inline void *elk_str_map_lookup(ElkStrMap *map, ElkStr key); // return NULL if not in map, otherwise return pointer to value
static inline ElkStrMapHandle const *elk_str_map_lookup_handle(ElkStrMap *map, ElkStr key); // return NULL if not in map, otherwise return pointer to handle
static inline size elk_str_map_len(ElkStrMap *map);
static inline ElkStrMapKeyIter elk_str_map_key_iter(ElkStrMap *map);
static inline ElkStrMapHandleIter elk_str_map_handle_iter(ElkStrMap *map);

static inline ElkStr elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter);
static inline ElkStrMapHandle elk_str_map_handle_iter_next(ElkStrMap *map, ElkStrMapHandleIter *iter);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                        Hash Set
 *---------------------------------------------------------------------------------------------------------------------------
 * The table size must be a power of two, so size_exp is used to calcualte the size of the table. If it needs to grow in 
 * size, it will grow the table, so this is only a starting point.
 *
 * The ELkHashSet does NOT copy any objects, so it only stores pointers. The user has to manage the memory for their own
 * objects.
 */
typedef struct // Internal only
{
    u64 hash;
    void *value;
} ElkHashSetHandle;

typedef struct 
{
    ElkStaticArena *arena;
    ElkHashSetHandle *handles;
    size num_handles;
    ElkSimpleHash hasher;
    ElkEqFunction eq;
    i8 size_exp;
} ElkHashSet;

typedef size ElkHashSetIter;

static inline ElkHashSet elk_hash_set_create(i8 size_exp, ElkSimpleHash val_hash, ElkEqFunction val_eq, ElkStaticArena *arena);
static inline void elk_hash_set_destroy(ElkHashSet *set);
static inline void *elk_hash_set_insert(ElkHashSet *set, void *value); // if return != value, value was already in the set
static inline void *elk_hash_set_lookup(ElkHashSet *set, void *value); // return NULL if not in set, else return ptr to value
static inline size elk_hash_set_len(ElkHashSet *set);
static inline ElkHashSetIter elk_hash_set_value_iter(ElkHashSet *set);

static inline void *elk_hash_set_value_iter_next(ElkHashSet *set, ElkHashSetIter *iter);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                            Generic Macros for Collections
 *---------------------------------------------------------------------------------------------------------------------------
 * These macros take any collection and return a result.
 */
#define pak_len(x) _Generic((x),                                                                                            \
        ElkQueueLedger *: elk_queue_ledger_len,                                                                             \
        ElkArrayLedger *: elk_array_ledger_len,                                                                             \
        ElkHashMap *: elk_hash_map_len,                                                                                     \
        ElkStrMap *: elk_str_map_len,                                                                                       \
        ElkHashSet *: elk_hash_set_len)(x)

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                         
 *                                                        Sorting
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       Radix Sort 
 *---------------------------------------------------------------------------------------------------------------------------
 * I've only implemented radix sort for fixed size signed intengers, unsigned integers, and floating point types thus far.
 *
 * The sort requires extra memory that depends on the size of the list being sorted, so the user must provide that. In order
 * to ensure proper alignment and size, the user must provide the scratch buffer. Probably using an arena with 
 * elk_static_arena_nmalloc() to create a buffer and then freeing it after the sort would be the most efficient way to handle
 * that.
 *
 * The API is set up for sorting an array of structures. The 'offset' argument is the offset in bytes into the structure 
 * where the sort key can be found. The 'stride' argument is just the size of the structures so we know how to iterate the
 * array. The sort order (ascending / descending) and sort key type (signed integer, unsigned integer, floating) are passed
 * as arguments. The sort key type includes the size of the sort key in bits.
 */

typedef enum
{
    ELK_RADIX_SORT_UINT8,
    ELK_RADIX_SORT_INT8,
    ELK_RADIX_SORT_UINT16,
    ELK_RADIX_SORT_INT16,
    ELK_RADIX_SORT_UINT32,
    ELK_RADIX_SORT_INT32,
    ELK_RADIX_SORT_F32,
    ELK_RADIX_SORT_UINT64,
    ELK_RADIX_SORT_INT64,
    ELK_RADIX_SORT_F64,
} ElkRadixSortByType;

typedef enum { ELK_SORT_ASCENDING, ELK_SORT_DESCENDING } ElkSortOrder;

static inline void elk_radix_sort(
        void *buffer, 
        size num, 
        size offset, 
        size stride, 
        void *scratch, 
        ElkRadixSortByType sort_type, 
        ElkSortOrder order);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *                                          Implementation of `inline` functions.
 *
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static inline ElkStringInterner 
elk_string_interner_create(i8 size_exp, ElkStaticArena *storage)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    usize const handles_len = (usize)(1 << size_exp);
    ElkStringInternerHandle *handles = elk_static_arena_nmalloc(storage, handles_len, ElkStringInternerHandle);
    PanicIf(!handles);

    return (ElkStringInterner)
    {
        .storage = storage,
        .handles = handles,
        .num_handles = 0, 
        .size_exp = size_exp
    };
}

static inline void
elk_string_interner_destroy(ElkStringInterner *interner)
{
    return;
}

static inline b32
elk_hash_table_large_enough(usize num_handles, i8 size_exp)
{
    // Shoot for no more than 75% of slots filled.
    return num_handles < 3 * (1 << size_exp) / 4;
}

static inline u32
elk_hash_lookup(u64 hash, i8 exp, u32 idx)
{
    // Copied from https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.
    u32 mask = (UINT32_C(1) << exp) - 1;
    u32 step = (u32)((hash >> (64 - exp)) | 1);    // the | 1 forces an odd number
    return (idx + step) & mask;
}

static inline void
elk_string_interner_expand_table(ElkStringInterner *interner)
{
    i8 const size_exp = interner->size_exp;
    i8 const new_size_exp = size_exp + 1;

    usize const handles_len = (usize)(1 << size_exp);
    usize const new_handles_len = (usize)(1 << new_size_exp);

    ElkStringInternerHandle *new_handles = elk_static_arena_nmalloc(interner->storage, new_handles_len, ElkStringInternerHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (handle->str.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkStringInternerHandle *new_handle = &new_handles[j];

            if (!new_handle->str.start)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    interner->handles = new_handles;
    interner->size_exp = new_size_exp;

    return;
}

static inline ElkStr
elk_string_interner_intern_cstring(ElkStringInterner *interner, char *string)
{
    ElkStr str = elk_str_from_cstring(string);
    return elk_string_interner_intern(interner, str);
}

static inline ElkStr
elk_string_interner_intern(ElkStringInterner *interner, ElkStr str)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = elk_fnv1a_hash_str(str);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, interner->size_exp, i);
        ElkStringInternerHandle *handle = &interner->handles[i];

        if (!handle->str.start)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(interner->num_handles, interner->size_exp))
            {
                char *dest = elk_static_arena_nmalloc(interner->storage, str.len + 1, char);
                PanicIf(!dest);
                ElkStr interned_str = elk_str_copy(str.len + 1, dest, str);

                *handle = (ElkStringInternerHandle){.hash = hash, .str = interned_str};
                interner->num_handles += 1;

                return handle->str;
            }
            else 
            {
                // Grow the table so we have room
                elk_string_interner_expand_table(interner);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_string_interner_intern(interner, str);
            }
        }
        else if (handle->hash == hash && !elk_str_cmp(str, handle->str)) 
        {
            // found it!
            return handle->str;
        }
    }
}

static ElkHashMap 
elk_hash_map_create(i8 size_exp, ElkSimpleHash key_hash, ElkEqFunction key_eq, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size const handles_len = (size)(1 << size_exp);
    ElkHashMapHandle *handles = elk_static_arena_nmalloc(arena, handles_len, ElkHashMapHandle);
    PanicIf(!handles);

    return (ElkHashMap)
    {
        .arena = arena,
        .handles = handles,
        .num_handles = 0, 
        .hasher = key_hash,
        .eq = key_eq,
        .size_exp = size_exp
    };
}

static inline void 
elk_hash_map_destroy(ElkHashMap *map)
{
    return;
}

static inline void
elk_hash_table_expand(ElkHashMap *map)
{
    i8 const size_exp = map->size_exp;
    i8 const new_size_exp = size_exp + 1;

    size const handles_len = (size)(1 << size_exp);
    size const new_handles_len = (size)(1 << new_size_exp);

    ElkHashMapHandle *new_handles = elk_static_arena_nmalloc(map->arena, new_handles_len, ElkHashMapHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkHashMapHandle *handle = &map->handles[i];

        if (handle->key == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkHashMapHandle *new_handle = &new_handles[j];

            if (!new_handle->key)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    map->handles = new_handles;
    map->size_exp = new_size_exp;

    return;
}

static inline void *
elk_hash_map_insert(ElkHashMap *map, void *key, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = map->hasher(key);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkHashMapHandle *handle = &map->handles[i];

        if (!handle->key)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(map->num_handles, map->size_exp))
            {

                *handle = (ElkHashMapHandle){.hash = hash, .key=key, .value=value};
                map->num_handles += 1;

                return handle->value;
            }
            else 
            {
                // Grow the table so we have room
                elk_hash_table_expand(map);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_hash_map_insert(map, key, value);
            }
        }
        else if (handle->hash == hash && map->eq(handle->key, key)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

static inline void *
elk_hash_map_lookup(ElkHashMap *map, void *key)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = map->hasher(key);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkHashMapHandle *handle = &map->handles[i];

        if (!handle->key)
        {
            return NULL;
            
        }
        else if (handle->hash == hash && map->eq(handle->key, key)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

static inline size 
elk_hash_map_len(ElkHashMap *map)
{
    return map->num_handles;
}

static inline ElkHashMapKeyIter 
elk_hash_map_key_iter(ElkHashMap *map)
{
    return 0;
}

static inline void *
elk_hash_map_key_iter_next(ElkHashMap *map, ElkHashMapKeyIter *iter)
{
    size const max_iter = (size)(1 << map->size_exp);
    void *next_key = NULL;
    if(*iter >= max_iter) { return next_key; }

    do
    {
        next_key = map->handles[*iter].key;
        *iter += 1;

    } while(next_key == NULL && *iter < max_iter);

    return next_key;
}

static inline ElkStrMap 
elk_str_map_create(i8 size_exp, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size const handles_len = (size)(1 << size_exp);
    ElkStrMapHandle *handles = elk_static_arena_nmalloc(arena, handles_len, ElkStrMapHandle);
    PanicIf(!handles);

    return (ElkStrMap)
    {
        .arena = arena,
        .handles = handles,
        .num_handles = 0, 
        .size_exp = size_exp,
    };
}

static inline void
elk_str_map_destroy(ElkStrMap *map)
{
    return;
}

static inline void
elk_str_table_expand(ElkStrMap *map)
{
    i8 const size_exp = map->size_exp;
    i8 const new_size_exp = size_exp + 1;

    size const handles_len = (size)(1 << size_exp);
    size const new_handles_len = (size)(1 << new_size_exp);

    ElkStrMapHandle *new_handles = elk_static_arena_nmalloc(map->arena, new_handles_len, ElkStrMapHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkStrMapHandle *handle = &map->handles[i];

        if (handle->key.start == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkStrMapHandle *new_handle = &new_handles[j];

            if (!new_handle->key.start)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    map->handles = new_handles;
    map->size_exp = new_size_exp;

    return;
}

static inline void *
elk_str_map_insert(ElkStrMap *map, ElkStr key, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = elk_fnv1a_hash_str(key);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkStrMapHandle *handle = &map->handles[i];

        if (!handle->key.start)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(map->num_handles, map->size_exp))
            {

                *handle = (ElkStrMapHandle){.hash = hash, .key=key, .value=value};
                map->num_handles += 1;

                return handle->value;
            }
            else 
            {
                // Grow the table so we have room
                elk_str_table_expand(map);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the table.
                return elk_str_map_insert(map, key, value);
            }
        }
        else if (handle->hash == hash && !elk_str_cmp(key, handle->key)) 
        {
            // found it! Replace value
            void *tmp = handle->value;
            handle->value = value;

            return tmp;
        }
    }
}

static inline void *
elk_str_map_lookup(ElkStrMap *map, ElkStr key)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = elk_fnv1a_hash_str(key);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkStrMapHandle *handle = &map->handles[i];

        if (!handle->key.start) { return NULL; }
        else if (handle->hash == hash && !elk_str_cmp(key, handle->key)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

static inline ElkStrMapHandle const *
elk_str_map_lookup_handle(ElkStrMap *map, ElkStr key)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = elk_fnv1a_hash_str(key);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, map->size_exp, i);
        ElkStrMapHandle *handle = &map->handles[i];

        if (!handle->key.start) { return NULL; }
        else if (handle->hash == hash && !elk_str_cmp(key, handle->key)) 
        {
            // found it!
            return handle;
        }
    }

    return NULL;
}

static inline size
elk_str_map_len(ElkStrMap *map)
{
    return map->num_handles;
}

static inline ElkHashMapKeyIter 
elk_str_map_key_iter(ElkStrMap *map)
{
    return 0;
}

static inline ElkStrMapHandleIter 
elk_str_map_handle_iter(ElkStrMap *map)
{
    return 0;
}

static inline ElkStr 
elk_str_map_key_iter_next(ElkStrMap *map, ElkStrMapKeyIter *iter)
{
    size const max_iter = (size)(1 << map->size_exp);
    if(*iter < max_iter) 
    {
        ElkStr next_key = map->handles[*iter].key;
        *iter += 1;
        while(next_key.start == NULL && *iter < max_iter)
        {
            next_key = map->handles[*iter].key;
            *iter += 1;
        }

        return next_key;
    }
    else
    {
        return (ElkStr){.start=NULL, .len=0};
    }

}

static inline ElkStrMapHandle 
elk_str_map_handle_iter_next(ElkStrMap *map, ElkStrMapHandleIter *iter)
{
    size const max_iter = (size)(1 << map->size_exp);
    if(*iter < max_iter) 
    {
        ElkStrMapHandle next = map->handles[*iter];
        *iter += 1;
        while(next.key.start == NULL && *iter < max_iter)
        {
            next = map->handles[*iter];
            *iter += 1;
        }

        return next;
    }
    else
    {
        return (ElkStrMapHandle){ .key = (ElkStr){.start=NULL, .len=0}, .hash = 0, .value = NULL };
    }
}

static inline ElkHashSet 
elk_hash_set_create(i8 size_exp, ElkSimpleHash val_hash, ElkEqFunction val_eq, ElkStaticArena *arena)
{
    Assert(size_exp > 0 && size_exp <= 31); // Come on, 31 is HUGE

    size const handles_len = (size)(1 << size_exp);
    ElkHashSetHandle *handles = elk_static_arena_nmalloc(arena, handles_len, ElkHashSetHandle);
    PanicIf(!handles);

    return (ElkHashSet)
    {
        .arena = arena,
        .handles = handles,
        .num_handles = 0, 
        .hasher = val_hash,
        .eq = val_eq,
        .size_exp = size_exp
    };
}

static inline void 
elk_hash_set_destroy(ElkHashSet *set)
{
    return;
}

static void
elk_hash_set_expand(ElkHashSet *set)
{
    i8 const size_exp = set->size_exp;
    i8 const new_size_exp = size_exp + 1;

    size const handles_len = (size)(1 << size_exp);
    size const new_handles_len = (size)(1 << new_size_exp);

    ElkHashSetHandle *new_handles = elk_static_arena_nmalloc(set->arena, new_handles_len, ElkHashSetHandle);
    PanicIf(!new_handles);

    for (u32 i = 0; i < handles_len; i++) 
    {
        ElkHashSetHandle *handle = &set->handles[i];

        if (handle->value == NULL) { continue; } // Skip if it's empty

        // Find the position in the new table and update it.
        u64 const hash = handle->hash;
        u32 j = hash & 0xffffffff; // truncate
        while (true) 
        {
            j = elk_hash_lookup(hash, new_size_exp, j);
            ElkHashSetHandle *new_handle = &new_handles[j];

            if (!new_handle->value)
            {
                // empty - put it here. Don't need to check for room because we just expanded
                // the hash table of handles, and we're not copying anything new into storage,
                // it's already there!
                *new_handle = *handle;
                break;
            }
        }
    }

    set->handles = new_handles;
    set->size_exp = new_size_exp;

    return;
}

static inline void *
elk_hash_set_insert(ElkHashSet *set, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = set->hasher(value);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, set->size_exp, i);
        ElkHashSetHandle *handle = &set->handles[i];

        if (!handle->value)
        {
            // empty, insert here if room in the table of handles. Check for room first!
            if (elk_hash_table_large_enough(set->num_handles, set->size_exp))
            {

                *handle = (ElkHashSetHandle){.hash = hash, .value=value};
                set->num_handles += 1;

                return handle->value;
            }
            else 
            {
                // Grow the table so we have room
                elk_hash_set_expand(set);

                // Recurse because all the state needed by the *_lookup function was just crushed
                // by the expansion of the set.
                return elk_hash_set_insert(set, value);
            }
        }
        else if (handle->hash == hash && set->eq(handle->value, value)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

static inline void *
elk_hash_set_lookup(ElkHashSet *set, void *value)
{
    // Inspired by https://nullprogram.com/blog/2022/08/08
    // All code & writing on this blog is in the public domain.

    u64 const hash = set->hasher(value);
    u32 i = hash & 0xffffffff; // truncate
    while (true)
    {
        i = elk_hash_lookup(hash, set->size_exp, i);
        ElkHashSetHandle *handle = &set->handles[i];

        if (!handle->value)
        {
            return NULL;
        }
        else if (handle->hash == hash && set->eq(handle->value, value)) 
        {
            // found it!
            return handle->value;
        }
    }

    return NULL;
}

static inline size 
elk_hash_set_len(ElkHashSet *set)
{
    return set->num_handles;
}

static inline ElkHashSetIter
elk_hash_set_value_iter(ElkHashSet *set)
{
    return 0;
}

static inline void *
elk_hash_set_value_iter_next(ElkHashSet *set, ElkHashSetIter *iter)
{
    size const max_iter = (size)(1 << set->size_exp);
    void *next_value = NULL;
    if(*iter >= max_iter) { return next_value; }

    do
    {
        next_value = set->handles[*iter].value;
        *iter += 1;

    } while(next_value == NULL && *iter < max_iter);

    return next_value;
}

#define ELK_I8_FLIP(x) ((x) ^ UINT8_C(0x80))
#define ELK_I8_FLIP_BACK(x) ELK_I8_FLIP(x)

#define ELK_I16_FLIP(x) ((x) ^ UINT16_C(0x8000))
#define ELK_I16_FLIP_BACK(x) ELK_I16_FLIP(x)

#define ELK_I32_FLIP(x) ((x) ^ UINT32_C(0x80000000))
#define ELK_I32_FLIP_BACK(x) ELK_I32_FLIP(x)

#define ELK_I64_FLIP(x) ((x) ^ UINT64_C(0x8000000000000000))
#define ELK_I64_FLIP_BACK(x) ELK_I64_FLIP(x)

#define ELK_F32_FLIP(x) ((x) ^ (-(i32)(((u32)(x)) >> 31) | UINT32_C(0x80000000)))
#define ELK_F32_FLIP_BACK(x) ((x) ^ (((((u32)(x)) >> 31) - 1) | UINT32_C(0x80000000)))

#define ELK_F64_FLIP(x) ((x) ^ (-(i64)(((u64)(x)) >> 63) | UINT64_C(0x8000000000000000)))
#define ELK_F64_FLIP_BACK(x) ((x) ^ (((((u64)(x)) >> 63) - 1) | UINT64_C(0x8000000000000000)))

static inline void
elk_radix_pre_sort_transform(void *buffer, size num, size offset, size stride, ElkRadixSortByType sort_type)
{
    Assert(
           sort_type == ELK_RADIX_SORT_UINT64 || sort_type == ELK_RADIX_SORT_INT64 || sort_type == ELK_RADIX_SORT_F64
        || sort_type == ELK_RADIX_SORT_UINT32 || sort_type == ELK_RADIX_SORT_INT32 || sort_type == ELK_RADIX_SORT_F32
        || sort_type == ELK_RADIX_SORT_UINT16 || sort_type == ELK_RADIX_SORT_INT16
        || sort_type == ELK_RADIX_SORT_UINT8  || sort_type == ELK_RADIX_SORT_INT8
    );

    for(size i = 0; i < num; ++i)
    {
        switch(sort_type)
        {
            case ELK_RADIX_SORT_F64:
            {
                /* Flip bits so doubles sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F64_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT64:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I64_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_F32:
            {
                /* Flip bits so doubles sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F32_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT32:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I32_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT16:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u16 *v = (u16 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I16_FLIP(*v);
            } break;

            case ELK_RADIX_SORT_INT8:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u8 *v = (u8 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I8_FLIP(*v);
            } break;

            /* Fall through without doing anything. */
            case ELK_RADIX_SORT_UINT64:
            case ELK_RADIX_SORT_UINT32:
            case ELK_RADIX_SORT_UINT16:
            case ELK_RADIX_SORT_UINT8: {}
        }
    }
}

static inline void
elk_radix_post_sort_transform(void *buffer, size num, size offset, size stride, ElkRadixSortByType sort_type)
{
    Assert(
           sort_type == ELK_RADIX_SORT_UINT64 || sort_type == ELK_RADIX_SORT_INT64 || sort_type == ELK_RADIX_SORT_F64
        || sort_type == ELK_RADIX_SORT_UINT32 || sort_type == ELK_RADIX_SORT_INT32 || sort_type == ELK_RADIX_SORT_F32
        || sort_type == ELK_RADIX_SORT_UINT16 || sort_type == ELK_RADIX_SORT_INT16
        || sort_type == ELK_RADIX_SORT_UINT8  || sort_type == ELK_RADIX_SORT_INT8
    );

    for(size i = 0; i < num; ++i)
    {
        switch(sort_type)
        {
            case ELK_RADIX_SORT_F64:
            {
                /* Flip bits so doubles sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F64_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT64:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u64 *v = (u64 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I64_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_F32:
            {
                /* Flip bits so doubles sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_F32_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT32:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u32 *v = (u32 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I32_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT16:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u16 *v = (u16 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I16_FLIP_BACK(*v);
            } break;

            case ELK_RADIX_SORT_INT8:
            {
                /* Flip bits so two's complement signed integers sort correctly. */
                u8 *v = (u8 *)((byte *)buffer + offset + i * stride);
                *v = ELK_I8_FLIP_BACK(*v);
            } break;

            /* Fall through with no-op */
            case ELK_RADIX_SORT_UINT64:
            case ELK_RADIX_SORT_UINT32:
            case ELK_RADIX_SORT_UINT16:
            case ELK_RADIX_SORT_UINT8: {}
        }
    }
}

static inline void
elk_radix_sort_8(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256] = {0};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    for(size i = 0; i < num; ++i)
    {
        u8 b0 = *(u8 *)position;
        counts[b0] += 1;
        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i] += counts[i - 1];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i] += counts[i + 1];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    for(size i = num - 1; i >= 0; --i)
    {
        void *val_src = (byte *)source + i * stride;
        u8 cnts_idx = *((u8 *)val_src + offset);
        void *val_dest = (byte *)dest + (--counts[cnts_idx]) * stride;

        memcpy(val_dest, val_src, stride);
    }

    /* Copy back to the original buffer. */
    memcpy(buffer, scratch, stride * num);
}

static inline void
elk_radix_sort_16(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256][2] = {0};
    i32 skips[2] = {1, 1};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    u16 val = *(u16 *)position;
    u8 b0 = UINT16_C(0xFF) & (val >>  0);
    u8 b1 = UINT16_C(0xFF) & (val >>  8);

    counts[b0][0] += 1;
    counts[b1][1] += 1;

    position += stride;

    u8 initial_values[2] = {b0, b1};

    for(size i = 1; i < num; ++i)
    {
        val = *(u16 *)position;
        b0 = UINT16_C(0xFF) & (val >>  0);
        b1 = UINT16_C(0xFF) & (val >>  8);

        counts[b0][0] += 1;
        counts[b1][1] += 1;

        skips[0] &= initial_values[0] == b0;
        skips[1] &= initial_values[1] == b1;

        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i][0] += counts[i - 1][0];
            counts[i][1] += counts[i - 1][1];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i][0] += counts[i + 1][0];
            counts[i][1] += counts[i + 1][1];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    size num_skips = 0;
    for(size b = 0; b < 2; ++b)
    {
        if(!skips[b])
        {
            for(size i = num - 1; i >= 0; --i)
            {
                void *val_src = (byte *)source + i * stride;
                u16 val = *(u16 *)((byte *)val_src + offset);
                u8 cnts_idx = UINT16_C(0xFF) & (val >> (b * 8));
                void *val_dest = (byte *)dest + (--counts[cnts_idx][b]) * stride;

                memcpy(val_dest, val_src, stride);
            }

            /* Flip the source & destination buffers. */
            dest = dest == scratch ? buffer : scratch;
            source = source == scratch ? buffer : scratch;
        }
        else
        {
            num_skips++;
        }
    }

    /* Swap back into the original buffer. */
    if(num_skips % 2)
    {
        memcpy(buffer, scratch, num * stride);
    }
}

static inline void
elk_radix_sort_32(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256][4] = {0};
    i32 skips[4] = {1, 1, 1, 1};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    u32 val = *(u32 *)position;
    u8 b0 = UINT32_C(0xFF) & (val >>  0);
    u8 b1 = UINT32_C(0xFF) & (val >>  8);
    u8 b2 = UINT32_C(0xFF) & (val >> 16);
    u8 b3 = UINT32_C(0xFF) & (val >> 24);

    counts[b0][0] += 1;
    counts[b1][1] += 1;
    counts[b2][2] += 1;
    counts[b3][3] += 1;

    position += stride;

    u8 initial_values[4] = {b0, b1, b2, b3};

    for(size i = 1; i < num; ++i)
    {
        val = *(u32 *)position;
        b0 = UINT32_C(0xFF) & (val >>  0);
        b1 = UINT32_C(0xFF) & (val >>  8);
        b2 = UINT32_C(0xFF) & (val >> 16);
        b3 = UINT32_C(0xFF) & (val >> 24);

        counts[b0][0] += 1;
        counts[b1][1] += 1;
        counts[b2][2] += 1;
        counts[b3][3] += 1;

        skips[0] &= initial_values[0] == b0;
        skips[1] &= initial_values[1] == b1;
        skips[2] &= initial_values[2] == b2;
        skips[3] &= initial_values[3] == b3;

        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i][0] += counts[i - 1][0];
            counts[i][1] += counts[i - 1][1];
            counts[i][2] += counts[i - 1][2];
            counts[i][3] += counts[i - 1][3];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i][0] += counts[i + 1][0];
            counts[i][1] += counts[i + 1][1];
            counts[i][2] += counts[i + 1][2];
            counts[i][3] += counts[i + 1][3];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    size num_skips = 0;
    for(size b = 0; b < 4; ++b)
    {
        if(!skips[b])
        {
            for(size i = num - 1; i >= 0; --i)
            {
                void *val_src = (byte *)source + i * stride;
                u32 val = *(u32 *)((byte *)val_src + offset);
                u8 cnts_idx = UINT32_C(0xFF) & (val >> (b * 8));
                void *val_dest = (byte *)dest + (--counts[cnts_idx][b]) * stride;

                memcpy(val_dest, val_src, stride);
            }

            /* Flip the source & destination buffers. */
            dest = dest == scratch ? buffer : scratch;
            source = source == scratch ? buffer : scratch;
        }
        else
        {
            num_skips++;
        }
    }

    /* Swap back into the original buffer. */
    if(num_skips % 2)
    {
        memcpy(buffer, scratch, num * stride);
    }
}

static inline void
elk_radix_sort_64(void *buffer, size num, size offset, size stride, void *scratch, ElkSortOrder order)
{
    i64 counts[256][8] = {0};
    i32 skips[8] = {1, 1, 1, 1, 1, 1, 1, 1};

    /* Build the histograms. */
    byte *position = (byte *)buffer + offset;

    u64 val = *(u64 *)position;
    u8 b0 = UINT64_C(0xFF) & (val >>  0);
    u8 b1 = UINT64_C(0xFF) & (val >>  8);
    u8 b2 = UINT64_C(0xFF) & (val >> 16);
    u8 b3 = UINT64_C(0xFF) & (val >> 24);
    u8 b4 = UINT64_C(0xFF) & (val >> 32);
    u8 b5 = UINT64_C(0xFF) & (val >> 40);
    u8 b6 = UINT64_C(0xFF) & (val >> 48);
    u8 b7 = UINT64_C(0xFF) & (val >> 56);

    counts[b0][0] += 1;
    counts[b1][1] += 1;
    counts[b2][2] += 1;
    counts[b3][3] += 1;
    counts[b4][4] += 1;
    counts[b5][5] += 1;
    counts[b6][6] += 1;
    counts[b7][7] += 1;

    position += stride;

    u8 initial_values[8] = {b0, b1, b2, b3, b4, b5, b6, b7};

    for(size i = 1; i < num; ++i)
    {
        val = *(u64 *)position;
        b0 = UINT64_C(0xFF) & (val >>  0);
        b1 = UINT64_C(0xFF) & (val >>  8);
        b2 = UINT64_C(0xFF) & (val >> 16);
        b3 = UINT64_C(0xFF) & (val >> 24);
        b4 = UINT64_C(0xFF) & (val >> 32);
        b5 = UINT64_C(0xFF) & (val >> 40);
        b6 = UINT64_C(0xFF) & (val >> 48);
        b7 = UINT64_C(0xFF) & (val >> 56);

        counts[b0][0] += 1;
        counts[b1][1] += 1;
        counts[b2][2] += 1;
        counts[b3][3] += 1;
        counts[b4][4] += 1;
        counts[b5][5] += 1;
        counts[b6][6] += 1;
        counts[b7][7] += 1;

        skips[0] &= initial_values[0] == b0;
        skips[1] &= initial_values[1] == b1;
        skips[2] &= initial_values[2] == b2;
        skips[3] &= initial_values[3] == b3;
        skips[4] &= initial_values[4] == b4;
        skips[5] &= initial_values[5] == b5;
        skips[6] &= initial_values[6] == b6;
        skips[7] &= initial_values[7] == b7;

        position += stride;
    }

    /* Sum the histograms. */
    if(order == ELK_SORT_ASCENDING)
    {
        for(size i = 1; i < 256; ++i)
        {
            counts[i][0] += counts[i - 1][0];
            counts[i][1] += counts[i - 1][1];
            counts[i][2] += counts[i - 1][2];
            counts[i][3] += counts[i - 1][3];
            counts[i][4] += counts[i - 1][4];
            counts[i][5] += counts[i - 1][5];
            counts[i][6] += counts[i - 1][6];
            counts[i][7] += counts[i - 1][7];
        }
    }
    else
    {
        for(size i = 254; i >= 0; --i)
        {
            counts[i][0] += counts[i + 1][0];
            counts[i][1] += counts[i + 1][1];
            counts[i][2] += counts[i + 1][2];
            counts[i][3] += counts[i + 1][3];
            counts[i][4] += counts[i + 1][4];
            counts[i][5] += counts[i + 1][5];
            counts[i][6] += counts[i + 1][6];
            counts[i][7] += counts[i + 1][7];
        }
    }

    /* Build the output array. */
    void *dest = scratch;
    void *source = buffer;
    size num_skips = 0;
    for(size b = 0; b < 8; ++b)
    {
        if(!skips[b])
        {
            for(size i = num - 1; i >= 0; --i)
            {
                void *val_src = (byte *)source + i * stride;
                u64 val = *(u64 *)((byte *)val_src + offset);
                u8 cnts_idx = UINT64_C(0xFF) & (val >> (b * 8));
                void *val_dest = (byte *)dest + (--counts[cnts_idx][b]) * stride;

                memcpy(val_dest, val_src, stride);
            }

            /* Flip the source & destination buffers. */
            dest = dest == scratch ? buffer : scratch;
            source = source == scratch ? buffer : scratch;
        }
        else
        {
            num_skips++;
        }
    }

    /* Swap back into the original buffer. */
    if(num_skips % 2)
    {
        memcpy(buffer, scratch, num * stride);
    }
}

static inline void
elk_radix_sort(void *buffer, size num, size offset, size stride, void *scratch, ElkRadixSortByType sort_type, ElkSortOrder order)
{
    elk_radix_pre_sort_transform(buffer, num, offset, stride, sort_type);

    switch(sort_type)
    {
        case ELK_RADIX_SORT_UINT64:
        case ELK_RADIX_SORT_INT64:
        case ELK_RADIX_SORT_F64:
        {
            elk_radix_sort_64(buffer, num, offset, stride, scratch, order);
        } break;

        case ELK_RADIX_SORT_UINT32:
        case ELK_RADIX_SORT_INT32:
        case ELK_RADIX_SORT_F32:
        {
            elk_radix_sort_32(buffer, num, offset, stride, scratch, order);
        } break;

        case ELK_RADIX_SORT_UINT16:
        case ELK_RADIX_SORT_INT16:
        {
            elk_radix_sort_16(buffer, num, offset, stride, scratch, order);
        } break;

        case ELK_RADIX_SORT_UINT8:
        case ELK_RADIX_SORT_INT8:
        {
            elk_radix_sort_8(buffer, num, offset, stride, scratch, order);
        } break;

        default: Panic();
    }
    
    elk_radix_post_sort_transform(buffer, num, offset, stride, sort_type);
}

#endif

