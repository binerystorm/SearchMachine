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
// NOTE(gerick): The `start_temp_region` field can never
// be zero as first 8 bytes of the arena always store the
// address to the previous block, or `NULL` in the case of
// there not being a preivios block.
struct Arena {

    // arena's meta data
    const size_t cap;
    size_t top;
    size_t start_temp_region;
    void* page_of_start_temp_region;

    // arena's current block
    void *data;
};

struct FixedArena {
    size_t cap;
    size_t top;
    void *data;
};

void die(int exit_code);
char **get_files_in_dir(Arena *arena, const char *path, size_t *file_count);

void get_stdin(char *buffer, size_t buffer_len);

ReadBuffer slurp_file_or_panic(const char *path);
void unmap_buffer(ReadBuffer *buf);


// TODO(gerick): Extract all arena code into its own library,
// It is cluttering thing op here 
// TODO(gerick): Simplify Arena api by implementing constant
// temporary region mode. Like this arena allcate will allways
// allcate on a temp region, and if you want to keep the temp
// region just commit it
Arena arena_init();
void *arena_alloc(Arena *arena, size_t nbytes);
void arena_start_temp_region(Arena *arena);
bool arena_temp_mode(Arena *arena);
void *arena_alloc_temp(Arena *arena, size_t nbytes);
void arena_discard_temp(Arena *arena);
void arena_commit_temp(Arena *arena);
void arena_reset(Arena *arena);
void arena_unmap(Arena *arena);

FixedArena fixed_arena_init(size_t nbytes);
void *fixed_arena_alloc(FixedArena *arena, size_t nbytes);
void fixed_arena_discard(FixedArena *arena);
#endif // GLIBC_PLATFORM_H_
