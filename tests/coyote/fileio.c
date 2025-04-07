#include "test.h"

/*--------------------------------------------------------------------------------------------------------------------------
 *
 *                                                   Tests for File IO
 *
 *-------------------------------------------------------------------------------------------------------------------------*/
static void
test_file_size(void)
{
    // Create a path to the file.
    char path_buf[1024] = {0};
    b32 success = coy_path_append(sizeof(path_buf), path_buf, test_data_dir);
    Assert(success);
    success = coy_path_append(sizeof(path_buf), path_buf, "README.md");
    Assert(success);

    size file_size = coy_file_size(path_buf);
    Assert(file_size == 100);

    // Now try accessing an invalid file!
    memset(path_buf, 0, sizeof(path_buf));
    success = coy_path_append(sizeof(path_buf), path_buf, test_data_dir);
    Assert(success);
    success = coy_path_append(sizeof(path_buf), path_buf, "I_DO_NOT_EXIST.md");
    Assert(success);

    file_size = coy_file_size(path_buf);
    Assert(file_size == -1);

    return;
}

static void
test_file_create_write_append_open_read_close(void)
{
    // Create a path to the file.
    char path_buf[1024] = {0};
    b32 success = coy_path_append(sizeof(path_buf), path_buf, test_data_dir);
    Assert(success);
    success = coy_path_append(sizeof(path_buf), path_buf, "write_append_read_test.txt");
    Assert(success);

    // Create the file and write to it.
    CoyFileWriter write_test = coy_file_create(path_buf);
    Assert(write_test.valid);

    char const hello_str[] = "Hello File.\n";
    size num_bytes_written = coy_file_write(&write_test, sizeof(hello_str) - 1, hello_str);
    Assert(num_bytes_written == sizeof(hello_str) - 1);

    coy_file_writer_close(&write_test);
    Assert(!write_test.valid);

    // Open the file for appending and write to it.
    CoyFileWriter append_test = coy_file_append(path_buf);
    Assert(append_test.valid);

    char const hello_again_str[] = "Hello Again File.\n";
    num_bytes_written = coy_file_write(&append_test, sizeof(hello_again_str) - 1, hello_again_str);
    Assert(num_bytes_written == sizeof(hello_again_str) - 1);

    coy_file_writer_close(&append_test);
    Assert(!append_test.valid);

    // Open the file and read from it.
    CoyFileReader read_test = coy_file_open_read(path_buf);
    Assert(read_test.valid);
    char read_str[40] = {0};
    size num_bytes_read = coy_file_read(&read_test, sizeof(read_str), read_str);
    Assert(num_bytes_read == 30);
    char const should_have_read_str[] = "Hello File.\nHello Again File.\n";
    for(int i = 0; i < num_bytes_read; ++i)
    {
	  Assert(should_have_read_str[i] == read_str[i]);
    }

    coy_file_reader_close(&read_test);
    Assert(!read_test.valid);

    return;
}

static void
test_memmap_read(void)
{
    // Create a path to the file.
    char path_buf[1024] = {0};
    b32 success = coy_path_append(sizeof(path_buf), path_buf, test_data_dir);
    Assert(success);
    success = coy_path_append(sizeof(path_buf), path_buf, "README.md");
    Assert(success);

    CoyMemMappedFile mmf = coy_memmap_read_only(path_buf);
    Assert(mmf.valid);

    char const test_str[] =
	  "This directory should remain empty other than this file. It is used for writing test results into.\n\n";

    // Cast the memory to the correct type.
    char const *mmstr = (char const *)mmf.data;

    for(int i = 0; i < mmf.size_in_bytes; ++i)
    {
	  Assert(test_str[i] == mmstr[i]); 
    }

    coy_memmap_close(&mmf);
    Assert(!mmf.valid);

    return;
}

static void
test_file_slurp(void)
{
    // Create a path to the file.
    char path_buf[1024] = {0};
    b32 success = coy_path_append(sizeof(path_buf), path_buf, test_data_dir);
    Assert(success);
    success = coy_path_append(sizeof(path_buf), path_buf, "README.md");
    Assert(success);

    char file_contents[1024] = {0};
    size str_sz = coy_file_slurp(path_buf, sizeof(file_contents), file_contents);
    Assert(str_sz > 1);

    char const test_str[] =
	  "This directory should remain empty other than this file. It is used for writing test results into.\n\n";
    Assert(str_sz == sizeof(test_str) - 1); // -1 because the literal adds a \0 to the end.

    for(int i = 0; i < str_sz; ++i)
    {
	  Assert(test_str[i] == file_contents[i]); 
    }

    return;
}

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                    All file IO tests
 *-------------------------------------------------------------------------------------------------------------------------*/
void
coyote_file_tests(void)
{
    test_file_size();
    test_file_create_write_append_open_read_close();
    test_memmap_read();
    test_file_slurp();
}

