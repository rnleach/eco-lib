#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                 Test String Interner
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

static char *some_strings_ht[] = 
{
    "vegemite", "cantaloupe",    "poutine",    "cottonwood trees", "x",
    "y",        "peanut butter", "jelly time", "strawberries",     "and cream",
    "raining",  "cats and dogs", "sushi",      "date night",       "sour",
    "beer!",    "scotch",        "yes please", "raspberries",      "snack time",
};

static void
test_elk_str_table(void)
{
    size const NUM_TEST_STRINGS = sizeof(some_strings_ht) / sizeof(some_strings_ht[0]);

    ElkStr strs[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};
    i64 values[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};
    i64 values2[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};

    byte buffer[ECO_KB(2)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    PakStrMap map_ = pak_str_map_create(2, arena); // Use a crazy small size_exp to force it to grow, this IS a test!
    PakStrMap *map = &map_;
    Assert(map);

    // Fill the map
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings_ht[i];
        strs[i] = elk_str_from_cstring(str);
        values[i] = i;
        values2[i] = i; // We'll use this later!
        
        i64 *vptr = pak_str_map_insert(map, strs[i], &values[i]);
        Assert(vptr == &values[i]);
    }

    // Now see if we get the right ones back out!
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        i64 *vptr = pak_str_map_lookup(map, strs[i]);
        Assert(vptr == &values[i]);
        Assert(*vptr == i);
    }

    // Fill the map with NEW values
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        i64 *vptr = pak_str_map_insert(map, strs[i], &values2[i]);
        Assert(vptr == &values[i]); // should get the old value pointer back
        Assert(vptr != &values2[i]); // should not point at the pointer we passed in, because we replaced a value
    }

    pak_str_map_destroy(map);
}

static void
test_elk_str_key_iterator(void)
{
    size const NUM_TEST_STRINGS = sizeof(some_strings_ht) / sizeof(some_strings_ht[0]);

    ElkStr strs[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};
    i64 values[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};

    byte buffer[ECO_KB(2)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    PakStrMap map_ = pak_str_map_create(2, arena); // Use a crazy small size_exp to force it to grow, this IS a test!
    PakStrMap *map = &map_;
    Assert(map);

    // Fill the map
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings_ht[i];
        strs[i] = elk_str_from_cstring(str);
        values[i] = i;
        
        i64 *vptr = pak_str_map_insert(map, strs[i], &values[i]);
        Assert(vptr == &values[i]);
    }

    PakStrMapKeyIter iter = pak_str_map_key_iter(map);

    ElkStr key = {0};
    i32 key_count = 0;
    do
    {
        key = pak_str_map_key_iter_next(map, &iter);
        if(key.start) {
            key_count  += 1;
            //printf("%p %s\n", key.start, key.start); 
        }

    } while(key.start);

    Assert(key_count  == NUM_TEST_STRINGS);

    pak_str_map_destroy(map);
}

static void
test_elk_str_handle_iterator(void)
{
    size const NUM_TEST_STRINGS = sizeof(some_strings_ht) / sizeof(some_strings_ht[0]);

    ElkStr strs[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};
    i64 values[sizeof(some_strings_ht) / sizeof(some_strings_ht[0])] = {0};

    byte buffer[ECO_KB(2)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    PakStrMap map_ = pak_str_map_create(2, arena); // Use a crazy small size_exp to force it to grow, this IS a test!
    PakStrMap *map = &map_;
    Assert(map);

    // Fill the map
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings_ht[i];
        strs[i] = elk_str_from_cstring(str);
        values[i] = i;
        
        i64 *vptr = pak_str_map_insert(map, strs[i], &values[i]);
        Assert(vptr == &values[i]);
    }

    PakStrMapHandleIter iter = pak_str_map_handle_iter(map);

    PakStrMapHandle handle = {0};
    i32 key_count = 0;
    do
    {
        handle = pak_str_map_handle_iter_next(map, &iter);
        if(handle.key.start) {
            key_count  += 1;
            //printf("%p %s\n", handle.key.start, handle.key.start); 
        }

    } while(handle.key.start);

    Assert(key_count  == NUM_TEST_STRINGS);

    pak_str_map_destroy(map);
}

static u64 
id_hash(void const *value)
{
    return *(i64 const*)value;
}

static b32
int64_eq(void const *left, void const *right)
{
    i64 const *ileft = left;
    i64 const *iright = right;
    return *ileft == *iright;
}

#define NUM_KEYS 20
static void
test_pak_hash_table(void)
{
    ElkTime keys[NUM_KEYS] = {0};
    i64 values[NUM_KEYS] = {0};
    i64 values2[NUM_KEYS] = {0};

    // Initialize the keys and values
    for(i32 i = 0; i < NUM_KEYS; ++i)
    {
        keys[i] = elk_time_from_ymd_and_hms(2000 + i, 1 + i % 12, 1 + i, i, i, i);
        values[i] = i;
        values2[i] = i;
    }

    // Set up memory
    byte buffer[ECO_KB(2)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    // Fill the hashmap
    PakHashMap map_ = pak_hash_map_create(2, id_hash, int64_eq, arena);
    PakHashMap *map = &map_;
    for(i32 i = 0; i < NUM_KEYS; ++i)
    {
        i64 *vptr = pak_hash_map_insert(map, &keys[i], &values[i]);
        Assert(vptr == &values[i]);
    }

    // check the values
    for(i32 i = 0; i < NUM_KEYS; ++i)
    {
        i64 *vptr = pak_hash_map_lookup(map, &keys[i]);
        Assert(vptr == &values[i]);
    }

    // Fill the hashmap with something else
    for(i32 i = 0; i < NUM_KEYS; ++i)
    {
        i64 *vptr = pak_hash_map_insert(map, &keys[i], &values2[i]);
        Assert(vptr == &values[i]);
        Assert(vptr != &values2[i]);
    }

    // Clean up
    pak_hash_map_destroy(map);
}

static void
test_pak_hash_key_iterator(void)
{
    ElkTime keys[NUM_KEYS] = {0};
    i64 values[NUM_KEYS] = {0};

    // Initialize the keys and values
    for(i32 i = 0; i < NUM_KEYS; ++i)
    {
        keys[i] = elk_time_from_ymd_and_hms(2000 + i, 1 + i % 12, 1 + i, i, i, i);
        values[i] = i;
    }

    // Set up memory
    byte buffer[ECO_KB(2)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    // Fill the hashmap
    PakHashMap map_ = pak_hash_map_create(2, id_hash, int64_eq, arena);
    PakHashMap *map = &map_;
    for(i32 i = 0; i < NUM_KEYS; ++i)
    {
        i64 *vptr = pak_hash_map_insert(map, &keys[i], &values[i]);
        Assert(vptr == &values[i]);
    }

    PakHashMapKeyIter iter = pak_hash_map_key_iter(map);
    
    ElkTime *key = NULL;
    i32 num_keys = 0;
    do
    {
        key = pak_hash_map_key_iter_next(map, &iter);
        if(key) {
            num_keys += 1;
            //printf("%p %lld\n", key, *key); 
        }

    } while(key);

    Assert(num_keys == NUM_KEYS);
    
    // Clean up
    pak_hash_map_destroy(map);
}
#undef NUM_KEYS

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
pak_hash_table_tests()
{
    test_elk_str_table();
    test_elk_str_key_iterator();
    test_elk_str_handle_iterator();
    test_pak_hash_table();
    test_pak_hash_key_iterator();
}
