// File-System related functions that work on Unix and Windows
//
// Define UTIL_IMPLEMENTATION in some file, to include the function bodies
// Define UTIL_MALLOC to use a different allocator

#ifndef UTIL_H_
#define UTIL_H_

// @TODO: Allow use of custom allocators
#include <stdbool.h>  // For booleans
#include <stdint.h>   // For integer redefinitions
#include <stdlib.h>   // For memcpy (and malloc if not overwritten)
#include <fcntl.h>    // For file access flags
#include <sys/stat.h> // For file stats

////////////
// Macros //
////////////
#ifndef UTIL_MALLOC
#define UTIL_MALLOC(size) malloc(size)
#endif

#ifndef _UTIL_MACROS_
#define _UTIL_MACROS_

#define u8  uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t
#define i8  int8_t
#define i16 int16_t
#define i32 int32_t
#define i64 int64_t

// #if defined(_WIN32)
// 	#define mkdir(dir, mode) _mkdir(dir)
// 	#define open(name, ...) _open(name, __VA_ARGS__)
// 	#define read(fd, buf, count) _read(fd, buf, count)
// 	#define close(fd) _close(fd)
// 	#define write(fd, buf, count) _write(fd, buf, count)
// 	#define dup2(fd1, fd2) _dup2(fd1, fd2)
// 	#define unlink(file) _unlink(file)
// 	#define rmdir(dir) _rmdir(dir)
// 	#define getpid() _getpid()
// 	#define usleep(t) Sleep((t)/1000)
// 	#define sleep(t) Sleep((t)*1000)
// #endif

// @Note: All Macros expect the inputs to already be evaluated. On the implementation
// side of things, this means that the macros don't wrap their arguments in parantheses.
// The benefit of this is a potential performance increase (by not doing any computations
// several times in a macro) as well as guaranteed correctness (i.e. no bugs through doing
// `i++` repeatedly in a macro). The only downside is that the caller of macros might have
// to put parantheses around their arguments manually sometimes.
#define STRINGIZE2(x) #x
#define STRINGIZE(x) STRINGIZE2(x)
#define STR_LINE STRINGIZE(__LINE__)
#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define CLAMP(x, min, max) (x) > (max) ? (max) : (x) < (min) ? (min) : (x)
#define LERP(t, min, max) (min) + (t)*((max) - (min))
#define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#define LIKELY(expr)   __builtin_expect(!!(expr), 1)
#define SWAP(x, y) do { __typeof__(x) _swap_tmp_ = x; x = y; y = _swap_tmp_; } while (0)
#define PANIC(...) do { printf(__VA_ARGS__); printf("\n"); exit(1); } while (0)
#define TODO() do { printf("Hit TODO in " __FILE__ ":" STR_LINE "\n"); exit(1); } while(0)
#define UNREACHABLE() do { printf("Reached an unreachable place in " __FILE__ ":" STR_LINE "\n"); exit(1); } while(0)
#define STATIC_ASSERT_MSG(expr, msg) { extern int __attribute__((error("assertion failure: '" #msg "' in " __FILE__ ":" STR_LINE))) compile_time_check(); ((expr)?0:compile_time_check()),(void)0; }
#define STATIC_ASSERT(expr) STATIC_ASSERT_MSG(expr, #expr);
#define GLOBAL_STATIC_ASSERT(expr) extern int __attribute__((error("assertion failure: '" #expr "' in " __FILE__ ":" STR_LINE))) compile_time_check(); ((expr)?0:compile_time_check());

#endif // _UTIL_MACROS_

//////////////////
// Declarations //
//////////////////

void* util_memadd(const void *a, u64 a_size, const void *b, u64 b_size);
char* util_readFile(const char *fpath, u64 *size);
bool  util_writeFile(const char *fpath, char *buf, u64 size);


#endif // UTIL_H_


#ifdef UTIL_IMPLEMENTATION
#ifndef _UTIL_IMPL_GUARD_
#define _UTIL_IMPL_GUARD_

// Returns a new array, that contains first array a and then array b. Useful for adding strings for example
// a_size and b_size should both be the size in bytes, not the count of elements
void* util_memadd(const void *a, u64 a_size, const void *b, u64 b_size)
{
	char* out = UTIL_MALLOC(a_size * b_size);
	memcpy(out, a, a_size);
	memcpy(&out[a_size], b, b_size);
	return (void*) out;
}

char* util_readFile(const char *fpath, u64 *size)
{
    // Adapted from https://stackoverflow.com/a/68156485/13764271
    char* out = NULL;
    *size = 0;
    int fd = open(fpath, O_RDONLY, 0777);
    if (fd == -1) goto end;
    struct stat sb;
    if (stat(fpath, &sb) == -1) goto fd_end;
    if (sb.st_size == 0) goto fd_end;
    out = UTIL_MALLOC(sb.st_size);
    if (out == NULL) goto fd_end;
    if (read(fd, out, sb.st_size) == -1) goto fd_end;
    *size = (u64) sb.st_size;
fd_end:
    close(fd);
end:
    return out;
}

bool util_writeFile(const char *fpath, char *buf, u64 size)
{
    bool out = false;
    int fd = open(fpath, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd == -1) goto end;
    u64 written = 0;
    while (written < size) {
        int res = write(fd, &buf[written], size - written);
        if (res == -1) goto fd_end;
        written += res;
    }
    out = true;
fd_end:
    close(fd);
end:
    return out;
}

#endif // _UTIL_IMPL_GUARD_
#endif // UTIL_IMPLEMENTATION
