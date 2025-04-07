#ifndef _COYOTE_LINUX_APPLE_OSX_COMMON_H_
#define _COYOTE_LINUX_APPLE_OSX_COMMON_H_
/*---------------------------------------------------------------------------------------------------------------------------
 *                                         Apple/MacOSX Linux Common Implementation
 *-------------------------------------------------------------------------------------------------------------------------*/

#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <x86intrin.h>
#include <unistd.h>
#include <dlfcn.h>

static inline u64
coy_time_now(void)
{
    struct timeval tv = {0};
    int errcode = gettimeofday(&tv, NULL);

    StopIf(errcode != 0, goto ERR_RETURN);
    StopIf(tv.tv_sec < 0, goto ERR_RETURN);

    return tv.tv_sec;

ERR_RETURN:
    return UINT64_MAX;
}

static char const coy_path_sep = '/';

static inline CoyPathInfo
coy_path_info(char *path, b32 assume_file)
{
    CoyPathInfo info = {0};
    info.full_path = path;

    /* Get the length. */
    size len = 0;
    char *c = path;
    while(*c && len < PATH_MAX)
    {
        ++len;
        ++c;
    }

    /* Check for error condition. */
    if(len == 0 || len >= PATH_MAX)
    {
        return info; /* Return empty info, there's nothing here. */
    }

    /* Find extension, and base.*/
    size idx = len - 1;
    c = &path[idx];
    if(*c == coy_path_sep) /* Skip a trailing slash. */
    {
        --c;
        --idx;
    }
    size next_len = 0;
    while(idx >= 0)
    {
        /* Found the extension. */
        if(*c == '.' && !info.extension.start)
        {
            info.extension = (CoyPathStr){ .start = c + 1, .len = next_len };
        }

        /* Found file name, but check to make sure it's not just a trailing slash. */
        if(*c == coy_path_sep && !info.base.start && (assume_file || info.extension.start) && (idx < len - 1))
        {
            info.base = (CoyPathStr){ .start = c + 1, .len = next_len };
            info.is_file = true; /* Assume this is a file. */
            next_len = -1;
        }

        /* Increment counters. */
        ++next_len;
        --idx;
        --c;
    }

    info.dir.start = path;
    info.dir.len = next_len;

    /* Does it exist? */
    struct stat stat_buf = {0};
    int ret_code = stat(path, &stat_buf);
    if(ret_code != 0)
    {
        info.exists = false;
    }
    else
    {
        info.exists = true;
        info.is_file = stat_buf.st_mode & S_IFDIR ? false : true;
    }

    return info;
}

static inline b32 
coy_path_append(size buf_len, char path_buffer[], char const *new_path)
{
    // Find first '\0'
    size position = 0;
    char *c = path_buffer;
    while(position < buf_len && *c)
    {
      ++c;
      position += 1;
    }

    StopIf(position >= buf_len, goto ERR_RETURN);

    // Add a path separator - unless the buffer is empty or the last path character was a path separator.
    if(position > 0 && path_buffer[position - 1] != coy_path_sep)
    {
      path_buffer[position] = coy_path_sep;
      position += 1;
      StopIf(position >= buf_len, goto ERR_RETURN);
    }

    // Copy in the new path part.
    char const *new_c = new_path;
    while(position < buf_len && *new_c)
    {
        path_buffer[position] = *new_c;
        ++new_c;
        position += 1;
    }

    StopIf(position >= buf_len, goto ERR_RETURN);

    // Null terminate the path.
    path_buffer[position] = '\0';
    
    return true;

ERR_RETURN:
    path_buffer[buf_len - 1] = '\0';
    return false;
}

static inline CoyFileWriter
coy_file_create(char const *filename)
{
    int fd = open( filename,                                        // char const *pathname
                   O_WRONLY | O_CREAT | O_TRUNC,                    // Write only, create if needed, or truncate if needed.
                   S_IRWXU | S_IRGRP | S_IXGRP |S_IROTH | S_IXOTH); // Default permissions 0755

    if (fd >= 0)
    {
        return (CoyFileWriter){ .handle = (iptr) fd, .valid = true  };
    }
    else
    {
        return (CoyFileWriter){ .handle = (iptr) fd, .valid = false };
    }
}

static inline CoyFileWriter
coy_file_append(char const *filename)
{
    int fd = open( filename,                                        // char const *pathname
                   O_WRONLY | O_CREAT | O_APPEND,                   // Write only, create if needed, and append.
                   S_IRWXU | S_IRGRP | S_IXGRP |S_IROTH | S_IXOTH); // Default permissions 0755

    if (fd >= 0) {
        return (CoyFileWriter){ .handle = (iptr) fd, .valid = true  };
    }
    else
    {
        return (CoyFileWriter){ .handle = (iptr) fd, .valid = false };
    }
}

static inline size 
coy_file_writer_flush(CoyFileWriter *file)
{
    StopIf(!file->valid, goto ERR_RETURN);

    _Static_assert(sizeof(ssize_t) == sizeof(size), "oh come on people. ssize_t != intptr_t!? Really!");

    if(file->buf_cursor > 0)
    {
        ssize_t num_bytes_written = write((int)file->handle, file->buffer, file->buf_cursor);
        StopIf(num_bytes_written != file->buf_cursor, goto ERR_RETURN);
        file->buf_cursor = 0;
        return (size) num_bytes_written;
    }

    return 0;

ERR_RETURN:
    return -1;
}

static inline size 
coy_file_write(CoyFileWriter *file, size nbytes_to_write, byte const *buffer)
{
    Assert(nbytes_to_write >= 0);
    size num_bytes_written = 0;

    /* Check if we need to flush the buffer */
    if(nbytes_to_write > (COY_FILE_WRITER_BUF_SIZE - file->buf_cursor))
    {
        num_bytes_written = coy_file_writer_flush(file);
        StopIf(num_bytes_written < 0, goto ERR_RETURN);
    }

    /* For "small" writes, buffer the data. */
    if(nbytes_to_write < COY_FILE_WRITER_BUF_SIZE)
    {
        memcpy(file->buffer + file->buf_cursor, buffer, nbytes_to_write);
        file->buf_cursor += nbytes_to_write;
        return nbytes_to_write;
    }
    else
    {
        /* For large writes, just skip several trips through the buffer. */
        num_bytes_written = write((int)file->handle, buffer, nbytes_to_write);
        StopIf(num_bytes_written < 0, goto ERR_RETURN);
        return (size) num_bytes_written;
    }

ERR_RETURN:
    return -1;
}

static inline void 
coy_file_writer_close(CoyFileWriter *file)
{
    /* TODO: Rework API to return error if the flush fails. */
    coy_file_writer_flush(file);
    /* int err_code = */ close((int)file->handle);
    file->valid = false;
}

static inline size 
coy_file_slurp(char const *filename, size buf_size, byte *buffer)
{
    size file_size = coy_file_size(filename);
    StopIf(file_size < 1 || file_size > buf_size, goto ERR_RETURN);

    int fd = open( filename, // char const *pathname
                   O_RDONLY, // Read only
                   0);       // No mode information needed.

    StopIf(fd < 0, goto ERR_RETURN);
    
    size space_available = buf_size;
    size num_bytes_read = 0;
    size total_num_bytes_read = 0;
    do
    {
        num_bytes_read = read(fd, buffer + total_num_bytes_read, space_available);
        space_available -= num_bytes_read;
        total_num_bytes_read += num_bytes_read;

        StopIf(num_bytes_read < 0, goto ERR_RETURN);

    } while(space_available && num_bytes_read);

    close(fd);
    return (size) total_num_bytes_read;

ERR_RETURN:
    return -1;
}

static inline CoyFileReader 
coy_file_open_read(char const *filename)
{
    int fd = open( filename, // char const *pathname
                   O_RDONLY, // Read only
                   0);       // No mode information needed.

    if (fd >= 0)
    {
        return (CoyFileReader){ .handle = (iptr) fd, .valid = true  };
    }
    else
    {
        return (CoyFileReader){ .handle = (iptr) fd, .valid = false };
    }
}

static inline size 
coy_file_fill_buffer(CoyFileReader *file)
{
    _Static_assert(sizeof(ssize_t) <= sizeof(size), "oh come on people. ssize_t != intptr_t!? Really!");

    if(file->bytes_remaining > 0)
    {
        /* Move remaining data to the front of the buffer */
        memmove(file->buffer, file->buffer + file->buf_cursor, file->bytes_remaining);
    }

    file->buf_cursor = 0;

    size space_available = COY_FILE_READER_BUF_SIZE - file->bytes_remaining;
    size num_bytes_read = 0;
    size total_num_bytes_read = 0;
    while(space_available)
    {
        num_bytes_read = read((int)file->handle, file->buffer + file->bytes_remaining + total_num_bytes_read, space_available);
        space_available -= num_bytes_read;
        total_num_bytes_read += num_bytes_read;

        StopIf(num_bytes_read < 0, goto ERR_RETURN);

        if(num_bytes_read == 0) { break; }
    }

    file->bytes_remaining += total_num_bytes_read;

    return (size) total_num_bytes_read;

ERR_RETURN:
    return -1;
}

static inline size 
coy_file_read(CoyFileReader *file, size buf_size, byte *buffer)
{
    Assert(buf_size > 0);
    StopIf(!file->valid, goto ERR_RETURN);

    if(buf_size > file->bytes_remaining)
    {
        size bytes_read = coy_file_fill_buffer(file);
        StopIf(bytes_read < 0, goto ERR_RETURN);
    }

    size size_to_copy = buf_size > file->bytes_remaining ? file->bytes_remaining : buf_size;
    memcpy(buffer, file->buffer + file->buf_cursor, size_to_copy);
    file->buf_cursor += size_to_copy;
    file->bytes_remaining -= size_to_copy;

    return size_to_copy;

ERR_RETURN:
    return -1;
}

static inline void 
coy_file_reader_close(CoyFileReader *file)
{
    /* int err_code = */ close((int)file->handle);
    file->valid = false;
}

static inline size 
coy_file_size(char const *filename)
{
    _Static_assert(sizeof(off_t) == 8 && sizeof(size) == sizeof(off_t), "Need 64-bit off_t");
    struct stat statbuf = {0};
    int success = stat(filename, &statbuf);
    StopIf(success != 0, return -1);

    return (size)statbuf.st_size;
}

static inline CoyMemMappedFile 
coy_memmap_read_only(char const *filename)
{
    _Static_assert(sizeof(usize) == sizeof(size), "sizeof(size_t) != sizeof(intptr_t)");

    size size_in_bytes = coy_file_size(filename);
    StopIf(size_in_bytes == -1, goto ERR_RETURN);

    int fd = open( filename, // char const *pathname
                   O_RDONLY, // Read only
                   0);       // No mode information needed.
    StopIf(fd < 0, goto ERR_RETURN);

    byte const *data = mmap(NULL,           // Starting address - OS chooses
                            size_in_bytes,  // The size of the mapping,should be the file size for this
                            PROT_READ,      // Read only access
                            MAP_PRIVATE,    // flags - not sure if anything is necessary for read only
                            fd,             // file descriptor for the file to map
                            0);             // offset into the file to read

    close(fd);
    StopIf(data == MAP_FAILED, goto ERR_RETURN);

    return (CoyMemMappedFile){ .size_in_bytes = (size)size_in_bytes, 
                               .data = data, 
                               ._internal = {0}, 
                               .valid = true 
    };

ERR_RETURN:
    return (CoyMemMappedFile) { .valid = false };
}

static inline void 
coy_memmap_close(CoyMemMappedFile *file)
{
    /*BOOL success = */ munmap((void *)file->data, file->size_in_bytes);
    file->valid = false;

    return;
}

static inline CoyFileNameIter 
coy_file_name_iterator_open(char const *directory_path, char const *file_extension)
{
    DIR *d = opendir(directory_path);
    StopIf(!d, goto ERR_RETURN);

    return (CoyFileNameIter){ .os_handle = (iptr)d, .file_extension=file_extension, .valid=true};

ERR_RETURN:
    return (CoyFileNameIter) {.valid=false};
}

static inline char const *
coy_file_name_iterator_next(CoyFileNameIter *cfni)
{
    if(cfni->valid)
    {
        DIR *d = (DIR *)cfni->os_handle;
        struct dirent *entry = readdir(d);
        while(entry)
        {
            if(entry->d_type == DT_REG) {
                if(cfni->file_extension == NULL) 
                {
                    return entry->d_name;
                }
                else 
                {
                    char const *ext = coy_file_extension(entry->d_name);
                    if(coy_null_term_strings_equal(ext, cfni->file_extension))
                    {
                        return entry->d_name;
                    }
                }
            }
            entry = readdir(d);
        }
    }

    cfni->valid = false;
    return NULL;
}

static inline void 
coy_file_name_iterator_close(CoyFileNameIter *cfin)
{
    DIR *d = (DIR *)cfin->os_handle;
    /*int rc = */ closedir(d);
    *cfin = (CoyFileNameIter){0};
    return;
}

static inline CoySharedLibHandle 
coy_shared_lib_load(char const *lib_name)
{
    void *h = dlopen(lib_name, RTLD_LAZY);
    PanicIf(!h);
    return (CoySharedLibHandle) { .handle = h };
}

static inline void 
coy_shared_lib_unload(CoySharedLibHandle handle)
{
    dlclose(handle.handle);
}

static inline void *
coy_share_lib_load_symbol(CoySharedLibHandle handle, char const *symbol_name)
{
    void *s = dlsym(handle.handle, symbol_name);
    PanicIf(!s);
    return s;
}

static inline CoyTerminalSize 
coy_get_terminal_size(void)
{
    struct winsize w = {0};
    int ret_val = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if(ret_val == 0) 
    { 
        return (CoyTerminalSize){ .columns = w.ws_col, .rows = w.ws_row }; 
    }

    return (CoyTerminalSize){ .columns = -1, .rows = -1 };
}

static inline void *
coy_thread_func_internal(void *thread_params)
{
    CoyThread *thrd = thread_params;
    thrd->func(thrd->thread_data);

    return NULL;
}

static inline b32
coy_thread_create(CoyThread *thrd, CoyThreadFunc func, void *thread_data)
{
    *thrd = (CoyThread){ .func=func, .thread_data=thread_data };

    _Static_assert(sizeof(pthread_t) <= sizeof(thrd->handle), "pthread_t doesn't fit in CoyThread");
    _Static_assert(_Alignof(pthread_t) <= 16, "pthread_t doesn't fit alignment in CoyThread");

    return 0 == pthread_create((pthread_t *)thrd->handle, NULL, coy_thread_func_internal, thrd);
}

static inline b32
coy_thread_join(CoyThread *thread)
{

    pthread_t *t = (pthread_t  *)thread->handle;
    int status = pthread_join(*t, NULL);
    if(status == 0) { return true; }
    return false;
}

static inline void 
coy_thread_destroy(CoyThread *thread)
{
    *thread = (CoyThread){0};
}

static inline CoyMutex 
coy_mutex_create()
{
    CoyMutex mtx = {0};
    pthread_mutex_t mut = {0};

    _Static_assert(sizeof(pthread_mutex_t) <= sizeof(mtx.mutex), "pthread_mutex_t doesn't fit in CoyMutex");
    _Static_assert(_Alignof(pthread_mutex_t) <= 16, "pthread_mutex_t doesn't fit alignment in CoyMutex");

    int status = pthread_mutex_init(&mut, NULL);
    if(status == 0)
    {
        memcpy(&mtx.mutex[0], &mut, sizeof(mut));
        mtx.valid = true;

        return mtx;
    }

    return mtx;
}

static inline b32 
coy_mutex_lock(CoyMutex *mutex)
{
    return pthread_mutex_lock((pthread_mutex_t *)&mutex->mutex[0]) == 0;
}

static inline b32 
coy_mutex_unlock(CoyMutex *mutex)
{
    return pthread_mutex_unlock((pthread_mutex_t *)&mutex->mutex[0]) == 0;
}

static inline void 
coy_mutex_destroy(CoyMutex *mutex)
{
    pthread_mutex_destroy((pthread_mutex_t *)&mutex->mutex[0]);
    mutex->valid = false;
}

static inline CoyCondVar 
coy_condvar_create(void)
{
    CoyCondVar cv = {0};

    _Static_assert(sizeof(pthread_cond_t) <= sizeof(cv.cond_var), "pthread_cond_t doesn't fit in CoyCondVar");
    _Static_assert(_Alignof(pthread_cond_t) <= 16, "pthread_cond_t doesn't fit alignment in CoyCondVar");

    cv.valid = pthread_cond_init((pthread_cond_t *)&cv.cond_var[0], NULL) == 0;
    return cv;
}

static inline b32 
coy_condvar_sleep(CoyCondVar *cv, CoyMutex *mtx)
{
    return 0 == pthread_cond_wait((pthread_cond_t *)&cv->cond_var[0], (pthread_mutex_t *)&mtx->mutex[0]);
}

static inline b32 
coy_condvar_wake(CoyCondVar *cv)
{
    return 0 == pthread_cond_signal((pthread_cond_t *)&cv->cond_var[0]);
}

static inline b32 
coy_condvar_wake_all(CoyCondVar *cv)
{
    return 0 == pthread_cond_broadcast((pthread_cond_t *)&cv->cond_var[0]);
}

static inline void 
coy_condvar_destroy(CoyCondVar *cv)
{
    int status = pthread_cond_destroy((pthread_cond_t *)&cv->cond_var[0]);
    Assert(status == 0);
    cv->valid = false;
}

static inline u64
coy_profile_read_cpu_timer(void)
{
	return __rdtsc();
}

static inline u64 
coy_profile_get_os_timer_freq(void)
{
	return 1000000;
}

static inline u64 
coy_profile_read_os_timer(void)
{
	struct timeval value;
	gettimeofday(&value, 0);
	
	u64 res = coy_profile_get_os_timer_freq() * (u64)value.tv_sec + (u64)value.tv_usec;
	return res;
}

#endif
