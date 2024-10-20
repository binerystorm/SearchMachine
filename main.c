#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
// TODO(gerick): create my own assertion function
//#include <assert.h>

#include "linux_platform.h"

#define ArrayLen(ARR) sizeof((ARR))/sizeof((ARR)[0])

struct Str {
    const char *data;
    size_t len;
};

//struct Arena {
//    char *mem;
//    size_t len;
//    size_t cur;
//};

#include "linux_platform.c"

size_t cstr_len(const char *str)
{
    size_t len = 0;
    for(; str[len] != 0; len++);
    return len;
}

bool str_eq(const Str lft, const Str rgt)
{
    if(lft.len != rgt.len){
        return false;
    }
    size_t cur = 0;
    for(cur = 0;
        cur < lft.len;
        cur++)
    {
        if(lft.data[cur] != rgt.data[cur]){return false;}
    }
    return true;
}

bool str_ncstr_eq(const Str lft, const char *rgt, const size_t rgt_len){
    if(lft.len != rgt_len){
        return false;
    }
    size_t cur = 0;
    for(cur = 0;
        cur < lft.len;
        cur++)
    {
        if(lft.data[cur] != rgt[cur]){return false;}
    }
    return true;
}

// TODO(gerick): pull arena's into platform layer
//Arena arena_init(size_t len)
//{
//    Arena ret = {
//        .mem = (char*) malloc(len),
//        .len = len,
//        .cur = 0
//    };
//    return ret;
//}
//char *arena_aloc(Arena *arena, const size_t n)
//{
//    assert(arena->len > arena->cur + n && "to little memmory in arena");
//    char *ret = arena->mem + arena->cur;
//    arena->cur += n;
//    return ret;
//}

static inline bool is_space(char c)
{  
    return c == ' ' ||
           c == '\n' ||
           c == '\t';
}

static inline bool is_punctuation(char c)
{
    // TODO(gerick): make more robust punctuation filter
    return (c >= 33 && c <= 47) || c == '?' || c == ';';
}

static inline void str_shift_left(Str *str){
    str->data++;
    str->len--;
}

void parse_file(Arena arena, Str roam_buffer, Str *token_map, size_t *freq_map, size_t map_len){
    // TODO(gerick): Make better html parser
    while(roam_buffer.len > 0){
        for(;
        (roam_buffer.len > 0) &&
        (is_space(*roam_buffer.data) ||
        is_punctuation(*roam_buffer.data) ||
        *roam_buffer.data == '<');
        str_shift_left(&roam_buffer))
        {
            // TODO(gerick): research whether this way of filtering out html tags will lose information
            if(*roam_buffer.data == '<'){
                for(;(roam_buffer.len > 0) &&
                    (*(roam_buffer.data) != '>');
                    str_shift_left(&roam_buffer));
            }
        }

        // pulling token out of stream
        const char * const token_start_loc = roam_buffer.data;
        for(;
            (roam_buffer.len > 0) &&
            (!is_space(*roam_buffer.data) && 
            !is_punctuation(*roam_buffer.data) &&
            *roam_buffer.data != '<');
            str_shift_left(&roam_buffer));

        // TODO(gerick): copy tokens into own memory buffer, rather than borrowing them from the roam_buffer
        // extracting simple form of tokan into external arena
        const size_t token_len = (size_t)roam_buffer.data - (size_t)token_start_loc;
        char* token_buf = (char*)arena_alloc(&arena, token_len);
        for(size_t i = 0; i < token_len; i++){
            char c = token_start_loc[i];
            if(c >= 'A' && c <= 'Z') {
                c += 32;
            }
            token_buf[i] = c;
        }
        Str token = {
            token_buf,
            token_len
        };
        for(size_t cur = 0;;cur++)
        {
            if(cur >= map_len){return;}
            if(token_map[cur].data == 0){
                token_map[cur].data = token.data;
                token_map[cur].len = token.len;
                freq_map[cur] = 1;
                break;
            }
            if(str_eq(token_map[cur], token)){
                // TODO(gerick): Create proper scratch buffer system
                // This is dangerous as no arena allocations may occure until
                // I have decided to keep this or throw it
                freq_map[cur] += 1;
                assert((arena.top - sizeof(void**)) >= token.len);
                arena.top -= token.len;
                break;
            }
        }
    }
}
int main()
{
    //Assert(1==2);
    const char *files[] = {
        "./pygame-docs/ref/bufferproxy.html",
        "./pygame-docs/ref/camera.html",
        "./pygame-docs/ref/cdrom.html",
    };
    // TODO(gerick): start thinking about memory sizes less arbitrerally
    //Arena arena = arena_init(1024*6);
    Arena arena = arena_init();
    // TODO(gerick): consider making this a hash table

#if 1
    for(size_t i = 0; i < ArrayLen(files); i++){
        Str tokens[sizeof(Str)*1024*3] = {};
        size_t freq[sizeof(size_t)*1024*3] = {};
        ReadBuffer file_buf = slurp_file_or_panic(files[i]);
        Str roam_buffer = {
            file_buf.data,
            file_buf.cap
        };
        parse_file(arena, roam_buffer, tokens, freq, ArrayLen(tokens));
        for(size_t cur = 0; cur < ArrayLen(tokens) && tokens[cur].data != 0; cur++){
            if(tokens[cur].len >= 20){continue;}
            //printf("(%.*s):(%zu)\n", (int)tokens[cur].len, tokens[cur].data, freq[cur]);
            printf("(%.*s)\n", (int)tokens[cur].len, tokens[cur].data);
        }
        //printf("\n");
        unmap_buffer(&file_buf);
    }
#endif
#if 0
    const char *buf = "hello duude<shit und drek> cool";
    Str tokens[sizeof(Str)*1024*3] = {};
    size_t freq[sizeof(size_t)*1024*3] = {};
    Str roam_buffer = {
            buf,
            cstr_len(buf)
    };
    parse_file(arena, roam_buffer, tokens, freq, ArrayLen(tokens));
    for(size_t cur = 0; cur < ArrayLen(tokens) && tokens[cur].data != 0; cur++){
        if(tokens[cur].len >= 20){continue;}
        printf("(%.*s):(%zu)\n", (int)tokens[cur].len, tokens[cur].data, freq[cur]);
    }
    printf("\n");
#endif 
    return 0;
}
