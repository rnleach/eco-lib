#include <stdlib.h>
#include <inttypes.h>

#include "test.h"
/*-------------------------------------------------------------------------------------------------
 *
 *                                       Main - Run the tests
 *
 *-----------------------------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
    fprintf(stderr, "\n\n***        Starting Tests.        ***\n\n");
    coy_profile_begin();

    /* Elk Tests --------------------------------------------------------------------------------*/
    fprintf(stderr, "elk tests..");
    CoyProfileAnchor ap = COY_START_PROFILE_BLOCK("elk tests");
    elk_time_tests();
    elk_fnv1a_tests();
    elk_str_tests();
    elk_parse_tests();
    elk_arena_tests();
    elk_pool_tests();
    elk_queue_ledger_tests();
    elk_array_ledger_tests();
    elk_csv_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    /* Magpie Tests -----------------------------------------------------------------------------*/
    fprintf(stderr, "magpie_sys_memory_tests()..");
    ap = COY_START_PROFILE_BLOCK("magpie sys_memory_tests");
    magpie_sys_memory_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    /* Coyote Tests -----------------------------------------------------------------------------*/
    fprintf(stderr, "coyote_time_tests()..");
    ap = COY_START_PROFILE_BLOCK("coyote time_tests");
    coyote_time_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    fprintf(stderr, "coyote_file_tests()..");
    ap = COY_START_PROFILE_BLOCK("coyote file_tests");
    coyote_file_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    fprintf(stderr, "coyote_file_name_iterator_tests()..");
    ap = COY_START_PROFILE_BLOCK("coyote file_name_iterator_tests");
    coyote_file_name_iterator_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    fprintf(stderr, "coyote_terminal_tests()..");
    ap = COY_START_PROFILE_BLOCK("coyote terminal_tests");
    coyote_terminal_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    fprintf(stderr, "coyote_threads_tests()..");
    ap = COY_START_PROFILE_BLOCK("coyote threads");
    coyote_threads_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    /* Packrat Tests ----------------------------------------------------------------------------*/
    fprintf(stderr, "packrat tests..");
    ap = COY_START_PROFILE_BLOCK("packrat tests");
    pak_string_interner_tests();
    pak_hash_table_tests();
    pak_hash_set_tests();
    pak_sort_tests();
    COY_END_PROFILE(ap);
    fprintf(stderr, ".complete.\n");

    /* Done with Tests --------------------------------------------------------------------------*/
    coy_profile_end();
    fprintf(stderr, "\n\n*** Tests completed successfully. ***\n\n");

#if COY_PROFILE
    printf("Total Runtime = %.3lf seconds at a frequency of %"PRIu64"\n",
            coy_global_profiler.total_elapsed, coy_global_profiler.freq);

    for(i32 i = 0; i < COY_PROFILE_NUM_BLOCKS; ++i)
    {
        CoyBlockProfiler *block = &coy_global_profiler.blocks[i];
        if(block->hit_count)
        {
            printf("%-32s Hits: %3"PRIu64" Exclusive: %6.2lf%%", block->label, block->hit_count, block->exclusive_pct);
            
            if(block->inclusive_pct != block->exclusive_pct)
            {
                printf(" Inclusive: %6.2lf%%\n", block->inclusive_pct);
            }
            else
            {
                printf("\n");
            }
        }
    }
#endif
    return EXIT_SUCCESS;
}


#include "elk/arena.c"
#include "elk/array_ledger.c"
#include "elk/csv.c"
#include "elk/fnv1a.c"
#include "elk/parse.c"
#include "elk/pool.c"
#include "elk/queue_ledger.c"
#include "elk/str.c"
#include "elk/time.c"

#include "magpie/sys_memory.c"

#include "coyote/fileio.c"
#include "coyote/file_name_iterator.c"
#include "coyote/terminal.c"
#include "coyote/threads.c"
#include "coyote/time.c"

#include "packrat/hash_set.c"
#include "packrat/hash_tables.c"
#include "packrat/sort.c"
#include "packrat/string_interner.c"

