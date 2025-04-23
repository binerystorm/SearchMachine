#include <stdio.h>
// TODO(gerick): write my own log aproximation funtion
#include <math.h>

// TODO(gerick): maybe make libstemmer work with my allocators
#include "libstemmer.h"
#include "glibc_platform.h"
#include "glibc_log.h"

#define ArrayLen(ARR) sizeof((ARR))/sizeof((ARR)[0])

typedef struct sb_stemmer Stemmer;
struct Str {
    const char *data;
    size_t len;
};

#define MAP_LEN 800

struct MapEntry {
    Str key;
    size_t val;
};

struct Map {
    FixedArena *entry_arena;
    MapEntry *entries;
    size_t len;
};

#include "glibc_log.c"
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

static inline bool is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool is_numeric(char c)
{
    return (c >= '0' && c <= '9');
}
static inline bool is_alphanumeric(char c)
{
    return is_alpha(c) || is_numeric(c);
}

static inline bool is_punctuation(char c)
{
    // TODO(gerick): make more robust punctuation filter
    return (c >= 33 && c <= 47) || c == '?' || c == ';' || c == '|';
}

static inline void str_shift_left(Str *str)
{
    str->data++;
    str->len--;
}

static MapEntry *map_get(Map *map, Str key)
{
    for(size_t map_idx = 0; map_idx < map->len; map_idx++){
        if(str_eq(map->entries[map_idx].key, key)){
            return &(map->entries[map_idx]);
        }
    }
    return NULL;
}

static void map_insert(Map *map, Str key, size_t val)
{
    MapEntry *new_entry = (MapEntry*)fixed_arena_alloc(map->entry_arena, sizeof(MapEntry));
    map->len += 1;

    new_entry->key = key;
    new_entry->val = val;
}

Map map_init(FixedArena *const arena)
{
    return (Map){
        arena,
        (MapEntry*)fixed_arena_alloc(arena, 0),
        0
    };
}


// TODO(gerick): Abstract lexing into a lexing of search terms
// and a lexing of file. This is neccesary as html will not need
// to be filtered out of a search term (in principle)
Str lex_next_token(Arena *arena, Stemmer *stemmer, Str *buffer)
{
    Assert(buffer->data != NULL, "lex_next_token expects a vallid buffer to operate on");
    for(; (buffer->len > 0) && 
        !is_alphanumeric(*(buffer->data));
        str_shift_left(buffer))
    {
        // TODO(gerick): Make better html parser
        // TODO(gerick): research whether this way of filtering out html tags will lose information
        if(*(buffer->data) == '<'){
            for(;(buffer->len > 0) &&
                (*(buffer->data) != '>');
                str_shift_left(buffer));
        }
    }

    const char *const token_start = buffer->data;
    for(; (buffer->len > 0) && is_alphanumeric(*buffer->data);
        str_shift_left(buffer));

    size_t token_len = (size_t)buffer->data - (size_t)token_start;

    if (token_len == 0){
        Assert(buffer->len == 0, "The buffer should be empty if we are returning an empty token");
        return (Str){NULL, 0};
    }

    char* token_buf = (char*)arena_alloc_temp(arena, token_len*sizeof(char));
    for(size_t i = 0; i < token_len; i++){
        char c = token_start[i];
        if(c >= 'A' && c <= 'Z') {
            c += 32;
        }
        token_buf[i] = c;
    }

    const sb_symbol *stemmed_token = sb_stemmer_stem(stemmer, (const sb_symbol*)token_buf, token_len);
    Assert(stemmed_token != NULL, "Stemmer ran out of memory");
    token_len = sb_stemmer_length(stemmer);
    arena_discard_temp(arena);
    arena_start_temp_region(arena);
    token_buf = (char*)arena_alloc_temp(arena, token_len);

    for(size_t i = 0; i < token_len; i++) token_buf[i] = stemmed_token[i];
    
    Str token = {
        token_buf,
        token_len
    };
    return token;
}

void parse_file(Arena *arena, Stemmer *stemmer, Str *roam_buffer, Map *map, Map *global_map, size_t *token_count)
{
    *token_count = 0;

    while(roam_buffer->len > 0){
        arena_start_temp_region(arena);
        Str token = lex_next_token(arena, stemmer, roam_buffer);
        if(token.data == NULL){ 
            Assert(roam_buffer->len == 0, "if lex token returns NULL the buffer should be empty");
            arena_discard_temp(arena);
            break;
        }

        (*token_count)++;
        MapEntry *local_entry = map_get(map, token);
        if(local_entry == NULL){
            MapEntry *global_entry;
            
            if ((global_entry = map_get(global_map, token)) == NULL){
                arena_commit_temp(arena);
                map_insert(global_map, token, 1);
                map_insert(map, token, 1);

            } else {
                global_entry->val += 1;
                map_insert(map, global_entry->key, 1);
                arena_discard_temp(arena);
            }
        }else{
            arena_discard_temp(arena);
            local_entry->val += 1;
        }
    }
}

void tfidf_search_and_print(Str *search_term, size_t search_term_len,
                      char** files, Map *maps, size_t *token_counts, size_t files_len,
                      Map *global_map)
{
    Assert(files_len > 0, "expect atleast one file to search through");
    bool marks[100] = {0};
    float ranks[500] = {0};
    // Ranking files
    for(size_t file_idx = 0; file_idx < files_len; file_idx++){
        // NOTE: file empty, nothing to see here
        if(token_counts[file_idx] == 0) continue;

        float rank = 0;
        for(size_t term_idx = 0; term_idx < search_term_len; term_idx++){
            Str term = search_term[term_idx];
            Map current_map = maps[file_idx];
            float term_freq = 0;
            float inverse_doc_freq = 0;
            MapEntry *map_entry = NULL;

            if((map_entry = map_get(&current_map, term)) != NULL){
                Assert(token_counts[file_idx] > 0, "this function assumes the file has lenth");
                term_freq = (float) map_entry->val / (float)token_counts[file_idx];
            }
            if((map_entry = map_get(global_map, term)) != NULL){
                Assert(map_entry->val > 0, "term occurence in the corpus should not be negative");
                inverse_doc_freq = logf((float)files_len / (float)map_entry->val);
            }
            rank += term_freq*inverse_doc_freq;
        }
        ranks[file_idx] = rank;
    }

    // Sorting file based on their rank.
    for(size_t result_num = 0; result_num < 10; result_num++){
        size_t idx_of_max = 0;
        for(size_t file_idx = 0; file_idx < files_len; file_idx++){
            if(ranks[file_idx] > ranks[idx_of_max] && !marks[file_idx]){
                idx_of_max = file_idx;
            }
        }
        marks[idx_of_max] = true;
        // TODO(gerick): replace with a platform layer function
        printf("%f: %s\n", ranks[idx_of_max], files[idx_of_max]);
    }
}

void search_and_print(Str *search_term, size_t search_term_len,
                      char** files, Map *maps, size_t files_len,
                      Map *global_map)
{

    // TODO(gerick): either give marks an arena or change the sorting 
    // algorithm to use linked lists.
    // NOTE(gerick): marks marks a index as used by the sorting algorithm
    // essentially removing it from the files list without the hasle of 
    // of removing thing from an array;
    bool marks[100] = {0};
    float ranks[500] = {0};
    printf("%zu\n", search_term_len);
    for(size_t i = 0; i < search_term_len; i++){
        printf("%.*s\n", (int)search_term[i].len, search_term[i].data);
    }
    for(size_t file_idx = 0; file_idx < files_len; file_idx++){
        float rank = 0;
        for(size_t term_idx = 0; term_idx < search_term_len; term_idx++){
            Str term = search_term[term_idx];
            Map current_map = maps[file_idx];
            float term_freq = 0;
            float inverse_term_freq = 0;
            MapEntry *map_entry = NULL;

            if((map_entry = map_get(&current_map, term)) != NULL){
                term_freq = (float) map_entry->val;
            }
            if((map_entry = map_get(global_map, term)) != NULL){
                inverse_term_freq = 1/((float) map_entry->val - term_freq);
            }
            rank += term_freq*inverse_term_freq;
        }
        ranks[file_idx] = rank;
    }

    // TODO(gerick): something fishy here
    // files are not sorted when ranks of files being compared are both zero
    for(size_t result_num = 0; result_num < 10; result_num++){
        size_t idx_of_max = 0;
        for(size_t file_idx = 0; file_idx < files_len; file_idx++){
            if(ranks[file_idx] > ranks[idx_of_max] && !marks[file_idx]){
                idx_of_max = file_idx;
            }
        }
        marks[idx_of_max] = true;
        // TODO(gerick): replace with a platform layer function
        printf("%f: %s\n", ranks[idx_of_max], files[idx_of_max]);
    }

}


int main(int argc, char **argv)
{
    if(argc > 1){
        struct sb_stemmer *stemmer = sb_stemmer_new("english", NULL); 
        const unsigned char *stemmed = sb_stemmer_stem(stemmer, (const sb_symbol*)argv[1], strlen(argv[1]));
        printf("%s\n", stemmed);
        printf("%.*s\n", sb_stemmer_length(stemmer), stemmed);
        sb_stemmer_delete(stemmer);
        // once again this is a test
        return 1;
    }

    FixedArena global_map_arena = fixed_arena_init(sizeof(Str)*1024*1024);
    FixedArena local_map_arena = fixed_arena_init(sizeof(Str)*1024*1024);
    FixedArena io_arena = fixed_arena_init(1024);
    Arena token_value_arena = arena_init();
    Arena file_name_arena = arena_init();
    Stemmer *stemmer = sb_stemmer_new("english", NULL); 

    size_t files_len = 0;
    INFO("Finding files...");
    char **files = get_files_in_dir(&file_name_arena, "./pygame-docs/ref/", &files_len);
    // float *ranks = (float*)arena_alloc(&file_name_arena, sizeof(float)*files_len);
    // TODO(gerick): consider making this a hash table
    Map global_map = map_init(&global_map_arena);
    Map *maps = (Map*)arena_alloc(&file_name_arena, sizeof(Map)*files_len);
    size_t *file_token_counts = (size_t*)arena_alloc(&file_name_arena, sizeof(size_t)*files_len);

    if(files == NULL){
        ERROR("No files available to be indexed, exiting...");
        die(1);
    }

    // indexing files
    for(size_t i = 0; i < files_len; i++){
        ReadBuffer file_buf = slurp_file_or_panic(files[i]);
        Str roam_buffer = {
            file_buf.data,
            file_buf.cap
        };
        maps[i] = map_init(&local_map_arena);
        Assert(maps[i].len == 0, "Map must have changed and must not be tampered with.");
        parse_file(&token_value_arena, stemmer, &roam_buffer, &(maps[i]), &global_map, &(file_token_counts[i]));
        unmap_buffer(&file_buf);
    }

    const size_t input_buffer_cap = 100;
    char *const input_buffer_start = (char*)fixed_arena_alloc(&io_arena, input_buffer_cap);
    Str input_buffer = {};
    input_buffer.len = 0;
    input_buffer.data = input_buffer_start;
    // TODO(gerick): make more explicit way of saying that the rest of the arena is for 
    // a some array in the arena system
    Str *search_term = (Str*)fixed_arena_alloc(&io_arena, 0);

    while(true){
        input_buffer.data = input_buffer_start;
        input_buffer.len = 0; 

        // TODO(gerick): replace with platform level function
        printf("> ");
        fflush(stdout);
        get_stdin((char*)input_buffer.data, input_buffer_cap, &(input_buffer.len));
        if(input_buffer.len == input_buffer_cap &&
            input_buffer.data[input_buffer.len - 1] != '\n'){
            WARN("Truncating inputed search string."
                   " Input may be a maximum of %zu (including enter)",
                   input_buffer_cap);
        }

        size_t search_term_len = 0;
        arena_start_temp_region(&token_value_arena);
        for(;;search_term_len++){
            
            search_term[search_term_len] = lex_next_token(&token_value_arena, stemmer, &input_buffer);
            if(search_term[search_term_len].data == NULL){
                break;
            } 
        }

        tfidf_search_and_print(search_term, search_term_len, 
                         files, maps, file_token_counts, files_len,
                         &global_map);

        arena_discard_temp(&token_value_arena);
    }

    fixed_arena_discard(&global_map_arena);
    fixed_arena_discard(&local_map_arena);
    fixed_arena_discard(&io_arena);
    arena_unmap(&token_value_arena);
    arena_unmap(&file_name_arena);
    sb_stemmer_delete(stemmer);
    return 0;
}
