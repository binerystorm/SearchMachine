#include "linux_platform.h"

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

char **get_files_in_dir(Arena *arena, const char *path, size_t *file_count)
{
    // TODO(gerick): handle errors properly
    DIR *dir_hdl = opendir(path);
    long start_loc = telldir(dir_hdl);
    struct dirent *entry;
    struct stat st;
    size_t entry_count = 0;
    errno = 0;
    while(true){
        entry = readdir(dir_hdl);
        assert(errno == 0);
        if(entry == NULL){break;}

        const size_t path_len = strlen(path) + strlen(entry->d_name) + 1;
        char* file_name_buf = (char*)arena_alloc(arena, path_len);

        strcpy(file_name_buf, path);
        strcat(file_name_buf, entry->d_name);
        assert(stat(file_name_buf, &st) == 0);
        if ((st.st_mode & S_IFMT) == S_IFREG){
            entry_count += 1;
        }
        assert((arena->top - sizeof(void**)) >= path_len);
        arena->top -= path_len;
    }
    if(entry_count == 0){
        return NULL;
    }

    char **file_list = (char**)arena_alloc(arena, entry_count * sizeof(char*));
    seekdir(dir_hdl, start_loc);
    for(size_t idx = 0; idx < entry_count;) {
        entry = readdir(dir_hdl);
        assert(errno == 0 && entry != NULL);
        // NOTE(gerick): +1 is for the NULL byte at the end of the cstr after concatination.
        const size_t path_len = strlen(path) + strlen(entry->d_name) + 1;
        file_list[idx] = (char*)arena_alloc(arena, path_len);
        strcpy(file_list[idx], path);
        strcat(file_list[idx], entry->d_name);
        assert(stat(file_list[idx], &st) == 0);
        if ((st.st_mode & S_IFMT) == S_IFREG){
            idx++;
        } else {
            //entry_count -= 1;
            assert((arena->top - sizeof(void**)) >= path_len);
            arena->top -= path_len;
        }
    }
    *file_count = entry_count;
    return file_list;
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
// TODO(gerick): Handle all errors that can occure during mem_mapping
// and mem_unmapping

FixedArena fixed_arena_init(size_t nbytes)
{
    return {
        nbytes,
        0,
        mmap(NULL, nbytes, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)
    };
}
void *fixed_arena_alloc(FixedArena *arena, size_t nbytes)
{
    // TODO(gerick): replace this with a log message or maybe a returned error
    // so the rest of the program can handle it
    // NOTE(gerick): test code!!! relies on the user only allocating one data
    // type per arena
    assert((arena->top + nbytes) < arena->cap);
    void *ret = (void*)((uint64)arena->data + (uint64)arena->top);
    arena->top += nbytes;
    return ret;
}
void fixed_arena_discard(FixedArena *arena)
{
    munmap(arena->data, arena->cap);
    arena->cap = 0;
    arena->top = 0;
    arena->data = NULL;
}

