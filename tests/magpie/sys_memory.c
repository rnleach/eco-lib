#include "../test.h"

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                    Tests for Memory
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static void
test_allocate_free(void)
{
    MagMemoryBlock mem = mag_sys_memory_allocate(1);
    Assert(mem.valid);
    mag_sys_memory_free(&mem);
    Assert(!mem.valid);

    mem = mag_sys_memory_allocate(ECO_KiB(1));
    Assert(mem.valid);

    u8 *byte = mem.mem;
    for(size i = 0; i < mem.size; ++i) { byte[i] = (i & 0xFF); }
    for(size i = 0; i < mem.size; ++i) { Assert(byte[i] == (i & 0xFF)); }

    mag_sys_memory_free(&mem);
    Assert(!mem.valid);

    mem = mag_sys_memory_allocate(ECO_MiB(1));
    Assert(mem.valid);

    byte = mem.mem;
    for(size i = 0; i < mem.size; ++i) { byte[i] = (i & 0xFF); }
    for(size i = 0; i < mem.size; ++i) { Assert(byte[i] == (i & 0xFF)); }

    mag_sys_memory_free(&mem);
    Assert(!mem.valid);

    mem = mag_sys_memory_allocate(ECO_GiB(1));
    Assert(mem.valid);

    byte = mem.mem;
    for(size i = 0; i < mem.size; ++i) { byte[i] = (i & 0xFF); }
    for(size i = 0; i < mem.size; ++i) { Assert(byte[i] == (i & 0xFF)); }

    mag_sys_memory_free(&mem);
    Assert(!mem.valid);
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                     All file Memory
 *-------------------------------------------------------------------------------------------------------------------------*/
void
magpie_sys_memory_tests(void)
{
    test_allocate_free();
}

