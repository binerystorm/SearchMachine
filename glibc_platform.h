#ifndef LINUX_PLATFORM_H_
#define LINUX_PLATFORM_H_
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
// TODO(gerick): make my own utilties for this
#include <string.h>

// #define Assert(pred) do{if(!(pred)){*(char*)NULL = 0;}}while(0)
// TODO(gerick): maybe pull these typedefs into a comman types file
typedef unsigned char uint8;
typedef unsigned long long uint64;
typedef unsigned long uint32;

struct ReadBuffer {
    const char * const data;
    const size_t cap;
    bool unmapped;
};

//NOTE(gerick): arena meta data always stores the meta
// data for the current block. the first 8 bytes of the 
// current block store the start location of the previos
// block or NULL for no previous block. Previous blocks
// are always assumed to be full.
struct Arena {

    // arena's meta data
    size_t cap;
    size_t top;

    // arena's current block
    void *data;
};

struct FixedArena {
    size_t cap;
    size_t top;
    void *data;
};

char **get_files_in_dir(Arena *arena, const char *path, size_t *file_count);

void get_stdin(char *buffer, size_t buffer_len);

ReadBuffer slurp_file_or_panic(const char *path);
void unmap_buffer(ReadBuffer *buf);

Arena arena_init();
void *arena_alloc(Arena *arena, size_t nbytes);

FixedArena fixed_arena_init(size_t nbytes);
void *fixed_arena_alloc(FixedArena *arena, size_t nbytes);
void fixed_arena_discard(FixedArena *arena);
#endif // LINUX_PLATFORM_H_
