#include "glibc_platform.h"
#include "defer.h"

void die(int exit_code)
{
    exit(exit_code);
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
        die(1);
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


void get_stdin(char *buffer, size_t buffer_len, size_t *bytes_read)
{
    *bytes_read = read(0, buffer, buffer_len);
}

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

