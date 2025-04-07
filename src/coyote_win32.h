#ifndef _COYOTE_WIN32_H_
#define _COYOTE_WIN32_H_

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                 Windows Implementation
 *-------------------------------------------------------------------------------------------------------------------------*/
#include <windows.h>
#include <bcrypt.h>
#include <intrin.h>
#include <psapi.h>

_Static_assert(UINT32_MAX < INTPTR_MAX, "DWORD cannot be cast to intptr_t safely.");

union WinTimePun
{
    FILETIME ft;
    ULARGE_INTEGER as_uint64;
};

static inline u64
coy_time_now(void)
{
    SYSTEMTIME now_st = {0};
    GetSystemTime(&now_st);

    union WinTimePun now = {0};
    b32 success = SystemTimeToFileTime(&now_st, &now.ft);
    StopIf(!success, goto ERR_RETURN);

    SYSTEMTIME epoch_st = { 
        .wYear=1970, 
        .wMonth=1,
        .wDayOfWeek=4,
        .wDay=1,
        .wHour=0,
        .wMinute=0,
        .wSecond=0,
        .wMilliseconds=0
    };

    union WinTimePun epoch = {0};
    success = SystemTimeToFileTime(&epoch_st, &epoch.ft);
    PanicIf(!success);

    return (now.as_uint64.QuadPart - epoch.as_uint64.QuadPart) / 10000000;

ERR_RETURN:
    return UINT64_MAX;
}

static char const coy_path_sep = '\\';

static inline CoyPathInfo
coy_path_info(char *path, b32 assume_file)
{
    CoyPathInfo info = {0};
    info.full_path = path;

    /* Get the length. */
    size len = 0;
    char *c = path;
    while(*c && len < MAX_PATH)
    {
        ++len;
        ++c;
    }

    /* Check for error condition. */
    if(len == 0 || len >= MAX_PATH)
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
    DWORD attr = GetFileAttributes(path);
    if(attr == INVALID_FILE_ATTRIBUTES)
    {
        info.exists = false;
    }
    else
    {
        info.exists = true;
        info.is_file = FILE_ATTRIBUTE_DIRECTORY & attr ? false : true;
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
    HANDLE fh = CreateFileA(filename,              // [in]           LPCSTR                lpFileName,
                            GENERIC_WRITE,         // [in]           DWORD                 dwDesiredAccess,
                            0,                     // [in]           DWORD                 dwShareMode,
                            NULL,                  // [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            CREATE_ALWAYS,         // [in]           DWORD                 dwCreationDisposition,
                            FILE_ATTRIBUTE_NORMAL, // [in]           DWORD                 dwFlagsAndAttributes,
                            NULL);                 // [in, optional] HANDLE                hTemplateFile

    if(fh != INVALID_HANDLE_VALUE)
    {
        return (CoyFileWriter){.handle = (iptr)fh, .valid = true};
    }
    else
    {
        return (CoyFileWriter){.handle = (iptr)INVALID_HANDLE_VALUE, .valid = false};
    }
}

static inline CoyFileWriter
coy_file_append(char const *filename)
{
    HANDLE fh = CreateFileA(filename,              // [in]           LPCSTR                lpFileName,
                            FILE_APPEND_DATA,      // [in]           DWORD                 dwDesiredAccess,
                            0,                     // [in]           DWORD                 dwShareMode,
                            NULL,                  // [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            OPEN_ALWAYS,           // [in]           DWORD                 dwCreationDisposition,
                            FILE_ATTRIBUTE_NORMAL, // [in]           DWORD                 dwFlagsAndAttributes,
                            NULL);                 // [in, optional] HANDLE                hTemplateFile

    if(fh != INVALID_HANDLE_VALUE)
    {
        return (CoyFileWriter){.handle = (iptr)fh, .valid = true};
    }
    else
    {
        return (CoyFileWriter){.handle = (iptr)INVALID_HANDLE_VALUE, .valid = false};
    }
}

static inline size 
coy_file_writer_flush(CoyFileWriter *file)
{
    StopIf(!file->valid, goto ERR_RETURN);

    if(file->buf_cursor)
    {
        DWORD nbytes_written = 0;
        BOOL success = WriteFile(
            (HANDLE)file->handle,     // [in]                HANDLE       hFile,
            file->buffer,             // [in]                LPCVOID      lpBuffer,
            (DWORD)file->buf_cursor,  // [in]                DWORD        nNumberOfBytesToWrite,
            &nbytes_written,          // [out, optional]     LPDWORD      lpNumberOfBytesWritten,
            NULL                      // [in, out, optional] LPOVERLAPPED lpOverlapped
        );

        StopIf(!success || file->buf_cursor != (size)nbytes_written, goto ERR_RETURN);

        file->buf_cursor = 0;

        return (size)nbytes_written;
    }

    return 0;

ERR_RETURN:
    return -1;
}

static inline void 
coy_file_writer_close(CoyFileWriter *file)
{
    /* TODO change API to return size so I can return an error if the flush fails. */
    coy_file_writer_flush(file);
    CloseHandle((HANDLE)file->handle);
    file->valid = false;

    return;
}

static inline size 
coy_file_write(CoyFileWriter *file, size nbytes_write, byte const *buffer)
{
    /* check to see if the buffer needs flushed. */
    if(file->buf_cursor + nbytes_write > COY_FILE_WRITER_BUF_SIZE)
    {
        size bytes_flushed = coy_file_writer_flush(file);
        StopIf(bytes_flushed < 0, goto ERR_RETURN);
    }

    if(nbytes_write < COY_FILE_WRITER_BUF_SIZE)
    {
        /* Dump small writes into the buffer. */
        memcpy(file->buffer + file->buf_cursor, buffer, nbytes_write);
        file->buf_cursor += nbytes_write;
        return nbytes_write;
    }
    else
    {
        /* Large writes bypass the buffer and go straight to the file. */
        Assert(INT32_MAX >= nbytes_write); /* Not prepared for REALLY large writes. */
        DWORD nbytes_written = 0;
        BOOL success = WriteFile(
            (HANDLE)file->handle,     // [in]                HANDLE       hFile,
            buffer,                   // [in]                LPCVOID      lpBuffer,
            (DWORD)nbytes_write,      // [in]                DWORD        nNumberOfBytesToWrite,
            &nbytes_written,          // [out, optional]     LPDWORD      lpNumberOfBytesWritten,
            NULL                      // [in, out, optional] LPOVERLAPPED lpOverlapped
        );

        StopIf(!success, goto ERR_RETURN);
        return (size)nbytes_written;
    }

ERR_RETURN:
    return -1;
}

static inline CoyFileReader
coy_file_open_read(char const *filename)
{
    HANDLE fh = CreateFileA(filename,              // [in]           LPCSTR                lpFileName,
                            GENERIC_READ,          // [in]           DWORD                 dwDesiredAccess,
                            FILE_SHARE_READ,       // [in]           DWORD                 dwShareMode,
                            NULL,                  // [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            OPEN_EXISTING,         // [in]           DWORD                 dwCreationDisposition,
                            FILE_ATTRIBUTE_NORMAL, // [in]           DWORD                 dwFlagsAndAttributes,
                            NULL);                 // [in, optional] HANDLE                hTemplateFile

    if(fh != INVALID_HANDLE_VALUE)
    {
        return (CoyFileReader){.handle = (iptr)fh, .valid = true};
    }
    else
    {
        return (CoyFileReader){.handle = (iptr)INVALID_HANDLE_VALUE, .valid = false};
    }
}

static inline size 
coy_file_fill_buffer(CoyFileReader *file)
{
    if(file->bytes_remaining > 0)
    {
        /* Move remaining data to the front of the buffer */
        memmove(file->buffer, file->buffer + file->buf_cursor, file->bytes_remaining);
    }
    file->buf_cursor = 0;

    size space_available = COY_FILE_READER_BUF_SIZE - file->bytes_remaining;

    Assert(INT32_MAX >= space_available); /* Not prepared for REALLY large reads. */
    DWORD nbytes_read = 0;
    BOOL success =  ReadFile((HANDLE) file->handle,                //  [in]                HANDLE       hFile,
                             file->buffer + file->bytes_remaining, //  [out]               LPVOID       lpBuffer,
                             (DWORD)space_available,               //  [in]                DWORD        nNumberOfBytesToRead,
                             &nbytes_read,                         //  [out, optional]     LPDWORD      lpNumberOfBytesRead,
                             NULL);                                //  [in, out, optional] LPOVERLAPPED lpOverlapped

    StopIf(!success, goto ERR_RETURN);

    file->bytes_remaining += nbytes_read;

    return (size)nbytes_read;

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
    CloseHandle((HANDLE)file->handle);
    file->valid = false;

    return;
}

static inline size 
coy_file_slurp(char const *filename, size buf_size, byte *buffer)
{
    size file_size = coy_file_size(filename);
    StopIf(file_size < 1 || file_size > buf_size, goto ERR_RETURN);

    HANDLE fh = CreateFileA(filename,              // [in]           LPCSTR                lpFileName,
                            GENERIC_READ,          // [in]           DWORD                 dwDesiredAccess,
                            FILE_SHARE_READ,       // [in]           DWORD                 dwShareMode,
                            NULL,                  // [in, optional] LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                            OPEN_EXISTING,         // [in]           DWORD                 dwCreationDisposition,
                            FILE_ATTRIBUTE_NORMAL, // [in]           DWORD                 dwFlagsAndAttributes,
                            NULL);                 // [in, optional] HANDLE                hTemplateFile

    StopIf(fh == INVALID_HANDLE_VALUE, goto ERR_RETURN);
    
    size space_available = buf_size;

    Assert(INT32_MAX >= space_available); /* Not prepared for REALLY large reads. */
    DWORD nbytes_read = 0;
    BOOL success =  ReadFile(fh,                     //  [in]                HANDLE       hFile,
                             buffer,                 //  [out]               LPVOID       lpBuffer,
                             (DWORD)space_available, //  [in]                DWORD        nNumberOfBytesToRead,
                             &nbytes_read,           //  [out, optional]     LPDWORD      lpNumberOfBytesRead,
                             NULL);                  //  [in, out, optional] LPOVERLAPPED lpOverlapped

    success &= CloseHandle(fh);

    StopIf(!success, goto ERR_RETURN);

    return (size)nbytes_read;

ERR_RETURN:
    return -1;
}

static inline size 
coy_file_size(char const *filename)
{
    WIN32_FILE_ATTRIBUTE_DATA attr = {0};
    BOOL success = GetFileAttributesExA(filename, GetFileExInfoStandard, &attr);
    StopIf(!success, goto ERR_RETURN);

    _Static_assert(sizeof(uptr) == 2 * sizeof(DWORD) && sizeof(DWORD) == 4);
    uptr file_size = ((uptr)attr.nFileSizeHigh << 32) | attr.nFileSizeLow;
    StopIf(file_size > INTPTR_MAX, goto ERR_RETURN);

    return (size) file_size;

ERR_RETURN:
    return -1;
}

static inline CoyMemMappedFile 
coy_memmap_read_only(char const *filename)
{
    CoyFileReader cf = coy_file_open_read(filename);
    StopIf(!cf.valid, goto ERR_RETURN);

    HANDLE fmh =  CreateFileMappingA((HANDLE)cf.handle, // [in]           HANDLE                hFile,
                                     NULL,              // [in, optional] LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
                                     PAGE_READONLY,     // [in]           DWORD                 flProtect,
                                     0,                 // [in]           DWORD                 dwMaximumSizeHigh,
                                     0,                 // [in]           DWORD                 dwMaximumSizeLow,
                                     NULL);             // [in, optional] LPCSTR                lpName
    StopIf(fmh == INVALID_HANDLE_VALUE, goto CLOSE_CF_AND_ERR);

    LPVOID ptr =  MapViewOfFile(fmh,           // [in] HANDLE hFileMappingObject,
                                FILE_MAP_READ, // [in] DWORD  dwDesiredAccess,
                                0,             // [in] DWORD  dwFileOffsetHigh,
                                0,             // [in] DWORD  dwFileOffsetLow,
                                0);            // [in] SIZE_T dwNumberOfBytesToMap
    StopIf(!ptr, goto CLOSE_FMH_AND_ERR);

    // Get the size of the file mapped.
    DWORD file_size_high = 0;
    DWORD file_size_low = GetFileSize((HANDLE)cf.handle, &file_size_high);
    StopIf(file_size_low == INVALID_FILE_SIZE, goto CLOSE_FMH_AND_ERR);

    uptr file_size = ((uptr)file_size_high << 32) | file_size_low;
    StopIf(file_size > INTPTR_MAX, goto CLOSE_FMH_AND_ERR);


    return (CoyMemMappedFile){
      .size_in_bytes = (size)file_size, 
        .data = ptr, 
        ._internal = { cf.handle, (size)fmh }, 
        .valid = true 
    };

CLOSE_FMH_AND_ERR:
    CloseHandle(fmh);
CLOSE_CF_AND_ERR:
    coy_file_reader_close(&cf);
ERR_RETURN:
    return (CoyMemMappedFile) { .valid = false };
}

static inline void 
coy_memmap_close(CoyMemMappedFile *file)
{
    void const*data = file->data;
    iptr fh = file->_internal[0];
    HANDLE fmh = (HANDLE)file->_internal[1];

    /*BOOL success = */UnmapViewOfFile(data);
    CloseHandle(fmh);
    CoyFileReader cf = { .handle = fh, .valid = true };
    coy_file_reader_close(&cf);

    file->valid = false;

    return;
}

// I don't normally use static storage, but the linux interface uses it internally, so I'm stuck with those semantics.
// I'll use my own here.
static WIN32_FIND_DATA coy_file_name_iterator_data;
static char coy_file_name[1024];

static inline CoyFileNameIter
coy_file_name_iterator_open(char const *directory_path, char const *file_extension)
{
    char path_buf[1024] = {0};
    int i = 0;
    for(i = 0; i < sizeof(path_buf) && directory_path[i]; ++i)
    {
        path_buf[i] = directory_path[i];
    }
    StopIf(i + 2 >= sizeof(path_buf), goto ERR_RETURN);
    path_buf[i] = '\\';
    path_buf[i + 1] = '*';
    path_buf[i + 2] = '\0';

    HANDLE finder = FindFirstFile(path_buf, &coy_file_name_iterator_data);
    StopIf(finder == INVALID_HANDLE_VALUE, goto ERR_RETURN);
    return (CoyFileNameIter) { .os_handle=(iptr)finder, .file_extension=file_extension, .valid=true };

ERR_RETURN:
    return (CoyFileNameIter) { .valid=false };
}

static inline char const *
coy_file_name_iterator_next(CoyFileNameIter *cfni)
{
    if(cfni->valid)
    {
        // The first call to coy_file_name_iterator_open() should have populated
        char const *fname = coy_file_name_iterator_data.cFileName;
        b32 found = false;
        while(!found)
        {
            if(!(coy_file_name_iterator_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                if(cfni->file_extension == NULL)
                {
                    found = true;
                }
                else
                {
                    char const *ext = coy_file_extension(fname);
                    if(coy_null_term_strings_equal(ext, cfni->file_extension))
                    {
                        found = true;
                    }
                }
            }
            if(found)
            {
                int i = 0;
                for(i = 0; i < sizeof(coy_file_name) && fname[i]; ++i)
                {
                    coy_file_name[i] = fname[i];
                }
                coy_file_name[i] = '\0';
            }

            BOOL foundnext = FindNextFileA((HANDLE)cfni->os_handle, &coy_file_name_iterator_data);

            if(!foundnext)
            {
                cfni->valid = false;
                break;
            }
        }

        if(found) 
        {
            return coy_file_name;
        }
    }

    return NULL;
}

static inline void 
coy_file_name_iterator_close(CoyFileNameIter *cfin)
{
    HANDLE finder = (HANDLE)cfin->os_handle;
    FindClose(finder);
    *cfin = (CoyFileNameIter) {0};
    return;
}

static inline CoySharedLibHandle 
coy_shared_lib_load(char const *lib_name)
{
    void *h = LoadLibraryA(lib_name);
    PanicIf(!h);
    return (CoySharedLibHandle) { .handle = h };
}

static inline void 
coy_shared_lib_unload(CoySharedLibHandle handle)
{
    b32 success = FreeLibrary(handle.handle);
    PanicIf(!success);
}

static inline void *
coy_share_lib_load_symbol(CoySharedLibHandle handle, char const *symbol_name)
{
    void *s = GetProcAddress(handle.handle, symbol_name);
    PanicIf(!s);
    return s;
}

static inline CoyTerminalSize 
coy_get_terminal_size(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi = {0};
    BOOL success = GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

    if(success)
    {
        return (CoyTerminalSize) 
            {
                .columns = csbi.srWindow.Right - csbi.srWindow.Left + 1, 
                .rows =csbi.srWindow.Bottom - csbi.srWindow.Top + 1
            };
    }

    return (CoyTerminalSize) { .columns = -1, .rows = -1 };
}

static inline CoyMemoryBlock 
coy_memory_allocate(size minimum_num_bytes)
{
    if(minimum_num_bytes <= 0)
    {
         return (CoyMemoryBlock){.mem = 0, .size = minimum_num_bytes, .valid = false };
    }

    SYSTEM_INFO info = {0};
    GetSystemInfo(&info);
    uptr page_size = info.dwPageSize;
    uptr alloc_gran = info.dwAllocationGranularity;

    uptr target_granularity = (uptr)minimum_num_bytes > alloc_gran ? alloc_gran : page_size;

    uptr allocation_size = minimum_num_bytes;
    if(minimum_num_bytes % target_granularity)
    {
        allocation_size += target_granularity - (minimum_num_bytes % target_granularity);
    }

    if(allocation_size > INTPTR_MAX)
    {
         return (CoyMemoryBlock){.mem = 0, .size = INTPTR_MAX, .valid = false };
    }
    size a_size = (size)allocation_size;

    void *mem = VirtualAlloc(NULL, allocation_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if(!mem)
    {
         return (CoyMemoryBlock){.mem = 0, .size = 0, .valid = false };
    }

    return (CoyMemoryBlock){.mem = mem, .size = a_size, .valid = true };
}

static inline void
coy_memory_free(CoyMemoryBlock *mem)
{
    if(mem->valid)
    {
         /*BOOL success =*/ VirtualFree(mem->mem, 0, MEM_RELEASE);
         mem->valid = false;
    }

    return;
}

static inline u32 
coy_thread_func_internal(void *thread_params)
{
    CoyThread *thrd = thread_params;
    thrd->func(thrd->thread_data);

    return 0;
}


static inline b32
coy_thread_create(CoyThread *thrd, CoyThreadFunc func, void *thread_data)
{
    thrd->func = func;
    thrd->thread_data = thread_data;

    DWORD id = 0;
    HANDLE h =  CreateThread(
        NULL,                       // [in, optional]  LPSECURITY_ATTRIBUTES   lpThreadAttributes,
        0,                          // [in]            SIZE_T                  dwStackSize,
        coy_thread_func_internal,   // [in]            LPTHREAD_START_ROUTINE  lpStartAddress,
        thrd,                       // [in, optional]  __drv_aliasesMem LPVOID lpParameter,
        0,                          // [in]            DWORD                   dwCreationFlags,
        &id                         // [out, optional] LPDWORD                 lpThreadId
    );

    if(h == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    _Static_assert(sizeof(h) <= sizeof(thrd->handle), "handle doesn't fit in CoyThread");
    _Static_assert(_Alignof(HANDLE) <= 16, "handle doesn't fit alignment in CoyThread");
    memcpy(&thrd->handle[0], &h, sizeof(h));

    return true;
}

static inline b32
coy_thread_join(CoyThread *thread)
{
    HANDLE *h = (HANDLE *)&thread->handle[0];
    DWORD status = WaitForSingleObject(*h, INFINITE);
    return status == WAIT_OBJECT_0;
}

static inline void 
coy_thread_destroy(CoyThread *thread)
{
    HANDLE *h = (HANDLE *)&thread->handle[0];
    /* BOOL success = */ CloseHandle(*h);
    *thread = (CoyThread){0};
}

static inline CoyMutex 
coy_mutex_create()
{
    CoyMutex mutex = {0};

    _Static_assert(sizeof(CRITICAL_SECTION) <= sizeof(mutex.mutex), "CRITICAL_SECTION doesn't fit in CoyMutex");
    _Static_assert(_Alignof(CRITICAL_SECTION) <= 16, "CRITICAL_SECTION doesn't fit alignment in CoyMutex");

    CRITICAL_SECTION *m = (CRITICAL_SECTION *)&mutex.mutex[0];
    mutex.valid = InitializeCriticalSectionAndSpinCount(m, 0x400) != 0;
    return mutex;
}

static inline b32 
coy_mutex_lock(CoyMutex *mutex)
{
    EnterCriticalSection((CRITICAL_SECTION *)&mutex->mutex[0]);
    return true;
}

static inline b32 
coy_mutex_unlock(CoyMutex *mutex)
{
    LeaveCriticalSection((CRITICAL_SECTION *)&mutex->mutex[0]);
    return true;
}

static inline void 
coy_mutex_destroy(CoyMutex *mutex)
{
    DeleteCriticalSection((CRITICAL_SECTION *)&mutex->mutex[0]);
    mutex->valid = false;
}

static inline CoyCondVar 
coy_condvar_create(void)
{
    CoyCondVar cv = {0};

    _Static_assert(sizeof(CONDITION_VARIABLE) <= sizeof(cv.cond_var), "CONDITION_VARIABLE doesn't fit in CoyCondVar");
    _Static_assert(_Alignof(CONDITION_VARIABLE) <= 16, "CONDITION_VARIABLE doesn't fit alignment in CoyCondVar");

    InitializeConditionVariable((CONDITION_VARIABLE *)&cv.cond_var);
    cv.valid = true;
    return cv;
}

static inline b32 
coy_condvar_sleep(CoyCondVar *cv, CoyMutex *mtx)
{
    return 0 != SleepConditionVariableCS((CONDITION_VARIABLE *)&cv->cond_var, (CRITICAL_SECTION *)&mtx->mutex[0], INFINITE);
}

static inline b32 
coy_condvar_wake(CoyCondVar *cv)
{
    WakeConditionVariable((CONDITION_VARIABLE *)&cv->cond_var);
    return true;
}

static inline b32 
coy_condvar_wake_all(CoyCondVar *cv)
{
    WakeAllConditionVariable((CONDITION_VARIABLE *)&cv->cond_var);
    return true;
}

static inline void 
coy_condvar_destroy(CoyCondVar *cv)
{
    cv->valid = false;
}

static inline u64
coy_profile_read_cpu_timer(void)
{
	return __rdtsc();
}

static inline void 
coy_profile_initialize_os_metrics(void)
{
    if(!coy_global_os_metrics.initialized)
    {
        coy_global_os_metrics.handle = (uptr)OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, GetCurrentProcessId());
        coy_global_os_metrics.initialized = true;
    }
}

static inline void 
coy_profile_finalize_os_metrics(void)
{
    /* no op on win 32 */
}

static inline u64
coy_profile_read_os_page_fault_count(void)
{
    PROCESS_MEMORY_COUNTERS_EX memory_counters = { .cb = sizeof(memory_counters) };
    GetProcessMemoryInfo((HANDLE)coy_global_os_metrics.handle, (PROCESS_MEMORY_COUNTERS *)&memory_counters, sizeof(memory_counters));

    return memory_counters.PageFaultCount;
}

static inline u64 
coy_profile_get_os_timer_freq(void)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return freq.QuadPart;
}

static inline u64 
coy_profile_read_os_timer(void)
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}

#endif
