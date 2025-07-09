#ifndef ASSEMBLE_H__
#define ASSEMBLE_H__

#include "files.h"

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

obj_t assemble(char *buffer, char *buffer_name, int debug_info, int debug_flags, int allowed_flags);

#endif // ASSEMBLE_H__
