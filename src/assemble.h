#ifndef ASSEMBLE_H__
#define ASSEMBLE_H__

#include "../mystb/errors.h"
#include "files.h"

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
  int allowed_flags;
} asm_tokenizer_t;

typedef enum {
  ASM_ALLOWED_INST_AS_ARG,

  ASM_ALLOWED_MAX
} asm_allow_flag_t;

typedef enum {
  ASM_DEBUG_TOK,
  ASM_DEBUG_BYT,
  ASM_DEBUG_OBJ,

  ASM_DEBUG_MAX
} debug_flag_t;

void asm_tokenizer_init(asm_tokenizer_t *tok, char *buffer, char *buffer_name, int allowed_flags);
bytecode_t asm_parse_bytecode(asm_tokenizer_t *tok);

obj_t assemble(char *buffer, char *buffer_name, int debug_info, int debug_flags, int allowed_flags);

#endif // ASSEMBLE_H__
