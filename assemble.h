#ifndef ASSEMBLE_H__
#define ASSEMBLE_H__

#include "files.h"

typedef enum {
  DEBUG_TOKENIZER = 1 << 0,
  DEBUG_BYTECODES = 1 << 1,
  DEBUG_OBJ_STATE = 1 << 2,
} assemble_debug_flag_t;

obj_t assemble(char *buffer, char *filename, assemble_debug_flag_t flag);

#endif // ASSEMBLE_H__
