#include "test.h"

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                 Test String Interner
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

static char *some_strings_hash_set_tests[] = 
{
    "vegemite", "cantaloupe",    "poutine",    "cottonwood trees", "x",
    "y",        "peanut butter", "jelly time", "strawberries",     "and cream",
    "raining",  "cats and dogs", "sushi",      "date night",       "sour",
    "beer!",    "scotch",        "yes please", "raspberries",      "snack time",
};

static u64 simple_str_hash(void const *str)
{
    return elk_fnv1a_hash_str(*(ElkStr *)str);
}

static b32 str_eq(void const *left, void const *right)
{
    return elk_str_eq(*(ElkStr *)left, *(ElkStr *)right);
}

#define NUM_SET_TEST_STRINGS  (sizeof(some_strings_hash_set_tests) / sizeof(some_strings_hash_set_tests[0]))

static void
test_pak_hash_set(void)
{
    ElkStr strs[NUM_SET_TEST_STRINGS] = {0};
    for(i32 i = 0; i < NUM_SET_TEST_STRINGS; ++i)
    {
        strs[i] = elk_str_from_cstring(some_strings_hash_set_tests[i]);
    }

    byte buffer[ECO_KB(1)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    PakHashSet set_ = pak_hash_set_create(2, simple_str_hash, str_eq, arena);
    PakHashSet *set = &set_;
    for(i32 i = 0; i < NUM_SET_TEST_STRINGS; ++i)
    {
        ElkStr *str = pak_hash_set_insert(set, &strs[i]);
        Assert(str == &strs[i]);
    }

    for(i32 i = 0; i < NUM_SET_TEST_STRINGS; ++i)
    {
        ElkStr *str = pak_hash_set_lookup(set, &strs[i]);
        Assert(str == &strs[i]);
    }

    Assert(pak_len(set) == NUM_SET_TEST_STRINGS);

    ElkStr not_in_set = elk_str_from_cstring("green beans");
    ElkStr *str = pak_hash_set_lookup(set, &not_in_set);
    Assert(str == NULL);

    pak_hash_set_destroy(set);
}

static void
test_pak_hash_set_iter(void)
{
    ElkStr strs[NUM_SET_TEST_STRINGS] = {0};
    for(i32 i = 0; i < NUM_SET_TEST_STRINGS; ++i)
    {
        strs[i] = elk_str_from_cstring(some_strings_hash_set_tests[i]);
    }

    byte buffer[ECO_KB(1)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    PakHashSet set_ = pak_hash_set_create(2, simple_str_hash, str_eq, arena);
    PakHashSet *set = &set_;
    for(i32 i = 0; i < NUM_SET_TEST_STRINGS; ++i)
    {
        ElkStr *str = pak_hash_set_insert(set, &strs[i]);
        Assert(str == &strs[i]);
    }

    PakHashSetIter iter = pak_hash_set_value_iter(set);
    ElkStr *next = pak_hash_set_value_iter_next(set, &iter);
    i32 found_count = 0;
    while(next)
    {
        b32 found = false;
        for(i32 i = 0; i < NUM_SET_TEST_STRINGS; ++i)
        {
            if(&strs[i] == next) 
            { 
                found_count += 1;
                found = true;
                break;
            }
        }

        Assert(found);

        next = pak_hash_set_value_iter_next(set, &iter);
    }
    Assert(found_count == NUM_SET_TEST_STRINGS);

    pak_hash_set_destroy(set);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
pak_hash_set_tests()
{
    test_pak_hash_set();
    test_pak_hash_set_iter();
}
