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

#define MAP_LEN 800

// NOTE(gerick): the keys and vals arrays should have the same capacity and length
struct Map {
    FixedArena *keys_arena;
    FixedArena *vals_arena;
    Str *keys;
    size_t *vals;
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

static inline void str_shift_left(Str *str)
{
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

static void map_insert(Map *map, Str key, size_t val)
{
    Str *new_key = (Str*)fixed_arena_alloc(map->keys_arena, sizeof(Str));
    size_t *new_val = (size_t*)fixed_arena_alloc(map->vals_arena, sizeof(size_t));

    assert((Str*)((size_t)map->keys + (map->len * sizeof(Str))) == new_key);
    assert((size_t*)((size_t)map->vals + (map->len * sizeof(size_t))) == new_val);
    map->len += 1;

    *new_key = key;
    *new_val = val;
}

Map map_init(FixedArena *const keys_arena, FixedArena *const vals_arena)
{
    return (Map){
        keys_arena,
        vals_arena,
        (Str*)fixed_arena_alloc(keys_arena, 0),
        (size_t*)fixed_arena_alloc(vals_arena, 0),
        0
    };
}

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
        size_t *global_val = map_get(global_map, token);
        if(global_val == NULL){
            map_insert(global_map, token, 1);
            map_insert(map, token, 1);
            continue;
        } else {
            *global_val += 1;
            size_t *local_val = map_get(map, token);
            if(local_val == NULL){
                map_insert(map, token, 1);
            } else {
                *local_val += 1;
            }
            // TODO(gerick): Create proper scratch buffer system
            assert((arena->top - sizeof(void**)) >= token.len);
            arena->top -= token.len;
        }
    }
}

int main()
{

    FixedArena global_keys_arena = fixed_arena_init(sizeof(Str)*1024*1024);
    FixedArena global_vals_arena = fixed_arena_init(sizeof(size_t)*1024*1024);
    FixedArena local_keys_arena = fixed_arena_init(sizeof(Str)*1024*1024);
    FixedArena local_vals_arena = fixed_arena_init(sizeof(size_t)*1024*1024);
    Arena token_value_arena = arena_init();
    Arena file_name_arena = arena_init();

    char search_term[][10] = {
        "how", "to", "draw", "a", "rectangle", "to", "the", "screen"
    };
    
    size_t files_len = 0;
    char **files = get_files_in_dir(&file_name_arena, "./pygame-docs/ref/", &files_len);
    //const char *files[] = {
    //    "./pygame-docs/ref/bufferproxy.html",
    //    "./pygame-docs/ref/camera.html",
    //    "./pygame-docs/ref/cdrom.html",
    //    "./pygame-docs/ref/color_list.html",
    //    "./pygame-docs/ref/display.html",
    //    "./pygame-docs/ref/draw.html",
    //    "./pygame-docs/ref/event.html",
    //};
    // TODO(gerick): consider making this a hash table
    Map global_map = map_init(&global_keys_arena, &global_vals_arena);
    Map *maps = (Map*)arena_alloc(&file_name_arena, sizeof(Map)*files_len);

    // indexing files
    for(size_t i = 0; i < files_len; i++){
        // TODO(gerick): consider different way of asset management
        ReadBuffer file_buf = slurp_file_or_panic(files[i]);
        Str roam_buffer = {
            file_buf.data,
            file_buf.cap
        };
        maps[i] = map_init(&local_keys_arena, &local_vals_arena);
        assert(maps[i].len == 0);
        parse_file(&token_value_arena, roam_buffer, &(maps[i]), &global_map);
        for(size_t cur = 0; cur < maps[i].len; cur++){
            printf("(%.*s):(%zu)\n", (int)maps[i].keys[cur].len, maps[i].keys[cur].data, maps[i].vals[cur]);
            // printf("(%.*s)\n", (int)token_maps[i][cur].len, token_maps[i][cur].data);
        }
        printf("================================================================\n");
        unmap_buffer(&file_buf);
    }
    for(size_t cur = 0; cur < global_map.len; cur++){
            printf("(%.*s):(%zu)\n", (int)global_map.keys[cur].len, global_map.keys[cur].data, global_map.vals[cur]);
    }

    printf("================================================================\n\n");
    printf("search term: ");
    for (size_t i = 0; i < ArrayLen(search_term); i++){
        printf("%s ", search_term[i]);
    }
    printf("\n");

    // searching index
    for(size_t file_idx = 0; file_idx < files_len; file_idx++){
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
            // printf("%s:\n", term);
            // printf("\tterm frequency: %f\n", term_freq);
            // printf("\tinverse document frequency: %f\n", inverse_term_freq);
            rank += term_freq*inverse_term_freq;
        }
        printf("%s: %f\n", files[file_idx], rank);
    }
    fixed_arena_discard(&global_keys_arena);
    fixed_arena_discard(&global_vals_arena);
    fixed_arena_discard(&local_keys_arena);
    fixed_arena_discard(&local_vals_arena);
    return 0;
}
