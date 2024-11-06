#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
// TODO(gerick): create my own assertion function
//#include <assert.h>

#include "glibc_platform.h"
#include "glibc_log.c"

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

#include "glibc_platform.c"

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
        // FIX(gerick): at the end of a file we register an empty token.
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

void search_and_print(Str *search_term, size_t search_term_len,
                      char** files, float *ranks, Map *maps, size_t files_len,
                      Map *global_map)
{

    // TODO(gerick): either give marks an arena or change the sorting 
    // algorithm to use linked lists.
    // NOTE(gerick): marks marks a index as used by the sorting algorithm
    // essentially removing it from the files list without the hasle of 
    // of removing thing from an array;
    bool marks[100] = {};

    for(size_t file_idx = 0; file_idx < files_len; file_idx++){
        float rank = 0;
        for(size_t term_idx = 0; term_idx <= search_term_len; term_idx++){
            Str term = search_term[term_idx];
            Map current_map = maps[file_idx];
            float term_freq = 0;
            float inverse_term_freq = 0;
            size_t *map_val = NULL;

            if((map_val = map_get(&current_map, term)) != NULL){
                term_freq = (float) *map_val;
            }
            if((map_val = map_get(global_map, term)) != NULL){
                inverse_term_freq = 1/((float) *map_val - term_freq);
            }
            rank += term_freq*inverse_term_freq;
        }
        ranks[file_idx] = rank;
    }

    // TODO(gerick): file not marked when rank is supposed to be zero
    for(size_t result_num = 0; result_num < 10; result_num++){
        size_t idx_of_max = 0;
        for(size_t file_idx = 0; file_idx < files_len; file_idx++){
            if(ranks[file_idx] > ranks[idx_of_max] && !marks[file_idx]){
                idx_of_max = file_idx;
            }
        }
        marks[idx_of_max] = true;
        printf("%f: %s\n", ranks[idx_of_max], files[idx_of_max]);
    }

}

int main()
{
    ERROR("hello world %d", 69);
    WARN("This is a %s", "warning");
    INFO("Vex.x = %f", 1.4356f);
    Assert(false, "Does it work %d", 69);
    FixedArena global_keys_arena = fixed_arena_init(sizeof(Str)*1024*1024);
    FixedArena global_vals_arena = fixed_arena_init(sizeof(size_t)*1024*1024);
    FixedArena local_keys_arena = fixed_arena_init(sizeof(Str)*1024*1024);
    FixedArena local_vals_arena = fixed_arena_init(sizeof(size_t)*1024*1024);
    FixedArena io_arena = fixed_arena_init(1024);
    Arena token_value_arena = arena_init();
    Arena file_name_arena = arena_init();

    size_t files_len = 0;
    char **files = get_files_in_dir(&file_name_arena, "./pygame-docs/ref/", &files_len);
    float *ranks = (float*)arena_alloc(&file_name_arena, sizeof(float)*files_len);
    // TODO(gerick): consider making this a hash table
    Map global_map = map_init(&global_keys_arena, &global_vals_arena);
    Map *maps = (Map*)arena_alloc(&file_name_arena, sizeof(Map)*files_len);

    if(files == NULL){
        printf("[ERR] no files available to be indexed, exiting...\n");
        fflush(stdout);
        assert(0);
    }

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
        unmap_buffer(&file_buf);
    }

    const size_t input_buffer_cap = 100;
    size_t input_buffer_len = 0;
    char *input_buffer = (char*)fixed_arena_alloc(&io_arena, input_buffer_cap);
    // TODO(gerick): make more explicit way of saying that the rest of the arena is for 
    // a some array in the arena system
    Str *search_term = (Str*)fixed_arena_alloc(&io_arena, 0);

    while(true){
        size_t search_term_idx = 0;
        printf("> ");
        fflush(stdout);
        get_stdin(input_buffer, input_buffer_cap, &input_buffer_len);
        if(input_buffer_len == input_buffer_cap &&
            input_buffer[input_buffer_len - 1] != '\n'){
            printf("Warning! tuncating inputed search string."
                   " Input may be a maximum of %zu (including enter)",
                   input_buffer_cap);
        }

        for(size_t input_buffer_idx = 0;
            input_buffer_idx < input_buffer_len;
            input_buffer_idx++)
        {
            Str *current_term = &(search_term[search_term_idx]);
            for(;is_space(input_buffer[input_buffer_idx]) &&
                    input_buffer_idx < input_buffer_len;
                input_buffer_idx++);
            current_term->data = &(input_buffer[input_buffer_idx]);
            for(;!is_space(input_buffer[input_buffer_idx]) &&
                    input_buffer_idx < input_buffer_len;
                input_buffer_idx++);
            current_term->len = 
                (size_t)&(input_buffer[input_buffer_idx]) - (size_t)current_term->data;

            search_term_idx += 1;
        }

        search_and_print(search_term, search_term_idx + 1, 
                         files, ranks, maps, files_len,
                         &global_map);
    }

    fixed_arena_discard(&global_keys_arena);
    fixed_arena_discard(&global_vals_arena);
    fixed_arena_discard(&local_keys_arena);
    fixed_arena_discard(&local_vals_arena);
    fixed_arena_discard(&io_arena);
    return 0;
}
