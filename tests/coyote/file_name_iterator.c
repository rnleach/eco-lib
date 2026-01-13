#include "test.h"

int strcmp(const char *s1, const char *s2);
/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                               Tests for FileNameIterator
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static void
test_file_name_iterator(void)
{
    char const *src_files[] = 
    {
        "coyote.h", "coyote_win32.h", "coyote_apple_osx.h", "coyote_linux.h", "coyote_linux_apple_common.h",
        "magpie.h", "magpie_win32.h", "magpie_apple_osx.h", "magpie_linux.h", "magpie_emscripten.h",
        "elk.h", "packrat.h"
    };

    CoyFileNameIter iter_ = coy_file_name_iterator_open("src", NULL);
    CoyFileNameIter *iter = &iter_;

    i32 count = 0;
    const char *fname = coy_file_name_iterator_next(iter);
    while(fname)
    {
        b32 found = false;
        char const *f = 0;
        for(int i = 0; i < sizeof(src_files) / sizeof(src_files[0]); ++i)
        {
            f = src_files[i];
            if(strcmp(fname, f) == 0)
            {
                found = true;
                break;
            }
        }

        if(!found)
        {
            fprintf(stderr, "\n\nCould not find: '%s'\n\n", fname);
        }

        Assert(found);
        count += 1;

        fname = coy_file_name_iterator_next(iter);
    }

    Assert(count == sizeof(src_files) / sizeof(src_files[0]));

    coy_file_name_iterator_close(iter);
    Assert(iter->os_handle == 0 && iter->file_extension == NULL && iter->valid == false);
}

static void
test_file_name_iterator_filtering(void)
{
    char const *readme = "README.md"; 

    CoyFileNameIter iter_ = coy_file_name_iterator_open(".", "md");
    CoyFileNameIter *iter = &iter_;

    const char *fname = coy_file_name_iterator_next(iter);
    i32 count = 0;
    while(fname)
    {
        Assert(strcmp(fname, readme) == 0);

        count += 1;
        fname = coy_file_name_iterator_next(iter);
    }

    Assert(count == 1);

    coy_file_name_iterator_close(iter);
    Assert(iter->os_handle == 0 && iter->file_extension == NULL && iter->valid == false);
}

static void
test_null_term_strings_equal(void)
{
    char *left[5] = {"test", "test1", "test2", "test3", "test4"};
    char *right[5] = {"test", "test1", "test2", "test3", "test4"};

    for(int i = 0; i < 5; ++i)
    {
        Assert(coy_null_term_strings_equal(left[i], right[i]));

        for(int j = 0; j < 5; ++j)
        {
            if(i != j)
            {
                Assert(!coy_null_term_strings_equal(left[i], left[j]));
                Assert(!coy_null_term_strings_equal(right[i], right[j]));
            }
            else
            {
                Assert(coy_null_term_strings_equal(left[i], left[j]));
                Assert(coy_null_term_strings_equal(right[i], right[j]));
            }
        }
    }
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    All file IO tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
coyote_file_name_iterator_tests(void)
{
    test_null_term_strings_equal();
    test_file_name_iterator();
    test_file_name_iterator_filtering();
}

