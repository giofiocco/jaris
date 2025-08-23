#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "instructions.h"

// clang-format off
char *instruction_to_string(instruction_t instruction) {
  switch (instruction) {
    case NOP: return "NOP";
    case INCA: return "INCA";
    case DECA: return "DECA";
    case INCB: return "INCB";
    case RAM_AL: return "RAM_AL";
    case RAM_BL: return "RAM_BL";
    case RAM_A: return "RAM_A";
    case RAM_B: return "RAM_B";
    case INCSP: return "INCSP";
    case DECSP: return "DECSP";
    case PUSHA: return "PUSHA";
    case POPA: return "POPA";
    case PEEKA: return "PEEKA";
    case PEEKAR: return "PEEKAR";
    case PUSHAR: return "PUSHAR";
    case PUSHB: return "PUSHB";
    case POPB: return "POPB";
    case PEEKB: return "PEEKB";
    case SUM: return "SUM";
    case SUB: return "SUB";
    case SHR: return "SHR";
    case SHL: return "SHL";
    case AND: return "AND";
    case CMPA: return "CMPA";
    case CMPB: return "CMPB";
    case JMP: return "JMP";
    case JMPR: return "JMPR";
    case JMPRZ: return "JMPRZ";
    case JMPRN: return "JMPRN";
    case JMPRC: return "JMPRC";
    case JMPRNZ: return "JMPRNZ";
    case JMPRNN: return "JMPRNN";
    case JMPRNC: return "JMPRNC";
    case JMPA: return "JMPA";
    case JMPAR: return "JMPAR";
    case A_B: return "A_B";
    case B_A: return "B_A";
    case B_AH: return "B_AH";
    case AL_rB: return "AL_rB";
    case A_rB: return "A_rB";
    case rB_AL: return "rB_AL";
    case rB_A: return "rB_A";
    case A_SP: return "A_SP";
    case SP_A: return "SP_A";
    case A_SEC: return "A_SEC";
    case SEC_A: return "SEC_A";
    case RAM_NDX: return "RAM_NDX";
    case INCNDX: return "INCNDX";
    case NDX_A: return "NDX_A";
    case A_NDX: return "A_NDX";
    case MEM_A: return "MEM_A";
    case MEM_AH: return "MEM_AH";
    case A_MEM: return "A_MEM";
    case CALL: return "CALL";
    case CALLR: return "CALLR";
    case RET: return "RET";
    case _KEY_A: return "KEY_A";
    case DRW: return "DRW";
    case RAM_DRW: return "RAM_DRW";
    case HLT: return "HLT";
  }
  return NULL;
}
// clang-format on

int sv_to_instruction(sv_t sv, instruction_t *out) {
  if (sv_eq(sv, sv_from_cstr("NOP"))) {
    *out = NOP;
  } else if (sv_eq(sv, sv_from_cstr("INCA"))) {
    *out = INCA;
  } else if (sv_eq(sv, sv_from_cstr("DECA"))) {
    *out = DECA;
  } else if (sv_eq(sv, sv_from_cstr("INCB"))) {
    *out = INCB;
  } else if (sv_eq(sv, sv_from_cstr("RAM_AL"))) {
    *out = RAM_AL;
  } else if (sv_eq(sv, sv_from_cstr("RAM_BL"))) {
    *out = RAM_BL;
  } else if (sv_eq(sv, sv_from_cstr("RAM_A"))) {
    *out = RAM_A;
  } else if (sv_eq(sv, sv_from_cstr("RAM_B"))) {
    *out = RAM_B;
  } else if (sv_eq(sv, sv_from_cstr("INCSP"))) {
    *out = INCSP;
  } else if (sv_eq(sv, sv_from_cstr("DECSP"))) {
    *out = DECSP;
  } else if (sv_eq(sv, sv_from_cstr("PUSHA"))) {
    *out = PUSHA;
  } else if (sv_eq(sv, sv_from_cstr("POPA"))) {
    *out = POPA;
  } else if (sv_eq(sv, sv_from_cstr("PEEKA"))) {
    *out = PEEKA;
  } else if (sv_eq(sv, sv_from_cstr("PEEKAR"))) {
    *out = PEEKAR;
  } else if (sv_eq(sv, sv_from_cstr("PUSHAR"))) {
    *out = PUSHAR;
  } else if (sv_eq(sv, sv_from_cstr("PUSHB"))) {
    *out = PUSHB;
  } else if (sv_eq(sv, sv_from_cstr("POPB"))) {
    *out = POPB;
  } else if (sv_eq(sv, sv_from_cstr("PEEKB"))) {
    *out = PEEKB;
  } else if (sv_eq(sv, sv_from_cstr("SUM"))) {
    *out = SUM;
  } else if (sv_eq(sv, sv_from_cstr("SUB"))) {
    *out = SUB;
  } else if (sv_eq(sv, sv_from_cstr("SHR"))) {
    *out = SHR;
  } else if (sv_eq(sv, sv_from_cstr("SHL"))) {
    *out = SHL;
  } else if (sv_eq(sv, sv_from_cstr("AND"))) {
    *out = AND;
  } else if (sv_eq(sv, sv_from_cstr("CMPA"))) {
    *out = CMPA;
  } else if (sv_eq(sv, sv_from_cstr("CMPB"))) {
    *out = CMPB;
  } else if (sv_eq(sv, sv_from_cstr("JMP"))) {
    *out = JMP;
  } else if (sv_eq(sv, sv_from_cstr("JMPR"))) {
    *out = JMPR;
  } else if (sv_eq(sv, sv_from_cstr("JMPRZ"))) {
    *out = JMPRZ;
  } else if (sv_eq(sv, sv_from_cstr("JMPRN"))) {
    *out = JMPRN;
  } else if (sv_eq(sv, sv_from_cstr("JMPRC"))) {
    *out = JMPRC;
  } else if (sv_eq(sv, sv_from_cstr("JMPRNZ"))) {
    *out = JMPRNZ;
  } else if (sv_eq(sv, sv_from_cstr("JMPRNN"))) {
    *out = JMPRNN;
  } else if (sv_eq(sv, sv_from_cstr("JMPRNC"))) {
    *out = JMPRNC;
  } else if (sv_eq(sv, sv_from_cstr("JMPA"))) {
    *out = JMPA;
  } else if (sv_eq(sv, sv_from_cstr("JMPAR"))) {
    *out = JMPAR;
  } else if (sv_eq(sv, sv_from_cstr("A_B"))) {
    *out = A_B;
  } else if (sv_eq(sv, sv_from_cstr("B_A"))) {
    *out = B_A;
  } else if (sv_eq(sv, sv_from_cstr("B_AH"))) {
    *out = B_AH;
  } else if (sv_eq(sv, sv_from_cstr("AL_rB"))) {
    *out = AL_rB;
  } else if (sv_eq(sv, sv_from_cstr("A_rB"))) {
    *out = A_rB;
  } else if (sv_eq(sv, sv_from_cstr("rB_AL"))) {
    *out = rB_AL;
  } else if (sv_eq(sv, sv_from_cstr("rB_A"))) {
    *out = rB_A;
  } else if (sv_eq(sv, sv_from_cstr("A_SP"))) {
    *out = A_SP;
  } else if (sv_eq(sv, sv_from_cstr("SP_A"))) {
    *out = SP_A;
  } else if (sv_eq(sv, sv_from_cstr("A_SEC"))) {
    *out = A_SEC;
  } else if (sv_eq(sv, sv_from_cstr("SEC_A"))) {
    *out = SEC_A;
  } else if (sv_eq(sv, sv_from_cstr("RAM_NDX"))) {
    *out = RAM_NDX;
  } else if (sv_eq(sv, sv_from_cstr("INCNDX"))) {
    *out = INCNDX;
  } else if (sv_eq(sv, sv_from_cstr("NDX_A"))) {
    *out = NDX_A;
  } else if (sv_eq(sv, sv_from_cstr("A_NDX"))) {
    *out = A_NDX;
  } else if (sv_eq(sv, sv_from_cstr("MEM_A"))) {
    *out = MEM_A;
  } else if (sv_eq(sv, sv_from_cstr("MEM_AH"))) {
    *out = MEM_AH;
  } else if (sv_eq(sv, sv_from_cstr("A_MEM"))) {
    *out = A_MEM;
  } else if (sv_eq(sv, sv_from_cstr("CALL"))) {
    *out = CALL;
  } else if (sv_eq(sv, sv_from_cstr("CALLR"))) {
    *out = CALLR;
  } else if (sv_eq(sv, sv_from_cstr("RET"))) {
    *out = RET;
  } else if (sv_eq(sv, sv_from_cstr("KEY_A"))) {
    *out = _KEY_A;
  } else if (sv_eq(sv, sv_from_cstr("DRW"))) {
    *out = DRW;
  } else if (sv_eq(sv, sv_from_cstr("RAM_DRW"))) {
    *out = RAM_DRW;
  } else if (sv_eq(sv, sv_from_cstr("HLT"))) {
    *out = HLT;
  } else {
    return 0;
  }
  return 1;
}

// clang-format off
instruction_stat_t instruction_stat(instruction_t instruction) {
  switch (instruction) {
    case NOP: case INCA: case DECA: case INCB: case INCSP: case DECSP:
    case PUSHA: case POPA: case PEEKA: case PUSHB: case POPB: case PEEKB:
    case SUM: case SUB: case SHR: case SHL: case AND: case CMPA: case CMPB:
    case JMPA: case JMPAR:
    case A_B: case B_A: case B_AH:
    case AL_rB: case A_rB: case rB_AL: case rB_A:
    case A_SP: case SP_A:
    case A_SEC: case SEC_A:
    case INCNDX: case NDX_A: case A_NDX: case MEM_A: case MEM_AH: case A_MEM:
    case RET:
    case _KEY_A:
    case DRW:
    case HLT:
      return (instruction_stat_t){INST_NO_ARGS};
    case RAM_AL: case RAM_BL:
    case PEEKAR: case PUSHAR:
    case RAM_NDX:
    case RAM_DRW:
      return (instruction_stat_t){INST_8BITS_ARG};
    case RAM_A: case RAM_B:
      return (instruction_stat_t){INST_16BITS_ARG};
    case JMP: case CALL:
      return (instruction_stat_t){INST_LABEL_ARG};
    case JMPR: case JMPRZ: case JMPRN: case JMPRC: case JMPRNZ: case JMPRNN: case JMPRNC:
    case CALLR:
      return (instruction_stat_t){INST_RELLABEL_ARG};
  }
  assert(0);
}
// clang-format on

void bytecode_dump(bytecode_t bc) {
  switch (bc.kind) {
    case BNONE:
      printf("NONE\n");
      break;
    case BINST:
      printf("INST         %s\n", instruction_to_string(bc.inst));
      break;
    case BINSTHEX:
      printf("INSTHEX      %s 0x%02X\n", instruction_to_string(bc.inst), bc.arg.num);
      break;
    case BINSTHEX2:
      printf("INSTHEX2     %s 0x%04X\n", instruction_to_string(bc.inst), bc.arg.num);
      break;
    case BINSTLABEL:
      printf("INSTLABEL    %s %s\n", instruction_to_string(bc.inst), bc.arg.string);
      break;
    case BINSTRELLABEL:
      printf("INSTRELLABEL %s %s\n", instruction_to_string(bc.inst), bc.arg.string);
      break;
    case BHEX:
      printf("HEX          0x%02X\n", bc.arg.num);
      break;
    case BHEX2:
      printf("HEX2         0x%04X\n", bc.arg.num);
      break;
    case BSTRING:
      printf("STRING       \"%s\"\n", bc.arg.string);
      break;
    case BSETLABEL:
      printf("SETLABEL     %s\n", bc.arg.string);
      break;
    case BGLOBAL:
      printf("GLOBAL       %s\n", bc.arg.string);
      break;
    case BEXTERN:
      printf("EXTERN       %s\n", bc.arg.string);
      break;
    case BALIGN:
      printf("ALIGN\n");
      break;
    case BDB:
      printf("DB           %d\n", bc.arg.num);
      break;
  }
}

bytecode_t bytecode_with_string(bytecode_kind_t kind, instruction_t inst, char *str) {
  bytecode_t bc = {kind, inst, {}};
  strcpy(bc.arg.string, str);
  return bc;
}

bytecode_t bytecode_with_sv(bytecode_kind_t kind, instruction_t inst, sv_t sv) {
  bytecode_t bc = {kind, inst, {}};
  memcpy(bc.arg.string, sv.start, sv.len * sizeof(char));
  return bc;
}

void bytecode_to_asm(FILE *stream, bytecode_t bc) {
  switch (bc.kind) {
    case BNONE: assert(0); break;
    case BINST: fprintf(stream, "%s", instruction_to_string(bc.inst)); break;
    case BINSTHEX: fprintf(stream, "%s 0x%02X", instruction_to_string(bc.inst), bc.arg.num); break;
    case BINSTHEX2: fprintf(stream, "%s 0x%04X", instruction_to_string(bc.inst), bc.arg.num); break;
    case BINSTLABEL: fprintf(stream, "%s %s", instruction_to_string(bc.inst), bc.arg.string); break;
    case BINSTRELLABEL: fprintf(stream, "%s $%s", instruction_to_string(bc.inst), bc.arg.string); break;
    case BHEX: fprintf(stream, "0x%02X", bc.arg.num); break;
    case BHEX2: fprintf(stream, "0x%04X", bc.arg.num); break;
    case BSTRING: fprintf(stream, "\"%s\"", bc.arg.string); break;
    case BSETLABEL: fprintf(stream, "%s:", bc.arg.string); break;
    case BGLOBAL: fprintf(stream, "GLOBAL %s", bc.arg.string); break;
    case BEXTERN: fprintf(stream, "EXTERN %s", bc.arg.string); break;
    case BALIGN: fprintf(stream, "ALIGN"); break;
    case BDB: fprintf(stream, "db %d", bc.arg.num); break;
  }
}
