#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#define SV_IMPLEMENTATION
#include "assemble.h"
#include "instructions.h"
#include "mystb/errors.h"

#define TODO assert(0 && "TO IMPLEMENT")

typedef enum {
  T_NONE,
  T_SYM,
  T_INST,
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
  union {
    uint16_t num;
    instruction_t inst;
  } as;
} token_t;

typedef struct {
  char *buffer;
  location_t loc;
} tokenizer_t;

#define MACRO_MAX        128
#define TOKENS_MACRO_MAX 128

typedef struct {
  token_t name;
  token_t tokens[TOKENS_MACRO_MAX];
  int tokeni;
} macro_t;

typedef struct {
  tokenizer_t *tok;
  macro_t macros[MACRO_MAX];
  int macroi;
  macro_t *active_macro;
  int active_macro_tokeni;
} preprocessor_t;

// clang-format off
char *token_kind_to_string(token_kind_t kind) {
  switch (kind) {
    case T_NONE: return "NONE";
    case T_SYM: return "SYM";
    case T_INST: return "INST";
    case T_HEX: return "HEX";
    case T_HEX2: return "HEX2";
    case T_MACROO: return "MACROO";
    case T_MACROC: return "MACROC";
    case T_COLON: return "COLON";
    case T_REL: return "REL";
    case T_GLOBAL: return "GLOBAL";
    case T_EXTERN: return "EXTERN";
    case T_STRING: return "STRING";
    case T_ALIGN: return "ALIGN";
    case T_DB: return "DB";
    case T_INT: return "INT";
  }
  assert(0);
}
// clang-format on

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
      {filename, buffer, 1, 1, 0},
  };
}

token_t token_next(tokenizer_t *tokenizer) {
  assert(tokenizer);

  tokenizer->loc.len = 1;
  token_t token = {T_NONE, {}, tokenizer->loc, {}};

  token_kind_t table[128] = {0};
  table['{'] = T_MACROO;
  table['}'] = T_MACROC;
  table[':'] = T_COLON;
  table['$'] = T_REL;

  switch (*tokenizer->buffer) {
    case '\0':
      break;
    case ' ':
    case '\t':
      ++tokenizer->buffer;
      ++tokenizer->loc.col;
      return token_next(tokenizer);
    case '\r':
      ++tokenizer->buffer;
      return token_next(tokenizer);
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
      token = (token_t){table[(int)*tokenizer->buffer], {tokenizer->buffer, 1}, tokenizer->loc, {}};
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
      token = (token_t){T_STRING, {start, len}, tokenizer->loc, {}};
      tokenizer->loc.col += len;
    } break;
    default:
      if (isalpha(*tokenizer->buffer) || *tokenizer->buffer == '_') {
        char *start = tokenizer->buffer;
        int len = 0;
        while (isalnum(*tokenizer->buffer) || *tokenizer->buffer == '_') {
          ++len;
          ++tokenizer->buffer;
        }
        tokenizer->loc.len = len;
        sv_t image = {start, len};
        instruction_t inst;
        token = (token_t){sv_to_instruction(image, &inst)      ? T_INST :
                          sv_eq(image, sv_from_cstr("GLOBAL")) ? T_GLOBAL :
                          sv_eq(image, sv_from_cstr("EXTERN")) ? T_EXTERN :
                          sv_eq(image, sv_from_cstr("ALIGN"))  ? T_ALIGN :
                          sv_eq(image, sv_from_cstr("db"))     ? T_DB :
                                                                 T_SYM,
                          image,
                          tokenizer->loc,
                          {.inst = inst}};
        tokenizer->loc.col += len;
      } else if (*tokenizer->buffer == '0'
                 && (*(tokenizer->buffer + 1) == 'x' || *(tokenizer->buffer + 1) == 'X')) {
        char *start = tokenizer->buffer;
        int len = 0;
        tokenizer->buffer += 2;
        while (isdigit(*tokenizer->buffer)
               || ('a' <= *tokenizer->buffer && *tokenizer->buffer <= 'f')
               || ('A' <= *tokenizer->buffer && *tokenizer->buffer <= 'F')) {
          ++len;
          ++tokenizer->buffer;
        }
        tokenizer->loc.len = len + 2;
        if (len != 2 && len != 4) {
          eprintfloc(tokenizer->loc, "invalid HEX");
        }
        token = (token_t){len == 2 ? T_HEX : T_HEX2,
                          {start, len + 2},
                          tokenizer->loc,
                          {.num = strtol(start, NULL, 16)}};
        tokenizer->loc.col += len;
      } else if (isdigit(*tokenizer->buffer)) {
        char *start = tokenizer->buffer;
        int len = 0;
        while (isdigit(*tokenizer->buffer)) {
          ++len;
          ++tokenizer->buffer;
        }
        tokenizer->loc.len = len;
        token = (token_t){T_INT, {start, len}, tokenizer->loc, {.num = atoi(start)}};
        tokenizer->loc.col += len;
      } else if (*tokenizer->buffer == '-' && *(tokenizer->buffer + 1) == '-') {
        while (*tokenizer->buffer != '\n') {
          ++tokenizer->buffer;
          ++tokenizer->loc.col;
        }
        return token_next(tokenizer);
      } else {
        tokenizer->loc.len = 1;
        if (isprint(*tokenizer->buffer)) {
          eprintfloc(tokenizer->loc, "invalid char");
        } else {
          eprintfloc(tokenizer->loc, "invalid char: '%X'", *tokenizer->buffer);
        }
      }
  }

  return token;
}

macro_t *preprocessor_find_macro(preprocessor_t *pre, sv_t image) {
  for (int i = 0; i < pre->macroi; ++i) {
    if (sv_eq(image, pre->macros[i].name.image)) {
      return &pre->macros[i];
    }
  }
  return NULL;
}

token_t preprocessor_token_next(preprocessor_t *pre) {
  assert(pre);

  if (pre->active_macro != NULL) {
    if (pre->active_macro_tokeni < pre->active_macro->tokeni) {
      return pre->active_macro->tokens[pre->active_macro_tokeni++];
    } else {
      pre->active_macro = NULL;
    }
  }

  token_t token = token_next(pre->tok);

  if (token.kind == T_MACROO) {
    token_t name = token_next(pre->tok);
    if (name.kind != T_SYM) {
      eprintfloc(name.loc, "expected SYM, found %s", token_kind_to_string(name.kind));
    }
    macro_t *macro = preprocessor_find_macro(pre, name.image);
    if (macro != NULL) {
      eprintfloc(name.loc,
                 "macro redefinition, already defined at " LOCATION_FMT,
                 LOCATION_UNPACK(macro->name.loc));
    }
    assert(pre->macroi + 1 < MACRO_MAX);
    macro = &pre->macros[pre->macroi++];
    macro->name = name;

    while ((token = token_next(pre->tok)).kind != T_MACROC) {
      macro->tokens[macro->tokeni++] = token;
    }

    token = preprocessor_token_next(pre);
  }
  if (token.kind == T_SYM) {
    macro_t *macro = preprocessor_find_macro(pre, token.image);
    if (macro != NULL) {
      pre->active_macro = macro;
      pre->active_macro_tokeni = 0;
      return preprocessor_token_next(pre);
    }
  }

  return token;
}

token_t preprocessor_token_expect(preprocessor_t *pre, token_kind_t kind) {
  assert(pre);

  token_t token = preprocessor_token_next(pre);
  if (token.kind != kind) {
    eprintfloc(token.loc,
               "expected %s, found %s",
               token_kind_to_string(kind),
               token_kind_to_string(token.kind));
  }

  return token;
}

bytecode_t bytecode_with_sv(bytecode_kind_t kind, instruction_t inst, sv_t sv) {
  bytecode_t b = {kind, inst, {}};
  assert(sv.len < LABEL_MAX_LEN);
  memcpy(b.arg.string, sv.start, sv.len);
  return b;
}

bytecode_t compile(preprocessor_t *pre) {
  assert(pre);

  token_t token = preprocessor_token_next(pre);

  bytecode_t bytecode = {0};

  switch (token.kind) {
    case T_SYM:
      preprocessor_token_expect(pre, T_COLON);
      bytecode = bytecode_with_sv(BSETLABEL, 0, token.image);
      break;
    case T_INST:
      switch (instruction_stat(token.as.inst).arg) {
        case INST_NO_ARGS:
          bytecode = (bytecode_t){BINST, token.as.inst, {}};
          break;
        case INST_8BITS_ARG:
        {
          token_t arg = preprocessor_token_next(pre);
          if (arg.kind == T_HEX) {
            bytecode = (bytecode_t){BINSTHEX, token.as.inst, {.num = arg.as.num}};
          } else if (arg.kind == T_STRING && arg.image.len == 3) {
            bytecode = (bytecode_t){BINSTHEX, token.as.inst, {.num = arg.image.start[1]}};
          } else {
            eprintfloc(arg.loc,
                       "expected HEX or STRING with len 1, found %s",
                       token_kind_to_string(arg.kind));
          }
        } break;
        case INST_16BITS_ARG:
        {
          token_t arg = preprocessor_token_next(pre);
          if (arg.kind == T_HEX2) {
            bytecode = (bytecode_t){BINSTHEX2, token.as.inst, {.num = arg.as.num}};
          } else if (arg.kind == T_SYM) {
            bytecode = bytecode_with_sv(BINSTLABEL, token.as.inst, arg.image);
          } else if (arg.kind == T_STRING && arg.image.len == 2 + 2) {
            bytecode = (bytecode_t){
                BINSTHEX2, token.as.inst, {.num = arg.image.start[1] | (arg.image.start[2] << 8)}};
          } else {
            eprintfloc(arg.loc, "expected HEX2, found %s", token_kind_to_string(arg.kind));
          }
        } break;
        case INST_LABEL_ARG:
        {
          token_t arg = preprocessor_token_expect(pre, T_SYM);
          bytecode = bytecode_with_sv(BINSTLABEL, token.as.inst, arg.image);
        } break;
        case INST_RELLABEL_ARG:
        {
          preprocessor_token_expect(pre, T_REL);
          token_t arg = preprocessor_token_expect(pre, T_SYM);
          bytecode = bytecode_with_sv(BINSTRELLABEL, token.as.inst, arg.image);
        } break;
      }
      break;
    case T_GLOBAL:
    case T_EXTERN:
    {
      token_t arg = preprocessor_token_expect(pre, T_SYM);
      bytecode = bytecode_with_sv(token.kind == T_GLOBAL ? BGLOBAL : BEXTERN, 0, arg.image);
    } break;
    case T_ALIGN:
      bytecode = (bytecode_t){BALIGN, 0, {}};
      break;
    case T_DB:
    {
      token_t arg = preprocessor_token_expect(pre, T_INT);
      bytecode = (bytecode_t){BDB, 0, {.num = arg.as.num}};
    } break;
    case T_HEX:
    case T_HEX2:
      bytecode = (bytecode_t){token.kind == T_HEX ? BHEX : BHEX2, 0, {.num = token.as.num}};
      break;
    case T_STRING:
      bytecode = bytecode_with_sv(BSTRING, 0, (sv_t){token.image.start + 1, token.image.len - 2});
      break;
    case T_NONE:
      return (bytecode_t){BNONE, 0, {}};
    case T_MACROO:
    case T_MACROC:
    case T_COLON:
    case T_REL:
    case T_INT:
      eprintfloc(token.loc, "invalid token: %s", token_kind_to_string(token.kind));
  }

  return bytecode;
}

obj_t assemble(char *buffer, char *filename, assemble_debug_flag_t flag, int debug_info) {
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

  preprocessor_t pre = {0};
  pre.tok = &tokenizer;

  obj_state_t objs = {0};

  bytecode_t bc = {0};
  while ((bc = compile(&pre)).kind != BNONE) {
    if (flag & DEBUG_BYTECODES) {
      bytecode_dump(bc);
    }
    obj_compile_bytecode(&objs, bc);
  }

  if (debug_info || (flag & DEBUG_OBJ_STATE)) {
    printf("SYMBOLS: %d\n", objs.symbol_num);
    for (int i = 0; i < objs.symbol_num; ++i) {
      symbol_t *s = &objs.symbols[i];
      printf("\t%s: %04X\n", s->image, s->pos);
      if (s->relreloc_num != 0) {
        printf("\t\tRELRELOCS:");
        for (int j = 0; j < s->relreloc_num; ++j) {
          printf(" %04X", s->relrelocs[j]);
        }
        printf("\n");
      }
      if (s->reloc_num) {
        printf("\t\tRELOCS:");
        for (int j = 0; j < s->reloc_num; ++j) {
          printf(" %04X", s->relocs[j]);
        }
        printf("\n");
      }
    }
    obj_dump(&objs.obj);
  }

  obj_state_check_obj(&objs);

  return objs.obj;
}
