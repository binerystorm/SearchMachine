#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
// TODO(gerick): create my own assertion function
#include <assert.h>

#define ArrayLen(ARR) sizeof((ARR))/sizeof((ARR)[0])

//TODO(gerick): make this function "better"
char *slurp_file_or_panic(const char *path)
{
    long str_s;
    FILE *f = fopen(path, "rb");
    if (f == NULL){
        fprintf(stderr, "failed to open file %s: %s\n", path, strerror(errno));
        exit(1);
    }
    assert(!(fseek(f, 0, SEEK_END) < 0));
    str_s = ftell(f);
    assert(!(str_s < 0));
    assert(!(fseek(f, 0, SEEK_SET) < 0));

    char *str = (char *)malloc(str_s+1);
    if(str == NULL){
        fprintf(stderr, "Could not allocate memory: %s\n", strerror(errno));
        exit(1);
    }
    fread(str, str_s, sizeof(char), f);
    if(ferror(f)){
        fprintf(stderr, "File could not be read: Unknown\n");
        exit(1);
    }
    if(fclose(f)){
        fprintf(stderr, "File could not be closed: %s\n", strerror(errno));
        exit(0);
    }
    str[str_s] = 0;

    return str;
}

struct Str {
    const char *data;
    size_t len;
};

struct Arena {
    char *mem;
    size_t len;
    size_t cur;
};

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

Arena arena_init(size_t len)
{
    Arena ret = {
        .mem = (char*) malloc(len),
        .len = len,
        .cur = 0
    };
    return ret;
}
char *arena_aloc(Arena *arena, const size_t n)
{
    assert(arena->len > arena->cur + n && "to little memmory in arena");
    char *ret = arena->mem + arena->cur;
    arena->cur += n;
    return ret;
}

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
        char* token_buf = arena_aloc(&arena, token_len);
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
                freq_map[cur] += 1;
                assert(arena.cur > token.len);
                arena.cur -= token.len;
                break;
            }
        }
    }
}

int main(){
    const char *files[] = {
        "./pygame-docs/ref/bufferproxy.html",
        "./pygame-docs/ref/camera.html",
        "./pygame-docs/ref/cdrom.html",
    };
    // TODO(gerick): start thinking about memory sizes less arbitrerally
    Arena arena = arena_init(1024*4);
    // TODO(gerick): consider making this a hash table

#if 1
    for(size_t i = 0; i < ArrayLen(files); i++){
        Str tokens[sizeof(Str)*1024*3] = {};
        size_t freq[sizeof(size_t)*1024*3] = {};
        char *file = slurp_file_or_panic(files[i]);
        Str roam_buffer = {
            file,
            cstr_len(file)
        };
        parse_file(arena, roam_buffer, tokens, freq, ArrayLen(tokens));
        for(size_t cur = 0; cur < ArrayLen(tokens) && tokens[cur].data != 0; cur++){
            if(tokens[cur].len >= 20){continue;}
            printf("(%.*s):(%zu)\n", (int)tokens[cur].len, tokens[cur].data, freq[cur]);
        }
        printf("\n");
        free(file);
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
