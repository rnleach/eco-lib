#include "test.h"

#include <string.h>

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *                                                 Test String Interner
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

static char *some_strings[] = 
{
    "vegemite", "cantaloupe",    "poutine",    "cottonwood trees", "x",
    "y",        "peanut butter", "jelly time", "strawberries",     "and cream",
    "raining",  "cats and dogs", "sushi",      "date night",       "sour",
    "beer!",    "scotch",        "yes please", "raspberries",      "snack time",
};

static void
test_string_interner(void)
{
    size const NUM_TEST_STRINGS = sizeof(some_strings) / sizeof(some_strings[0]);

    ElkStr strs[sizeof(some_strings) / sizeof(some_strings[0])] = {0};

    byte buffer[ECO_KB(2)] = {0};
    MagAllocator arena_i = mag_allocator_static_arena_create(sizeof(buffer), buffer);
    MagAllocator *arena = &arena_i;

    PakStringInterner interner = pak_string_interner_create(3, arena);
    PakStringInterner *internerp = &interner;

    // Fill the interner with strings!
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings[i];
        strs[i] = pak_string_interner_intern_cstring(internerp, str);
    }

    // Now see if we get the right ones back out!
    for (size i = 0; i < NUM_TEST_STRINGS; ++i) 
    {
        char *str = some_strings[i];
        ElkStr interned_str = pak_string_interner_intern_cstring(internerp, str);
        Assert(strcmp(str, interned_str.start) == 0);
        Assert(strcmp(str, strs[i].start) == 0);
        Assert(interned_str.len == strs[i].len);
        Assert(interned_str.start == strs[i].start);
    }

    pak_string_interner_destroy(internerp);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       All tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
pak_string_interner_tests()
{
    test_string_interner();
}
