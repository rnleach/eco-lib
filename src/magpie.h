#ifndef _MAGPIE_H_
#define _MAGPIE_H_

#include <stdint.h>
#include <stddef.h>

#pragma warning(push)

/*---------------------------------------------------------------------------------------------------------------------------
 * TODO: Things I'd like to add.
 *-------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------------------------
 * Define simpler types.
 *-------------------------------------------------------------------------------------------------------------------------*/

/* Other libraries I may have already included may use these exact definitions too. */
#ifndef _TYPE_ALIASES_
#define _TYPE_ALIASES_

typedef int32_t     b32;
#ifndef false
   #define false 0
   #define true  1
#endif

#if !defined(_WINDOWS_) && !defined(_INC_WINDOWS)
typedef char       byte;
#endif

typedef ptrdiff_t  size;
typedef size_t    usize;

typedef uintptr_t  uptr;
typedef intptr_t   iptr;

typedef float       f32;
typedef double      f64;

typedef uint8_t      u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;
typedef int8_t       i8;
typedef int16_t     i16;
typedef int32_t     i32;
typedef int64_t     i64;
#endif

#define MAG_ARRAY_SIZE(A) (sizeof(A) / sizeof(A[0]))

/*---------------------------------------------------------------------------------------------------------------------------
 * Declare parts of the standard C library I use. These should almost always be implemented as compiler intrinsics anyway.
 *-------------------------------------------------------------------------------------------------------------------------*/

void *memset(void *buffer, int val, size_t num_bytes);
void *memcpy(void *dest, void const *src, size_t num_bytes);
void *memmove(void *dest, void const *src, size_t num_bytes);

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                       Error Handling
 *-------------------------------------------------------------------------------------------------------------------------*/

// Crash immediately, useful with a debugger!
#ifndef HARD_EXIT
  #define HARD_EXIT (*(int volatile*)0) 
#endif

#ifndef PanicIf
  #define PanicIf(assertion) StopIf((assertion), HARD_EXIT)
#endif

#ifndef Panic
  #define Panic() HARD_EXIT
#endif

#ifndef StopIf
  #define StopIf(assertion, error_action) if (assertion) { error_action; }
#endif

#ifndef Assert
  #ifndef NDEBUG
    #define Assert(assertion) if(!(assertion)) { HARD_EXIT; }
  #else
    #define Assert(assertion) (void)(assertion)
  #endif
#endif

/*---------------------------------------------------------------------------------------------------------------------------
 *                                                         Memory
 *---------------------------------------------------------------------------------------------------------------------------
 * Request big chunks of memory from the OS, bypassing the CRT. The system may round up your requested memory size, but it
 * will return an error instead of rounding down if there isn't enough memory.
 */
typedef struct
{
    void *mem;
    size size;
    b32 valid;
} MagMemoryBlock;

#define MAG_KB(a) ((a) * INT64_C(1000))
#define MAG_MB(a) (MAG_KB(a) * INT64_C(1000))
#define MAG_GB(a) (MAG_MB(a) * INT64_C(1000))
#define MAG_TB(a) (MAG_GB(a) * INT64_C(1000))

#define MAG_KiB(a) ((a) * (1024))
#define MAG_MiB(a) (MAG_KiB(a) * INT64_C(1024))
#define MAG_GiB(a) (MAG_MiB(a) * INT64_C(1024))
#define MAG_TiB(a) (MAG_GiB(a) * INT64_C(1024))

static inline MagMemoryBlock mag_sys_memory_allocate(size minimum_num_bytes);
static inline void mag_sys_memory_free(MagMemoryBlock *mem);

/*---------------------------------------------------------------------------------------------------------------------------
 *
 *
 *
 *                                          Implementation of `inline` functions.
 *                                                      Internal Only
 *
 *
 *
 *-------------------------------------------------------------------------------------------------------------------------*/

#if defined(_WIN32) || defined(_WIN64)

#pragma warning(disable: 4142)
#include "magpie_win32.h"
#pragma warning(default: 4142)

#elif defined(__linux__)

#include "magpie_linux.h"

#elif defined(__APPLE__)

#include "magpie_apple_osx.h"

#else
#error "Platform not supported by Coyote Library"
#endif

#pragma warning(pop)

#endif
