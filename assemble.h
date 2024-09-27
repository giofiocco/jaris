#ifndef ASSEMBLE_H__
#define ASSEMBLE_H__

#include "files.h"

#define MUL_PTR        0xF820
#define DIV_PTR        0xF822
#define SOLVE_PATH_PTR 0xF824
#define OPEN_FILE_PTR  0xF826
#define GET_FILE_PTR   0xF828
#define GET_FILE16_PTR 0xF82A
#define CLOSE_FILE_PTR 0xF82C
#define EXECUTE_PTR    0xF82E
#define EXIT_PTR       0xF830
#define PUT_CHAR_PTR   0xF832
#define PRINT_PTR      0xF834
#define GET_CHAR_PTR   0xF836
#define GET_DELIM_PTR  0xF838

typedef enum {
  DEBUG_TOKENIZER = 0b0001,
  DEBUG_BYTECODES = 0b0010,
  DEBUG_OBJ_STATE = 0b0100,
} assemble_debug_flag_t;

obj_t assemble(char *buffer, char *filename, assemble_debug_flag_t flag);

#endif  // ASSEMBLE_H__
