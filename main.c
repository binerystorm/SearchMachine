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

// #define NORMAL_MAP_LEN 600
// #define GLOBAL_MAP_LEN 800
#define MAP_LEN 800

// NOTE(gerick): the keys and vals arrays should have the same capacity and length
struct Map {
    Str keys[MAP_LEN];
    size_t vals[MAP_LEN];
    size_t cap;
    size_t len;
};

#include "linux_platform.c"

#define STR(cstr) (Str){(cstr), cstr_len((cstr))}
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

static size_t *map_get(Map *map, Str key)
{
    for(size_t map_idx = 0; map_idx < map->len; map_idx++){
        if(str_eq(map->keys[map_idx], key)){
            return &(map->vals[map_idx]);
        }
    }
    return NULL;
}

// void parse_file(Arena *arena, Str roam_buffer, Str *token_map, size_t *freq_map, size_t map_len,
//                 Str *global_token_map, size_t *global_freq_map, size_t global_map_len){
void parse_file(Arena *arena, Str roam_buffer, Map *map, Map *global_map)
{
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
        char* token_buf = (char*)arena_alloc(arena, token_len);
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
            if(cur >= global_map->cap){return;}
            if(global_map->keys[cur].data == 0){
                global_map->keys[cur].data = token.data;
                global_map->keys[cur].len = token.len;
                global_map->vals[cur] = 1;
                global_map->len++;
                break;
            }
            if(str_eq(global_map->keys[cur], token)){
                // TODO(gerick): Create proper scratch buffer system
                global_map->vals[cur] += 1;
                assert((arena->top - sizeof(void**)) >= token.len);
                arena->top -= token.len;
                token.data = global_map->keys[cur].data;
                break;
            }
        }
        for(size_t cur = 0;;cur++)
        {
            if(cur >= map->cap){return;}
            if(map->keys[cur].data == 0){
                map->keys[cur].data = token.data;
                map->keys[cur].len = token.len;
                map->vals[cur] = 1;
                map->len++;
                break;
            }
            if(str_eq(map->keys[cur], token)){
                map->vals[cur] += 1;
                break;
            }
        }
    }
}
int main()
{
    //Assert(1==2);
    // TODO(gerick): consider making this a hash table
    // Str global_tokens[GLOBAL_MAP_LEN] = {};
    // size_t global_freqs[GLOBAL_MAP_LEN] = {};
    // Str file1_tokens[NORMAL_MAP_LEN] = {};
    // Str file2_tokens[NORMAL_MAP_LEN] = {};
    // Str file3_tokens[NORMAL_MAP_LEN] = {};
    // size_t file1_freqs[NORMAL_MAP_LEN] = {};
    // size_t file2_freqs[NORMAL_MAP_LEN] = {};
    // size_t file3_freqs[NORMAL_MAP_LEN] = {};

    char search_term[][10] = {
        "the",
        "cdrom",
        "is",
        "often",
        "important",
    };
    
    const char *files[] = {
        "./pygame-docs/ref/bufferproxy.html",
        "./pygame-docs/ref/camera.html",
        "./pygame-docs/ref/cdrom.html",
    };
    Map global_map = {};
    global_map.cap = MAP_LEN;
    Map maps[3] = {};
    // size_t *freq_maps[3] = {
    //     file1_freqs,
    //     file2_freqs,
    //     file3_freqs,
    // };
    // Str *token_maps[3] = {
    //     file1_tokens,
    //     file2_tokens,
    //     file3_tokens,
    // };
    Arena arena = arena_init();

    // indexing files
    assert(global_map.len == 0);
    for(size_t i = 0; i < ArrayLen(files); i++){
        // TODO(gerick): consider different way of asset management
        ReadBuffer file_buf = slurp_file_or_panic(files[i]);
        Str roam_buffer = {
            file_buf.data,
            file_buf.cap
        };
        maps[i].cap = MAP_LEN;
        assert(maps[i].len == 0);
        parse_file(&arena, roam_buffer, &(maps[i]), &global_map);
        // parse_file(&arena, roam_buffer, token_maps[i], freq_maps[i], NORMAL_MAP_LEN,
        //            global_tokens, global_freqs, GLOBAL_MAP_LEN);
        for(size_t cur = 0; cur < maps[i].cap && maps[i].keys[cur].data != 0; cur++){
            printf("(%.*s):(%zu)\n", (int)maps[i].keys[cur].len, maps[i].keys[cur].data, maps[i].vals[cur]);
            // printf("(%.*s)\n", (int)token_maps[i][cur].len, token_maps[i][cur].data);
        }
        printf("================================================================\n");
        unmap_buffer(&file_buf);
    }
    for(size_t cur = 0; cur < global_map.cap && global_map.keys[cur].data != 0; cur++){
            printf("(%.*s):(%zu)\n", (int)global_map.keys[cur].len, global_map.keys[cur].data, global_map.vals[cur]);
    }

    printf("================================================================\n\n");
    printf("search term: ");
    for (size_t i = 0; i < ArrayLen(search_term); i++){
        printf("%s ", search_term[i]);
    }
    printf("\n");

    // searching index
    for(size_t file_idx = 0; file_idx < ArrayLen(files); file_idx++){
        float rank = 0;
        for(size_t term_idx = 0; term_idx < ArrayLen(search_term); term_idx++){
            char *term = search_term[term_idx];
            Map current_map = maps[file_idx];
            float term_freq = 0;
            float inverse_term_freq = 0;
            size_t *map_val = NULL;

            if((map_val = map_get(&current_map, STR(term))) != NULL){
                term_freq = (float) *map_val;
            }
            if((map_val = map_get(&global_map, STR(term))) != NULL){
                inverse_term_freq = 1/((float) *map_val);
            }
            //for(size_t map_idx = 0; map_idx < maps[file_idx].len; map_idx++){
            //    if(str_eq(current_map.keys[map_idx], STR(term))){
            //        term_freq = (float)current_map.vals[map_idx];
            //        break;
            //    }
            //}
            //for(size_t map_idx = 0; map_idx < global_map.len; map_idx++){
            //    if(str_eq(global_map.keys[map_idx], STR(term))){
            //        inverse_term_freq = 1/((float)global_map.vals[map_idx]);
            //        break;
            //    }
            //}
            rank += term_freq*inverse_term_freq;
        }
        printf("%s: %f\n", files[file_idx], rank);
    }
    return 0;
}
