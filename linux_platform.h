#ifndef LINUX_PLATFORM_H_
#define LINUX_PLATFORM_H_
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
// TODO(gerick): create my own assertion function
#include <assert.h>

//#define NULL (long long int) 0

struct Buffer {
    const char * const data;
    const size_t cap;
    bool unmapped;
};

Buffer slurp_file_or_panic(const char *path);
void unmap_buffer(Buffer *buf);
#endif // LINUX_PLATFORM_H_
