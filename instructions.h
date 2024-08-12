#ifndef INSTRUCTIONS_H__
#define INSTRUCTIONS_H__

#include <stdint.h>

#include "sv.h"

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

typedef struct {
  enum {
    BINST,
    BINSTHEX,
    BINSTHEX2,
    BINSTLABEL,
    BINSTRELLABEL,
    BSETLABEL,
    BGLOBAL,
    BEXTERN,
  } kind;
  instruction_t inst;
  union {
    uint16_t num;
    sv_t sv;
  } arg;
} bytecode_t;

char *instruction_to_string(instruction_t instruction);

#endif // INSTRUCTIONS_H__
