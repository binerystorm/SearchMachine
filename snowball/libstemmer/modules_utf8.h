/* libstemmer/modules_utf8.h: List of stemming modules. */

#include "stem_UTF_8_english.h"

typedef enum {
  ENC_UNKNOWN=0,
  ENC_UTF_8
} stemmer_encoding_t;

struct stemmer_encoding {
  const char * name;
  stemmer_encoding_t enc;
};
static const struct stemmer_encoding encodings[] = {
  {"UTF_8", ENC_UTF_8},
  {0,ENC_UNKNOWN}
};

struct stemmer_modules {
  const char * name;
  stemmer_encoding_t enc; 
  struct SN_env * (*create)(void);
  void (*close)(struct SN_env *);
  int (*stem)(struct SN_env *);
};
static const struct stemmer_modules modules[] = {
  {"en", ENC_UTF_8, english_UTF_8_create_env, english_UTF_8_close_env, english_UTF_8_stem},
  {"eng", ENC_UTF_8, english_UTF_8_create_env, english_UTF_8_close_env, english_UTF_8_stem},
  {"english", ENC_UTF_8, english_UTF_8_create_env, english_UTF_8_close_env, english_UTF_8_stem},
  {0,ENC_UNKNOWN,0,0,0}
};
static const char * algorithm_names[] = {
  "english", 
  0
};
