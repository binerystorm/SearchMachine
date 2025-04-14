#include "glibc_platform.h"
#include "defer.h"

void _crash_after_flush()
{
    fflush(stdout);
    fflush(stderr);
    Assert(false, "TODO make better exit");
}
ReadBuffer slurp_file_or_panic(const char *path)
{
    struct stat st;
    int fd;
    int stat_exit_code;
    char* data;
    ReadBuffer failed = {0,0,0};

    if ((fd = open(path, O_RDONLY)) < 0){
        WARN("Skipping, could not open %s: %s", path, strerror(errno));
        return failed;
    }
    defer(
        close(fd);
    );
    // int stat_exit_code = stat(path, &st);
    if((stat_exit_code = stat(path, &st)) != 0){
        WARN("Skipping, could not retrieve file status %s: %s", path, strerror(errno));
        return(failed);
    }
    if ((st.st_mode & S_IFMT) != S_IFREG){
        INFO("Skipping %s is not a file", path);
        return(failed);
    }
    Assert(st.st_size >= 0, "file has negative size???");

    if((data = (char*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
        ERROR("Could not allocate memory: %s", strerror(errno));
        Assert(false, "TODO: make better exit system");
    }

    ReadBuffer ret = {
        data,
        (size_t)st.st_size,
        false
    };
    return(ret);
}

void unmap_buffer(ReadBuffer *buf)
{
    munmap((void*)buf->data, buf->cap);
    buf->unmapped = true;
}

char **get_files_in_dir(Arena *arena, const char *path, size_t *file_count)
{
    DIR *dir_hdl;
    struct dirent *entry;
    size_t entry_count = 0;

    if((dir_hdl = opendir(path)) == NULL){
        WARN("Could not open %s: %s", path, strerror(errno));
        return NULL;
    }
    defer(
        closedir(dir_hdl);
    );

    errno = 0;
    while((entry = readdir(dir_hdl)) != NULL){
        entry_count += 1;
    }
    Assert(errno == 0, "all `readdir` errors are mistakes in program"
           "and out of user control");

    char **file_list = (char**)arena_alloc(arena, sizeof(char**) * entry_count);
    rewinddir(dir_hdl);

    for(size_t i = 0; i < entry_count; i++){
        entry = readdir(dir_hdl);
        Assert(errno == 0 && entry != NULL, "We have already cheacked for errors, one should not occur now");
        // NOTE(gerick): +1 is for the NULL byte at the end of the cstr after concatination.
        const size_t path_len = strlen(path) + strlen(entry->d_name) + 1;
        file_list[i] = (char*)arena_alloc(arena, path_len);
        strcpy(file_list[i], path);
        strcat(file_list[i], entry->d_name);
    }
    *file_count = entry_count;
    return(file_list);
}

char **get_files_in_dir2(Arena *arena, const char *path, size_t *file_count)
{
    // TODO(gerick): handle errors properly
    DIR *dir_hdl;
    // long start_loc = telldir(dir_hdl);
    struct dirent *entry;
    struct stat st;
    size_t entry_count = 0;

    // dir_hdl = opendir(path);
    if((dir_hdl = opendir(path)) == NULL){
        WARN("Could not open %s: %s", path, strerror(errno));
        return NULL;
    }
    defer(
        closedir(dir_hdl);
    );
    errno = 0;
    while(true){
        entry = readdir(dir_hdl);
        Assert(errno == 0, "all `readdir` errors are mistakes in program"
               "and out of user control");
        if(entry == NULL){break;}

        const size_t path_len = strlen(path) + strlen(entry->d_name) + 1;
        char* file_name_buf = (char*)arena_alloc(arena, path_len);

        strcpy(file_name_buf, path);
        strcat(file_name_buf, entry->d_name);
        if(stat(file_name_buf, &st) != 0){
            continue;
            Assert((arena->top - sizeof(void**)) >= path_len, "");
            arena->top -= path_len;
        }
        if ((st.st_mode & S_IFMT) == S_IFREG){
            entry_count += 1;
        }
        Assert((arena->top - sizeof(void**)) >= path_len, "");
        arena->top -= path_len;
    }
    if(entry_count == 0){
        return(NULL);
    }

    char **file_list = (char**)arena_alloc(arena, entry_count * sizeof(char*));
    //seekdir(dir_hdl, start_loc);
    rewinddir(dir_hdl);
    for(size_t idx = 0; idx < entry_count;) {
        entry = readdir(dir_hdl);
        Assert(errno == 0 && entry != NULL, "");
        // NOTE(gerick): +1 is for the NULL byte at the end of the cstr after concatination.
        const size_t path_len = strlen(path) + strlen(entry->d_name) + 1;
        file_list[idx] = (char*)arena_alloc(arena, path_len);
        strcpy(file_list[idx], path);
        strcat(file_list[idx], entry->d_name);
        Assert(stat(file_list[idx], &st) == 0, "");
        if ((st.st_mode & S_IFMT) == S_IFREG){
            idx++;
        } else {
            //entry_count -= 1;
            Assert((arena->top - sizeof(void**)) >= path_len, "");
            arena->top -= path_len;
        }
    }
    *file_count = entry_count;
    return(file_list);
}

void get_stdin(char *buffer, size_t buffer_len, size_t *bytes_read)
{
    *bytes_read = read(0, buffer, buffer_len);
}

Arena arena_init()
{
    void* data = (void*)mmap(NULL, getpagesize(), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    *(void**)data = (uint64)NULL;
    return (Arena) {
        (size_t)getpagesize(),
        sizeof(void**),
        NULL,
        data
    };
}
void *__arena_genral_alloc(Arena *arena, size_t nbytes)
{
    Assert(nbytes < (size_t)getpagesize(), "An arena cannot store a contiguis block of memory larger than the value returned by getpagesize()");
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
    Assert((uint64)ret % (uint64)8 == 0, "Make sure our allocation is 8 byte alighned");
    Assert(arena->top < arena->cap, "Make sure the arena size does not supercede its maximum size");
    return ret;
}
// TODO(gerick): Handle all errors that can occure during mem_mapping
// and mem_unmapping

void *arena_alloc(Arena *arena, size_t nbytes)
{
    if(arena->temp_region != NULL){
        ERROR("An arena cannot be used for regulare allocating, if a temporary region is active.");
        // TODO(gerick): If we add temporary region growing, Edit this note to recommend it as an option.
        INFO("To fix this problem you can: Discard or commit the temporary region, and then procede to allocate normally.");
        _crash_after_flush();
    }

    return __arena_genral_alloc(arena, nbytes);
}
void *arena_alloc_temp(Arena *arena, size_t nbytes)
{
    if(arena->temp_region != NULL){
        ERROR("An arena cannot allocate new temporary region, if a temporary region is already active.");
        // TODO(gerick): If we add temporary region growing, Edit this note to recommend it as an option.
        INFO("To fix this problem you can: Discard or commit the previous temporary region, and then allocate new one");
        _crash_after_flush();
    }
    arena->temp_region = __arena_genral_alloc(arena, nbytes);
    return arena->temp_region;
}
// TODO(gerick): If a temporary region allocation overflows a memory page --
// causing the allacation to take place on a new page -- when discarding the
// region, do we want to restore the dead bytes in the old page and
// unmap the new page?
void arena_discard_temp(Arena *arena)
{
    if(arena->temp_region == NULL){
        ERROR("There is currently no temporary region to discard.");
        _crash_after_flush();
    }
    size_t temp_region_size = ((size_t)arena->data + arena->top) - (size_t)arena->temp_region;
    Assert((arena->top - sizeof(void**)) >= temp_region_size,
           "Discarding the temporary region is smashing the ptr point to the previous memory page. Somthing has gone horribly wrong");
    arena->top -= temp_region_size;
    arena->temp_region = NULL;
}

void arena_commit_temp(Arena *arena)
{
    if(arena->temp_region == NULL){
        ERROR("There is currently no temporary region to commit.");
        _crash_after_flush();
    }
    arena->temp_region = NULL;
}

// TODO(gerick): Add arena free function, to unmap whole arena

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
    Assert((arena->top + nbytes) < arena->cap, "Make sure the arena's new size does not excede the max size of the arena.");
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

