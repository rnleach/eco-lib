# eco-lib
My base, or "standard" library.

Eco is for ecosystem. This is my base, or "standard" library that I've broken up into some sub-libraries that are named after animals that I'm familiar with. Together, they form an ecosystem. The analogy is pretty loose. All of the libraries are written in C and targeted for the C11 standard. 

My overall philosphy is to keep it simple, but not too simple. If it's in the library, I needed it at some point. The libraries are each combined into single header libraries. I may at some point add a build step that makes one single eco-lib.h file with everything. Each library is described below.

 Goals and non-Goals:
  1. I only implement the things I need. If it's in here, I needed it at some point.

  2. Single header file. Keeps it as simple as possible, just drop 1 file into your source tree and 
     make sure it's configured in your build system. (See design notes section below.)

  3. NOT threadsafe. Access to any objects will need to be protected by the user of those objects
     with a mutex or other means to prevent data races.

  4. Reentrant functions and NO global mutable state. All state related to any objects created by
     this library should be stored in that object. No global state and reentrant functions makes it
     possible to use it in multithreaded applications so long as the user ensures the data they
     need protected is protected. 

  5. As a result of number 4, functions are thread safe so as long as the input parameters are
     protected from data races before being passed into the function, the function itself will not
     introduce any data races.

Some of the libraries depend on each other. Elk is the most basic and doesn't depend on anything else. Magpie depends on 
elk, Coyote and Packrat each depend on Magpie and Elk.

## Elk - A no standard, OS agnostic foundation.

  I like elk. They're really interesting, majestic animals. And did you know they have ivory?! 
 
  Does not rely on system specific code. Also trying to minimalize dependencies on the C runtime and the C standard library. However I sometimes need to reference functions in string.h (memcpy, memmove) and stdint.h (integer types with specified widths).

### Time
  The time related functions in this library do NOT account for local time or timezones. I almost always only need to work in the UTC timezone, so I don't worry about it. 

  Know your time and what you need from it! THE TIME FUNCTIONALITY IN THIS LIBRARY IS NOT WIDELY APPLICABLE. I don't even know if something like that exists.

  My use cases typically involve meteorological forecasts and/or observations. The current implementation of this library uses January 1st, 1 AD as the epoch. It cannot handle times before that. The maximum time that can be handled by all the functions is December 31st, 32767. So this more than covers the useful period of meteorological observations and forecasts.

### CSV Parser
  The CSV parser is simple and only handles quoted strings and comment lines. It just returns a token at a time, then the user can do what it wants with each token.

### Types
  I redefined many of the builtin types to be more succinct. So uint64_t is u64 and the like. I think many people find this annoying, but types are so ubiquitous it seems weird to make their names so long. Save the long names for custom or unusual types. Also some of the renames (like size and byte) convey intent better than ptrdiff_t and char.

### String slices
  Basic string slices. This amounts to a fat-pointer that includes the string length.

### Math
  I include random numbers, Kahan summation, and mathematical constants in this library because they do not depend on OS specific code.

### Error handling
  Includes some basic macros for error handling.

### Hash functions
  Later libraries (especially Packrat) will need hashing, but hashing itself is not OS dependent, so it's included here.

## Magpie - memory management.
Memory management library.

Magpies are very smart birds, with a very good memory. They can remember a place they found food once (like my deck) and return there intermittently for years to the exact spot they found it and check to see if there is food there again. In the case of my deck, they've been returning for over 5 years as I write this, even though it's been that long since I left unattended food out there. Researchers have also shown that they remember specific people and how they treated them, allowing people and magpies to form (good or bad) relationships.

This really has nothing to do with computers or software, but I thought the loose association was reason enough to name my memory management library Magpie.

### Goals
 - Provide a common allocator interface for use in my projects.
 - Provide a cross platform (Windows, Linux, Mac) layer for getting memory from the OS to bypass malloc/free.
 - Create a variety of allocators that spans from simple and fast to more complicated and general so I can choose the best allocator for any specific task.
 - Build static (pre-determined size) and dynamic (unbounded) allocators.
 - Track memory usage and make metrics available so I can potentially tune allocation block sizes for better performance in my applications.

### Non-goals
 - Create every allocator I *might eventually* need. I only build what I need, so if it's in there, I've used it.
 - Interoperate with malloc/free.


## Coyote - general platform layer.

A very basic platform library.

Coyotes are extremely adaptable animals. They've been in the Continental U.S. since before humans, and they've remained even when many other animals have been pushed out by urbanization. There are reports of coyotes living in Los Angeles! Since a platform library is about adapting to different environments, I thought I'd name mine after coyotes, since they are so adaptable.

I plan to support Windows, Mac OSX (BSD unix), and Linux (Ubuntu). The last two may just be considered POSIX for now, but I'm not commited to Posix.

### Retrieve info from the system
 Get the current time or terminal dimensions.

### File system
 Interact with the OS file system and manipulate paths. Also I have functions for reading and writing numbers and strings from binary files to support bespoke file formats.

### Shared Libraries
 Dynamic loading and unloading of shared libraries.

### Threading
 Threads, mutexes, condition variables and a threadsafe channel for passing data around different threads.

### Profiling
 Functions and macros for profiling a program.


## Packrat - collections.

 Data structures and functions for collections of objects. Think lists, arrays, etc. These collections may (or may not) be backed by dynamic memory allocators so that they can grow if needed. More limited collections are also available that rely on a fixed memory pool, so they can fill up. Some algorithms only make sense when applied to a collection, like sorting, so some of those algorithms are also implemented in this library.

   - String interner
   - Queues
   - Hash maps, some specialized for strings.
   - Dynamic arrays.
   - Radix sort.

