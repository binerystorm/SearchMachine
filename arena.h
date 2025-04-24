#include "glibc_log.h"
#include <sys/mman.h>
#ifndef _ARENA_H_
#define _ARENA_H_

//NOTE(gerick): arena meta data always stores the meta
// data for the current block. the first 8 bytes of the 
// current block store the start location of the previos
// block or NULL for no previous block. Previous blocks
// are always assumed to be full.
// NOTE(gerick): The `start_temp_region` field can never
// be zero as first 8 bytes of the arena always store the
// address to the previous block, or `NULL` in the case of
// there not being a preivios block.

typedef unsigned char uint8;
typedef unsigned long long uint64;
typedef unsigned long uint32;

struct Arena {

    // arena's meta data
    const size_t cap;
    size_t top;
    size_t start_temp_region;
    void* page_of_start_temp_region;

    // arena's current block
    void *data;
};

Arena arena_init();
void *arena_alloc(Arena *arena, size_t nbytes);
void arena_discard_temp(Arena *arena);
void arena_commit_temp(Arena *arena);
void arena_reset(Arena *arena);
void arena_unmap(Arena *arena);


#ifndef ARENA_IMPLEMTATION
Arena arena_init()
{
    void* data = (void*)mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if(data == MAP_FAILED){
        ERROR("No more memory can be allocated by the system. Program exiting...");
        exit(1);
    }
    *(void**)data = (uint64)NULL;
    return (Arena) {
        (size_t)getpagesize(),
        sizeof(void**),
        sizeof(void**),
        data,
        data
    };
}
static void *__arena_genral_alloc(Arena *arena, size_t nbytes)
{
    Assert(nbytes < (size_t)getpagesize(), "An arena cannot store a contiguis block of memory larger than the value returned by getpagesize()");
    uint8 used_bytes = arena->top % 8;
    uint8 dead_bytes = (8 - used_bytes) * (used_bytes != 0);

    if(arena->top + dead_bytes + nbytes >= arena->cap){
        void *prev_block = arena->data;
        // arena->cap = getpagesize();
        arena->top = sizeof(void**);

        void *data = (void*)mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if(data == MAP_FAILED){
            ERROR("No more memory can be allocated by the system. Program exiting...");
            exit(1);
        }

        arena->data = data;
        *(void**)arena->data = prev_block;
    } else {
        arena->top += dead_bytes;
    }
    void *ret = (void*) ((uint64)arena->data + (uint64)arena->top);
    arena->top += nbytes;
    Assert((uint64)ret % (uint64)8 == 0, "Make sure our allocation is 8 byte alighned");
    Assert(arena->top < arena->cap, "Make sure the arena size does not supercede its maximum size");
    return ret;
}

static void __arena_unmap_until(Arena *arena, void *until)
{
    Assert(arena->data != NULL, "Arena cannot be freed if it has no allocated data");
    Assert(arena->data != until, "No pages will be freed, I don't trust this");
    while(arena->data != until && arena->data != NULL){
        void *prev_page = *(void**)arena->data;
        Assert(munmap(arena->data, arena->cap) == 0, 
               "Any error that can occur during unmapping is a fault in the arena library. Error: %s", strerror(errno));
        arena->data = prev_page;
        arena->top = arena->cap;
    }
    Assert(!(arena->data == NULL && until != NULL), "Until pointer provided was not a valid page in the arena.");
}

void *arena_alloc(Arena *arena, size_t nbytes)
{
    return __arena_genral_alloc(arena, nbytes);
}

void arena_discard_temp(Arena *arena)
{
    Assert(arena->start_temp_region >= 8, "The discarding of a temporary region is smashing the reserved space for the pointer which points to the previos memory page. Something catastrophic has happend.");
    if(arena->page_of_start_temp_region != arena->data){
        __arena_unmap_until(arena, arena->page_of_start_temp_region);
    }
    arena->top = arena->start_temp_region;
}

void arena_commit_temp(Arena *arena)
{
    arena->start_temp_region = arena->top;
    arena->page_of_start_temp_region = arena->data;
}

void arena_reset(Arena *arena)
{
    Assert(*((void**)arena->data) == NULL, "TODO: implement arena resets for arena's with multiple memory pages");
    arena->start_temp_region = 0;
    arena->top = 8;
}

void arena_unmap(Arena *arena)
{
    Assert(arena->data != NULL, "cant free an arena that has not been initialized");
    __arena_unmap_until(arena, NULL);
    arena->top = 0;
    arena->start_temp_region = 0;
    arena->page_of_start_temp_region = NULL;
}

#endif // ARENA_IMPLEMTATION
#endif // _ARENA_H_
