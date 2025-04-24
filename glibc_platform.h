#ifndef GLIBC_PLATFORM_H_
#define GLIBC_PLATFORM_H_
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
// TODO(gerick): make my own utilties for this
#include <string.h>

#include "arena.h"

struct ReadBuffer {
    const char * const data;
    const size_t cap;
    bool unmapped;
};


struct FixedArena {
    size_t cap;
    size_t top;
    void *data;
};

void die(int exit_code);
char **get_files_in_dir(Arena *arena, const char *path, size_t *file_count);

void get_stdin(char *buffer, const size_t buffer_len);

ReadBuffer slurp_file_or_panic(const char *path);
void unmap_buffer(ReadBuffer *buf);

FixedArena fixed_arena_init(size_t nbytes);
void *fixed_arena_alloc(FixedArena *arena, size_t nbytes);
void fixed_arena_discard(FixedArena *arena);
#endif // GLIBC_PLATFORM_H_
