#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
// TODO(gerick): create my own assertion function
#include <assert.h>

// TODO(gerick): 
// NOTE(gerick):
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

#define ArrayLen(ARR) sizeof((ARR))/sizeof((ARR)[0])
int main(){
    const char *file =  //"  hello  there hello do  duude\n there";
        slurp_file_or_panic("main.c");
    Str roam = {
        file,
        cstr_len(file)
    };
    // TODO(gerick): consider making this a hash table
    Str tokens[sizeof(Str)*1024*3] = {};

    while(roam.len > 0){
    // stripping white space
        for(;
            roam.len > 0 &&
            (*roam.data == ' ' ||
            *roam.data == '\n' ||
            *roam.data == '\t');
            // TODO(gerick): extract to function
            roam.data++, roam.len--);

        // pulling token out of stream
        // TODO(gerick): create more rigorouse tokenizer, rather than one that only works on white space
        const char * const token_start_loc = roam.data;
        for(;
            roam.len > 0 &&
            *roam.data != ' ' &&
            *roam.data != '\n' &&
            *roam.data != '\t';
            roam.data++, roam.len--);

        const Str token = {
            token_start_loc,
            (size_t)roam.data - (size_t)token_start_loc
        };

        // putting token in token set
        for(size_t cur = 0; 
        cur < ArrayLen(tokens);
        cur++)
        {
            if(tokens[cur].data == 0){
                tokens[cur].data = token.data;
                tokens[cur].len = token.len;
                break;
            }
            if(str_eq(tokens[cur], token)){
                break;
            }
        }
    }
    for(size_t cur = 0; cur < ArrayLen(tokens) && tokens[cur].data != 0; cur++){
        printf("(%.*s)\n", (int)tokens[cur].len, tokens[cur].data);
    }
    return 0;
}
