#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "assemble.h"
#include "errors.h"
#include "sv.h"

#define TODO assert(0 && "TO IMPLEMENT")

typedef enum {
  T_NONE,
  T_SYM,
  T_HEX,
  T_HEX2,
  T_MACROO,
  T_MACROC,
  T_COLON,
  T_REL,
  T_GLOBAL,
  T_EXTERN,
  T_STRING,
  T_ALIGN,
  T_DB,
  T_INT
} token_kind_t;
#define TOKEN_KIND_MAX_LEN 6

typedef struct {
  token_kind_t kind;
  sv_t image;
  location_t loc;
} token_t;

typedef struct {
  char *buffer;
  location_t loc;
} tokenizer_t;

char *token_kind_to_string(token_kind_t kind) {
  switch (kind) {
    case T_NONE:   assert(0);
    case T_SYM:    return "SYM";
    case T_HEX:    return "HEX";
    case T_HEX2:   return "HEX2";
    case T_MACROO: return "MACROO";
    case T_MACROC: return "MACROC";
    case T_COLON:  return "COLON";
    case T_REL:    return "REL";
    case T_GLOBAL: return "GLOBAL";
    case T_EXTERN: return "EXTERN";
    case T_STRING: return "STRING";
    case T_ALIGN:  return "ALIGN";
    case T_DB:     return "DB";
    case T_INT:    return "INT";
  }
  assert(0);
}

void token_dump(token_t token) {
  printf(LOCATION_FMT " %-*.*s '" SV_FMT "'\n",
         LOCATION_UNPACK(token.loc),
         TOKEN_KIND_MAX_LEN,
         TOKEN_KIND_MAX_LEN,
         token_kind_to_string(token.kind),
         SV_UNPACK(token.image));
}

void tokenizer_init(tokenizer_t *tokenizer, char *buffer, char *filename) {
  assert(tokenizer);
  assert(buffer);
  *tokenizer = (tokenizer_t){
    buffer,
    {filename, buffer, 1, 1, 0}
  };
}

token_t token_next(tokenizer_t *tokenizer) {
  assert(tokenizer);

  token_t token = {0};

  token_kind_t table[128] = {0};
  table['{'] = T_MACROO;
  table['}'] = T_MACROC;
  table[':'] = T_COLON;
  table['$'] = T_REL;

  switch (*tokenizer->buffer) {
    case ' ':
    case '\t':
      ++tokenizer->buffer;
      ++tokenizer->loc.col;
      return token_next(tokenizer);
    case '\r': ++tokenizer->buffer; return token_next(tokenizer);
    case '\n':
      ++tokenizer->buffer;
      tokenizer->loc.row_start = tokenizer->buffer;
      ++tokenizer->loc.row;
      tokenizer->loc.col = 1;
      return token_next(tokenizer);
    case '{':
    case '}':
    case ':':
    case '$':
      assert(table[(int)*tokenizer->buffer]);
      tokenizer->loc.len = 1;
      token = (token_t){
        table[(int)*tokenizer->buffer],
        {tokenizer->buffer, 1},
        tokenizer->loc
      };
      ++tokenizer->buffer;
      ++tokenizer->loc.col;
      break;
    case '"':
      {
        char *start = tokenizer->buffer;
        int len = 1;
        ++tokenizer->buffer;
        while (*tokenizer->buffer != '"') {
          ++len;
          ++tokenizer->buffer;
        }
        ++tokenizer->buffer;
        ++len;
        tokenizer->loc.len = len;
        token = (token_t){
          T_STRING,
          {start, len},
          tokenizer->loc
        };
        tokenizer->loc.col += len;
      }
      break;
    default:
      if (isalpha(*tokenizer->buffer) || *tokenizer->buffer == '_') {
        char *start = tokenizer->buffer;
        int len = 0;
        while (isalpha(*tokenizer->buffer) || *tokenizer->buffer == '_') {
          ++len;
          ++tokenizer->buffer;
        }
        tokenizer->loc.len = len;
        sv_t image = {start, len};
        token = (token_t){sv_eq(image, sv_from_cstr("GLOBAL"))   ? T_GLOBAL
                          : sv_eq(image, sv_from_cstr("EXTERN")) ? T_EXTERN
                          : sv_eq(image, sv_from_cstr("ALIGN"))  ? T_ALIGN
                          : sv_eq(image, sv_from_cstr("DB"))     ? T_DB
                                                                 : T_SYM,
                          image,
                          tokenizer->loc};
        tokenizer->loc.col += len;
      } else if (*tokenizer->buffer == '0' && (*(tokenizer->buffer + 1) == 'x' || *(tokenizer->buffer + 1) == 'X')) {
        char *start = tokenizer->buffer;
        int len = 0;
        tokenizer->buffer += 2;
        while (isdigit(*tokenizer->buffer) || ('a' <= *tokenizer->buffer && *tokenizer->buffer <= 'f') ||
               ('A' <= *tokenizer->buffer && *tokenizer->buffer <= 'F')) {
          ++len;
          ++tokenizer->buffer;
        }
        tokenizer->loc.len = len + 2;
        if (len != 2 && len != 4) {
          eprintfloc(tokenizer->loc, "invalid HEX");
        }
        token = (token_t){
          len == 2 ? T_HEX : T_HEX2,
          {start, len + 2},
          tokenizer->loc
        };
        tokenizer->loc.col += len;
      } else if (isdigit(*tokenizer->buffer)) {
        char *start = tokenizer->buffer;
        int len = 0;
        while (isdigit(*tokenizer->buffer)) {
          ++len;
          ++tokenizer->buffer;
        }
        tokenizer->loc.len = len;
        token = (token_t){
          T_INT,
          {start, len},
          tokenizer->loc
        };
        tokenizer->loc.col += len;
      } else if (*tokenizer->buffer == '-' && *(tokenizer->buffer + 1) == '-') {
        while (*tokenizer->buffer != '\n') {
          ++tokenizer->buffer;
          ++tokenizer->loc.col;
        }
        return token_next(tokenizer);
      } else {
        tokenizer->loc.len = 1;
        eprintfloc(tokenizer->loc, "invalid char");
      }
  }

  return token;
}

void token_compile(token_t token, obj_t *obj) {
  assert(obj);

  switch (token.kind) {
    case T_SYM:    TODO;
    case T_MACROO: TODO;
    case T_MACROC: TODO;
    case T_GLOBAL: TODO;
    case T_EXTERN: TODO;
    case T_ALIGN:  TODO;
    case T_DB:     TODO;
    case T_NONE:
    case T_HEX:
    case T_HEX2:
    case T_COLON:
    case T_REL:
    case T_STRING:
    case T_INT:    assert(0);
  }
}

obj_t assemble(char *buffer, char *filename, debug_flag_t flag) {
  assert(buffer);

  tokenizer_t tokenizer;
  tokenizer_init(&tokenizer, buffer, filename);

  if (flag & DEBUG_TOKENIZER) {
    token_t token = {0};
    while ((token = token_next(&tokenizer)).kind != T_NONE) {
      token_dump(token);
    }
    tokenizer_init(&tokenizer, buffer, filename);
  }

  obj_t obj = {0};

  token_t token = {0};
  while ((token = token_next(&tokenizer)).kind != T_NONE) {
    token_compile(token, &obj);
  }

  return obj;
}
