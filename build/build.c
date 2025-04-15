/* This program combines all the header files together into the final product, a single header library. */
#include <stdio.h>

#include "../src/coyote.h"

void *memcpy(void *dest, const void *src, size_t n);
int memcmp (const void * ptr1, const void * ptr2, size_t num);
void *memchr(const void *, int, size_t);

static inline b32
str_eq(char const *left, char const *right, size len)
{
    return 0 == memcmp(left, right, len);
}

static b32
load_file(char const *fname, size buf_size, byte *buffer, size *read)
{
    *read = coy_file_slurp_internal(fname, buf_size, buffer);
    if(*read < 0) { return false; }
    return true;
}

static void
insert_buffer(size out_size, size *out_idx, char *out, size in_size, char *in)
{
    Assert(*out_idx + in_size < out_size);

    char *start = &out[*out_idx];
    memcpy(start, in, in_size);
    *out_idx += in_size;
}

void
build_elk()
{
    char main_buffer[ECO_KiB(340)] = {0};
    size mb_size = 0;

    // Load all the files
    char const *fname = "../src/elk.h";
    b32 success = load_file(fname, sizeof(main_buffer), main_buffer, &mb_size);
    StopIf(!success, return);

    // Write out the buffer
    CoyFileWriter elk_ = coy_file_create("elk.h");
    CoyFileWriter *elk = &elk_;
    StopIf(!elk->valid, fprintf(stderr, "Error opening elk.h for output\n"); return);

    size elk_wrote = coy_file_write(elk, mb_size, main_buffer);
    Assert(elk_wrote == mb_size);

    coy_file_writer_close(elk);
}

void
build_magpie()
{
    // Output buffer
    size oi = 0; //output index
    char finished_lib[ECO_KiB(350)] = {0};

    char main_buffer[ECO_KiB(100)] = {0};
    size mb_size = 0;
    char win32_buffer[ECO_KiB(100)] = {0};
    size w32_size = 0;
    char apple_buffer[ECO_KiB(20)] = {0};
    size ap_size = 0;
    char linux_buffer[ECO_KiB(20)] = {0};
    size li_size = 0;

    // Load all the files
    char const *fname = "../src/magpie.h";
    b32 success = load_file(fname, sizeof(main_buffer), main_buffer, &mb_size);
    StopIf(!success, return);

    char const *fname_win32 = "../src/magpie_win32.h";
    success = load_file(fname_win32, sizeof(win32_buffer), win32_buffer, &w32_size);
    StopIf(!success, return);

    char const *fname_apple = "../src/magpie_apple_osx.h";
    success = load_file(fname_apple, sizeof(apple_buffer), apple_buffer, &ap_size);
    StopIf(!success, return);

    char const *fname_linux = "../src/magpie_linux.h";
    success = load_file(fname_linux, sizeof(linux_buffer), linux_buffer, &li_size);
    StopIf(!success, return);

    // Merge them in a buffer
    char *c = main_buffer;
    char *end = memchr(c, '\0', sizeof(main_buffer));

    char *insert_marker = memchr(c, '#', end - c);
    Assert(insert_marker);
    while(insert_marker - c < end - c)
    {
	  if(str_eq("#include \"magpie_win32.h\"", insert_marker, 25))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, w32_size, win32_buffer);
		insert_marker += 25;
		c = insert_marker;
	  }
	  else if(str_eq("#include \"magpie_linux.h\"", insert_marker, 25))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, li_size, linux_buffer);
		insert_marker += 25;
		c = insert_marker;
	  }
	  else if(str_eq("#include \"magpie_apple_osx.h\"", insert_marker, 29))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, ap_size, apple_buffer);
		insert_marker += 29;
		c = insert_marker;
	  }
	  else
	  {
		insert_marker = memchr(insert_marker + 1, '#', end - insert_marker);
		if(!insert_marker)
		{
		    insert_buffer(sizeof(finished_lib), &oi, finished_lib, end - c, c);
		    break;
		}
	  }
    }

    // Write out the buffer
    CoyFileWriter magpie_ = coy_file_create("magpie.h");
    CoyFileWriter *magpie = &magpie_;
    StopIf(!magpie->valid, fprintf(stderr, "Error opening magpie.h for output\n"); return);

    size magpie_wrote = coy_file_write(magpie, oi, finished_lib);
    Assert(magpie_wrote == oi);

    coy_file_writer_close(magpie);
}

void
build_coyote()
{
    // Output buffer
    size oi = 0; //output index
    char finished_lib[ECO_KiB(350)] = {0};

    char main_buffer[ECO_KiB(100)] = {0};
    size mb_size = 0;
    char win32_buffer[ECO_KiB(100)] = {0};
    size w32_size = 0;
    char common_buffer[ECO_KiB(100)] = {0};
    size common_size = 0;
    char apple_buffer[ECO_KiB(20)] = {0};
    size ap_size = 0;
    char linux_buffer[ECO_KiB(20)] = {0};
    size li_size = 0;

    // Load all the files
    char const *fname = "../src/coyote.h";
    b32 success = load_file(fname, sizeof(main_buffer), main_buffer, &mb_size);
    StopIf(!success, return);

    char const *fname_win32 = "../src/coyote_win32.h";
    success = load_file(fname_win32, sizeof(win32_buffer), win32_buffer, &w32_size);
    StopIf(!success, return);

    char const *fname_common = "../src/coyote_linux_apple_common.h";
    success = load_file(fname_common, sizeof(common_buffer), common_buffer, &common_size);
    StopIf(!success, return);

    char const *fname_apple = "../src/coyote_apple_osx.h";
    success = load_file(fname_apple, sizeof(apple_buffer), apple_buffer, &ap_size);
    StopIf(!success, return);

    char const *fname_linux = "../src/coyote_linux.h";
    success = load_file(fname_linux, sizeof(linux_buffer), linux_buffer, &li_size);
    StopIf(!success, return);

    // Merge them in a buffer
    char *c = main_buffer;
    char *end = memchr(c, '\0', sizeof(main_buffer));

    char *insert_marker = memchr(c, '#', end - c);
    Assert(insert_marker);
    while(insert_marker - c < end - c)
    {
	  if(str_eq("#include \"coyote_win32.h\"", insert_marker, 25))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, w32_size, win32_buffer);
		insert_marker += 25;
		c = insert_marker;
	  }
	  if(str_eq("#include \"coyote_linux_apple_common.h\"", insert_marker, 38))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, common_size, common_buffer);
		insert_marker += 38;
		c = insert_marker;
	  }
	  else if(str_eq("#include \"coyote_linux.h\"", insert_marker, 25))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, li_size, linux_buffer);
		insert_marker += 25;
		c = insert_marker;
	  }
	  else if(str_eq("#include \"coyote_apple_osx.h\"", insert_marker, 29))
	  {
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, insert_marker - c, c);
		insert_buffer(sizeof(finished_lib), &oi, finished_lib, ap_size, apple_buffer);
		insert_marker += 29;
		c = insert_marker;
	  }
	  else
	  {
		insert_marker = memchr(insert_marker + 1, '#', end - insert_marker);
		if(!insert_marker)
		{
		    insert_buffer(sizeof(finished_lib), &oi, finished_lib, end - c, c);
		    break;
		}
	  }
    }

    // Write out the buffer
    CoyFileWriter coyote_ = coy_file_create("coyote.h");
    CoyFileWriter *coyote = &coyote_;
    StopIf(!coyote->valid, fprintf(stderr, "Error opening coyote.h for output\n"); return);

    size coyote_wrote = coy_file_write(coyote, oi, finished_lib);
    Assert(coyote_wrote == oi);

    coy_file_writer_close(coyote);
}

void
build_packrat()
{
    char main_buffer[ECO_KiB(340)] = {0};
    size mb_size = 0;

    // Load all the files
    char const *fname = "../src/packrat.h";
    b32 success = load_file(fname, sizeof(main_buffer), main_buffer, &mb_size);
    StopIf(!success, return);

    // Write out the buffer
    CoyFileWriter packrat_ = coy_file_create("packrat.h");
    CoyFileWriter *packrat = &packrat_;
    StopIf(!packrat->valid, fprintf(stderr, "Error opening packrat.h for output\n"); return);

    size packrat_wrote = coy_file_write(packrat, mb_size, main_buffer);
    Assert(packrat_wrote == mb_size);

    coy_file_writer_close(packrat);
}

int
main(int argc, char *argv[])
{
    build_elk();
    build_magpie();
    build_coyote();
    build_packrat();
}
