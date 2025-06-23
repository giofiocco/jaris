#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>

#include "../files.h"
#include "../instructions.h"
#include "../mystb/errors.h"
#include "../mystb/sv.h"

#define ASM_TOKENS_MACRO_MAX 32
#define ASM_MACROS_MAX       32

typedef enum {
  ASMT_NONE,
  ASMT_SYM,
  ASMT_INST,
  ASMT_HEX,
  ASMT_HEX2,
  ASMT_MACROO,
  ASMT_MACROC,
  ASMT_COLON,
  ASMT_REL,
  ASMT_GLOBAL,
  ASMT_EXTERN,
  ASMT_STRING,
  ASMT_ALIGN,
  ASMT_DB,
  ASMT_INT,
  ASMT_BIN,
} asm_token_kind_t;

typedef struct {
  asm_token_kind_t kind;
  sv_t image;
  location_t loc;
  uint16_t asnum;
  instruction_t asinst;
} asm_token_t;

typedef struct {
  sv_t name;
  asm_token_t tokens[ASM_TOKENS_MACRO_MAX];
  int token_count;
} asm_macro_t;

typedef struct {
  char *buffer;
  location_t loc;
  asm_macro_t macros[ASM_MACROS_MAX];
  int macro_count;
  int current_macro;
  int current_macro_token;
} asm_tokenizer_t;

char *asm_token_kind_to_string(asm_token_kind_t kind) {
  switch (kind) {
    case ASMT_NONE: return "NONE";
    case ASMT_SYM: return "SYM";
    case ASMT_INST: return "INST";
    case ASMT_HEX: return "HEX";
    case ASMT_HEX2: return "HEX2";
    case ASMT_MACROO: return "MACROO";
    case ASMT_MACROC: return "MACROC";
    case ASMT_COLON: return "COLON";
    case ASMT_REL: return "REL";
    case ASMT_GLOBAL: return "GLOBAL";
    case ASMT_EXTERN: return "EXTERN";
    case ASMT_STRING: return "STRING";
    case ASMT_ALIGN: return "ALIGN";
    case ASMT_DB: return "DB";
    case ASMT_INT: return "INT";
    case ASMT_BIN: return "BIN";
  }
  assert(0);
}

void asm_token_dump(asm_token_t token) {
  printf(LOCATION_FMT " %s '" SV_FMT "'\n",
         LOCATION_UNPACK(token.loc),
         asm_token_kind_to_string(token.kind),
         SV_UNPACK(token.image));
}

void asm_tokenizer_init(asm_tokenizer_t *tok, char *buffer, char *buffer_name) {
  assert(tok);
  assert(buffer);
  *tok = (asm_tokenizer_t){0};
  tok->buffer = buffer;
  tok->loc = (location_t){buffer_name, buffer, 1, 1, 0};
  tok->current_macro = -1;
}

asm_token_t asm_new_token_and_consume(asm_tokenizer_t *tok, asm_token_kind_t kind, int count, uint16_t asint, instruction_t asinst) {
  assert(tok);
  location_t loc = tok->loc;
  loc.len = count;
  asm_token_t token = {kind, {tok->buffer, count}, loc, asint, asinst};
  tok->loc.col += count;
  tok->buffer += count;
  return token;
}

asm_token_t asm_token_next(asm_tokenizer_t *tok) {
  assert(tok);
  tok->loc.len = 1;

  if (tok->current_macro != -1) {
    assert(0 <= tok->current_macro && tok->current_macro < tok->macro_count);
    asm_macro_t *macro = &tok->macros[tok->current_macro];
    assert(0 <= tok->current_macro_token && tok->current_macro_token <= macro->token_count);
    if (tok->current_macro_token == macro->token_count) {
      tok->current_macro = -1;
    } else {
      return macro->tokens[tok->current_macro_token++];
    }
  }

  switch (*tok->buffer) {
    case '\0':
      return (asm_token_t){ASMT_NONE, {NULL, 0}, tok->loc, 0, 0};
    case ' ':
    case '\t':
      tok->buffer++;
      tok->loc.col++;
      return asm_token_next(tok);
    case '\r':
      tok->buffer++;
      return asm_token_next(tok);
    case '\n':
      tok->buffer++;
      tok->loc.row_start = tok->buffer;
      tok->loc.row++;
      tok->loc.col = 1;
      return asm_token_next(tok);
    case '}':
      return asm_new_token_and_consume(tok, ASMT_MACROC, 1, 0, 0);
    case ':':
      return asm_new_token_and_consume(tok, ASMT_COLON, 1, 0, 0);
    case '$':
      return asm_new_token_and_consume(tok, ASMT_REL, 1, 0, 0);
    case '{':
    {
      tok->buffer++;
      asm_token_t name = asm_token_next(tok);
      if (name.kind != ASMT_SYM) {
        eprintfloc(name.loc, "expected SYM as macro name found %s", asm_token_kind_to_string(name.kind));
      }
      asm_token_t t = {0};
      assert(tok->macro_count + 1 < ASM_MACROS_MAX);
      asm_macro_t *macro = &tok->macros[tok->macro_count++];
      macro->name = name.image;
      while ((t = asm_token_next(tok)).kind != ASMT_MACROC) {
        assert(macro->token_count + 1 < ASM_TOKENS_MACRO_MAX);
        macro->tokens[macro->token_count++] = t;
      }
      return asm_token_next(tok);
    }
    case '"':
    {
      int i = 1;
      while (tok->buffer[i] && tok->buffer[i] != '"') {
        i++;
      }
      i++;
      return asm_new_token_and_consume(tok, ASMT_STRING, i, 0, 0);
    }
    case '-':
      if (tok->buffer[1] == '-') {
        int i = 1;
        while (tok->buffer[i] && tok->buffer[i] != '\n') {
          i++;
        }
        tok->buffer += i;
        return asm_token_next(tok); // the row ++ etc is done by asm_token_next matching '\n'
      }
      __attribute__((fallthrough));
    case '0':
      if (tok->buffer[1] == 'x' || tok->buffer[1] == 'X') {
        int i = 2;
        while (isdigit(tok->buffer[i])
               || ('a' <= tok->buffer[i] && tok->buffer[i] <= 'f')
               || ('A' <= tok->buffer[i] && tok->buffer[i] <= 'F')) {
          i++;
        }
        if (i != 6 && i != 4) {
          tok->loc.len = i;
          eprintfloc(tok->loc, "invalid HEX");
        }
        uint16_t asint = strtol(tok->buffer + 2, NULL, 16);
        return asm_new_token_and_consume(tok, i == 6 ? ASMT_HEX2 : ASMT_HEX, i, asint, 0);
      } else if (tok->buffer[1] == 'b' || tok->buffer[1] == 'B') {
        int i = 2;
        while (tok->buffer[i] == '1'
               || tok->buffer[i] == '0') {
          i++;
        }
        if (i != 10) {
          tok->loc.len = i;
          eprintfloc(tok->loc, "invalid BIN");
        }
        uint16_t asint = strtol(tok->buffer + 2, NULL, 2);
        return asm_new_token_and_consume(tok, ASMT_BIN, i, asint, 0);
      }
      __attribute__((fallthrough));
    default:
      if (isalpha(*tok->buffer) || *tok->buffer == '_') {
        int i = 1;
        while (isalnum(tok->buffer[i]) || tok->buffer[i] == '_') {
          i++;
        }
        sv_t image = {tok->buffer, i};
        instruction_t asinst = 0;
        asm_token_kind_t kind = sv_to_instruction(image, &asinst)    ? ASMT_INST :
                                sv_eq(image, sv_from_cstr("GLOBAL")) ? ASMT_GLOBAL :
                                sv_eq(image, sv_from_cstr("EXTERN")) ? ASMT_EXTERN :
                                sv_eq(image, sv_from_cstr("ALIGN"))  ? ASMT_ALIGN :
                                sv_eq(image, sv_from_cstr("db"))     ? ASMT_DB :
                                                                       ASMT_SYM;
        if (kind == ASMT_SYM) {
          for (int j = 0; j < tok->macro_count; ++j) {
            if (sv_eq(tok->macros[j].name, image)) {
              tok->current_macro = j;
              tok->current_macro_token = 0;
              return asm_token_next(tok);
            }
          }
        }
        return asm_new_token_and_consume(tok, kind, i, 0, asinst);
      } else if (isdigit(*tok->buffer)) {
        int i = 1;
        while (isdigit(tok->buffer[i])) {
          i++;
        }
        int asint = atoi(tok->buffer);
        assert(asint < 0xFFFF);
        return asm_new_token_and_consume(tok, ASMT_INT, i, asint, 0);
      } else {
        eprintfloc(tok->loc, "invalid token");
      }
  }
  assert(0);
}

asm_token_t asm_token_expect(asm_tokenizer_t *tok, asm_token_kind_t kind) {
  assert(tok);
  asm_token_t token = asm_token_next(tok);
  if (token.kind != kind) {
    eprintfloc(token.loc, "expected %s, found %s", asm_token_kind_to_string(kind), asm_token_kind_to_string(token.kind));
  }
  return token;
}

bytecode_t asm_parse_bytecode(asm_tokenizer_t *tok) {
  assert(tok);

  asm_token_t token = asm_token_next(tok);

  bytecode_t bc = {BNONE, 0, {}};

  switch (token.kind) {
    case ASMT_NONE:
      break;
    case ASMT_SYM:
      asm_token_expect(tok, ASMT_COLON);
      bc = bytecode_with_sv(BSETLABEL, 0, token.image);
      break;
    case ASMT_INST:
      switch (instruction_stat(token.asinst).arg) {
        case INST_NO_ARGS:
          bc = (bytecode_t){BINST, token.asinst, {}};
          break;
        case INST_8BITS_ARG:
        {
          asm_token_t arg = asm_token_next(tok);
          if (arg.kind == ASMT_HEX || arg.kind == ASMT_BIN) {
            bc = (bytecode_t){BINSTHEX, token.asinst, {.num = arg.asnum}};
          } else if (arg.kind == ASMT_STRING && arg.image.len == 3) {
            bc = (bytecode_t){BINSTHEX, token.asinst, {.num = arg.image.start[1]}};
          } else {
            eprintfloc(arg.kind == ASMT_NONE ? token.loc : arg.loc,
                       "expected HEX, BIN or STRING with len 1, found %s",
                       asm_token_kind_to_string(arg.kind));
          }
          if ((token.asinst == PEEKAR || token.asinst == PUSHAR) && bc.arg.num % 2 != 0) {
            eprintfloc(arg.loc, "expected even number");
          }
        } break;
        case INST_16BITS_ARG:
        {
          asm_token_t arg = asm_token_next(tok);
          if (arg.kind == ASMT_HEX2) {
            bc = (bytecode_t){BINSTHEX2, token.asinst, {.num = arg.asnum}};
          } else if (arg.kind == ASMT_SYM) {
            bc = bytecode_with_sv(BINSTLABEL, token.asinst, arg.image);
          } else if (arg.kind == ASMT_STRING && arg.image.len == 2 + 2) {
            bc = (bytecode_t){BINSTHEX2, token.asinst, {.num = arg.image.start[1] | (arg.image.start[2] << 8)}};
          } else {
            eprintfloc(arg.kind == ASMT_NONE ? token.loc : arg.loc, "expected HEX2, found %s", asm_token_kind_to_string(arg.kind));
          }
        } break;
        case INST_LABEL_ARG:
        {
          asm_token_t arg = asm_token_expect(tok, ASMT_SYM);
          bc = bytecode_with_sv(BINSTLABEL, token.asinst, arg.image);
        } break;
        case INST_RELLABEL_ARG:
        {
          asm_token_expect(tok, ASMT_REL);
          asm_token_t arg = asm_token_expect(tok, ASMT_SYM);
          bc = bytecode_with_sv(BINSTRELLABEL, token.asinst, arg.image);
        } break;
      }
      break;
    case ASMT_GLOBAL:
    case ASMT_EXTERN:
    {
      asm_token_t arg = asm_token_expect(tok, ASMT_SYM);
      bc = bytecode_with_sv(token.kind == ASMT_GLOBAL ? BGLOBAL : BEXTERN, 0, arg.image);
    } break;
    case ASMT_ALIGN:
      bc = (bytecode_t){BALIGN, 0, {}};
      break;
    case ASMT_DB:
    {
      asm_token_t arg = asm_token_expect(tok, ASMT_INT);
      bc = (bytecode_t){BDB, 0, {.num = arg.asnum}};
    } break;
    case ASMT_HEX:
    case ASMT_HEX2:
      bc = (bytecode_t){token.kind == ASMT_HEX ? BHEX : BHEX2, 0, {.num = token.asnum}};
      break;
    case ASMT_STRING:
      bc = bytecode_with_sv(BSTRING, 0, (sv_t){token.image.start + 1, token.image.len - 2});
      break;
    case ASMT_BIN:
      bc = (bytecode_t){BHEX, 0, {.num = token.asnum}};
      break;
    case ASMT_MACROO:
    case ASMT_MACROC:
    case ASMT_COLON:
    case ASMT_REL:
    case ASMT_INT:
      eprintfloc(token.loc, "invalid token: %s", asm_token_kind_to_string(token.kind));
  }

  return bc;
}

obj_t assemble(char *buffer, char *buffer_name, int debug_info, int debug_tok, int debug_byt, int debug_obj) {
  assert(buffer);

  asm_tokenizer_t tok = {0};
  asm_tokenizer_init(&tok, buffer, buffer_name);

  if (debug_tok) {
    asm_token_t t = {0};
    while ((t = asm_token_next(&tok)).kind != ASMT_NONE) {
      asm_token_dump(t);
    }
    asm_tokenizer_init(&tok, buffer, buffer_name);
  }

  obj_t obj = {0};
  bytecode_t bc = {0};
  while ((bc = asm_parse_bytecode(&tok)).kind != BNONE) {
    if (debug_byt) {
      bytecode_dump(bc);
    }

    obj_compile_bytecode(&obj, bc);
  }

  if (debug_obj) {
    obj_dump(&obj);
  }

  obj_check(&obj, debug_info);

  return obj;
}
