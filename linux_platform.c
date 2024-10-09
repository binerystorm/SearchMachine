
//TODO(gerick): pull all file io, dubug, asserts and memmory handeling into platform layer
Buffer slurp_file_or_panic(const char *path)
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
    Buffer ret = {
        (char*)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0),
        (size_t)st.st_size,
        false
    };
    close(fd);
    return ret;
}

void unmap_buffer(Buffer *buf)
{
    munmap((void*)buf->data, buf->cap);
    buf->unmapped = true;
}

