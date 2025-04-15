#ifndef _ECO_LIB_TESTS_
#define _ECO_LIB_TESTS_
//
// For testing, ensure we have some debugging tools activated.
//

// We must have asserts working for the tests to work.
#ifdef NDEBUG
#    undef NDEBUG
#endif

#define _ELK_TRACK_MEM_USAGE

#include <stdio.h>
#include "../build/elk.h"
#include "../build/magpie.h"
#include "../build/coyote.h"
#include "../build/packrat.h"

void elk_time_tests(void);
void elk_date_tests(void);
void elk_fnv1a_tests(void);
void elk_str_tests(void);
void elk_parse_tests(void);
void elk_arena_tests(void);
void elk_pool_tests(void);
void elk_queue_ledger_tests(void);
void elk_array_ledger_tests(void);
void elk_csv_tests(void);

void magpie_sys_memory_tests(void);

void coyote_time_tests(void);
void coyote_file_tests(void);
void coyote_file_name_iterator_tests(void);
void coyote_terminal_tests(void);
void coyote_threads_tests(void);

static char const *test_data_dir = "tmp_output";

void pak_string_interner_tests(void);
void pak_hash_table_tests(void);
void pak_hash_set_tests(void);
void pak_sort_tests(void);

#endif
