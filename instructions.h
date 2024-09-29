#ifndef INSTRUCTIONS_H__
#define INSTRUCTIONS_H__

#include <stdint.h>

#include "mystb/sv.h"

#define LABEL_MAX_LEN 256

// clang-format off
typedef enum {
  NOP = 0,
  INCA, DECA,
  RAM_AL, RAM_BL, RAM_A, RAM_B,
  INCSP, DECSP,
  PUSHA, POPA, PEEKA, PEEKAR, PUSHAR, 
  PUSHB, POPB, PEEKB, 
  SUM, SUB, SHR, SHL, CMPA, CMPB,
  JMP, JMPR, JMPRZ, JMPRN, JMPRC, JMPRNZ, JMPRNN, JMPRNC, JMPA,
  A_B, B_A, B_AH,
  AL_rB, A_rB, rB_AL, rB_A,
  A_SP, SP_A, 
  A_SEC, SEC_A, 
  RAM_NDX, INCNDX, NDX_A, A_NDX, MEM_A, MEM_AH,
  CALL, CALLR, CALLrRAM, RET,
  KEY,
  HLT = 0x3F
} instruction_t;
// clang-format on
#define INSTRUCTION_MAX_LEN 8

typedef enum {
  BNONE,
  BINST,
  BINSTHEX,
  BINSTHEX2,
  BINSTLABEL,
  BINSTRELLABEL,
  BHEX,
  BHEX2,
  BSTRING,
  BSETLABEL,
  BGLOBAL,
  BEXTERN,
  BALIGN,
  BDB,
} bytecode_kind_t;

typedef struct {
  bytecode_kind_t kind;
  instruction_t inst;
  union {
    uint16_t num;
    char string[LABEL_MAX_LEN];
  } arg;
} bytecode_t;

typedef struct {
  enum {
    INST_NO_ARGS,
    INST_8BITS_ARG,
    INST_16BITS_ARG,
    INST_LABEL_ARG,
    INST_RELLABEL_ARG,
  } arg;
} instruction_stat_t;

char *instruction_to_string(instruction_t instruction);
int sv_to_instruction(sv_t sv, instruction_t *out);
instruction_stat_t instruction_stat(instruction_t instruction);

void bytecode_dump(bytecode_t bc);
bytecode_t bytecode_with_string(bytecode_kind_t kind, instruction_t instruction, char *string);

#endif  // INSTRUCTIONS_H__
