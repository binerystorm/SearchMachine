#include "linux_platform.h"
//
//TODO(gerick): pull all file io, dubug, asserts and memmory handeling into platform layer
ReadBuffer slurp_file_or_panic(const char *path)
{
    struct stat st;
    // TODO(gerick): handle errors correctly in this whole function
    int fd = open(path, O_RDONLY);
    if (fd < 0){
        assert(false && "opening file failed");
    }
    int stat_exit_code = stat(path, &st);
    if (stat_exit_code != 0){
        assert(false && "retrieving status of file failed");
    }
    assert(st.st_size >= 0 && "file has negative size???");
    ReadBuffer ret = {
        (char*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0),
        (size_t)st.st_size,
        false
    };
    close(fd);
    return ret;
}

void unmap_buffer(ReadBuffer *buf)
{
    munmap((void*)buf->data, buf->cap);
    buf->unmapped = true;
}

Arena arena_init()
{
    void* data = (void*)mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    *(void**)data = (uint64)NULL;
    return (Arena) {
        (size_t)getpagesize(),
        sizeof(void**),
        data
    };
}
void *arena_alloc(Arena *arena, size_t nbytes)
{
    // TODO(gerick): when nbytes overflows arena remaining space we alocate new block
    // to ensure struct are stored in contiguous memory however when allocating massive
    // amounts of memory, block becomes fragmented
    // TODO(gerick): Make better alignment system instead always alighning on 8 bytes
    // STUDY(gerick): How does memory alignment word and what are the negative effects of bad
    // alignment (at least on my machine)
    uint8 used_bytes = arena->top % 8;
    uint8 dead_bytes = (8 - used_bytes) * (used_bytes != 0);

    if(arena->top + dead_bytes + nbytes >= arena->cap){
        void *prev_block = arena->data;
        arena->cap = getpagesize();
        arena->top = sizeof(void**);
        arena->data = 
            (void*)mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        *(void**)arena->data = prev_block;
    } else {
        arena->top += dead_bytes;
    }
    void *ret = (void*) ((uint64)arena->data + (uint64)arena->top);
    arena->top += nbytes;
    assert((uint64)ret % (uint64)8 == 0);
    assert(arena->top < arena->cap);
    return ret;
}
// TODO(gerick): Add arena free function, to unmap whole arena
