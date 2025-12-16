#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <raylib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ERRORS_IMPLEMENTATION
#define SV_IMPLEMENTATION
#include "files.h"
#include "instructions.h"
#include "runtime.h"

Color screen_palette[] = {
    {0x34, 0x68, 0x56, 0xFF},
    {0x08, 0x18, 0x20, 0xFF},

    // {0x61, 0xA6, 0xB4, 0xFF},
    // {0x01, 0x46, 0x54, 0xFF},

    // {0xE2, 0x7E, 0x80, 0xFF},
    // {0x82, 0x1E, 0x20, 0xFF},
};

microcode_t control_rom[1 << (6 + 4 + 3)] = {0};

void cpu_init(cpu_t *cpu, char *mem_path) {
  assert(cpu);

  *cpu = (cpu_t){0};

  FILE *file = fopen(mem_path, "rb");
  if (!file) {
    eprintf("cannot open file: '%s': %s\n", mem_path, strerror(errno));
  }
  assert(fread(cpu->MEM, 1, 1 << 19, file) == 1 << 19);
  assert(fclose(file) == 0);

  cpu->IP = 0;
  cpu->SEC = cpu->IP;
  for (int i = 0; i < 256; ++i) {
    cpu->IP = (cpu->IP - 1) & 0xFFFF;
    uint16_t bus = cpu->IP;
    cpu->MAR = bus;
    cpu->NDX = bus & 0xFF;
    cpu->RAM[cpu->MAR] = cpu->MEM[cpu->NDX | (cpu->SEC << 8)];
  }
  assert(cpu->IP == 0xFF00);
}

uint16_t cpu_read16(cpu_t *cpu, uint16_t at) {
  assert(cpu);
  assert(at % 2 == 0);
  return cpu->RAM[at] | (cpu->RAM[at + 1] << 8);
}

void cpu_write16(cpu_t *cpu, uint16_t at, uint16_t what) {
  assert(cpu);
  assert(at % 2 == 0);
  cpu->RAM[at] = what & 0xFF;
  cpu->RAM[at + 1] = (what >> 8) & 0xFF;
}

void dumpsbit(uint32_t num, int bits) {
  putchar('0');
  putchar('b');
  for (int i = 0; i < bits; ++i) {
    putchar((num >> (bits - 1 - i)) & 1 ? '1' : '0');
  }
}

void cpu_dump(cpu_t *cpu) {
  assert(cpu);

  printf("A:   %5d %04X ", cpu->A, cpu->A);
  dumpsbit(cpu->A, 16);
  printf(" %c\n", isprint(cpu->A) ? cpu->A : ' ');
  printf("B:   %5d %04X ", cpu->B, cpu->B);
  dumpsbit(cpu->B, 16);
  printf(" %c\n", isprint(cpu->B) ? cpu->B : ' ');
  printf("IP:  %5d %04X %s\n", cpu->IP, cpu->IP, instruction_to_string(cpu->IR));
  printf("FR:         %c%c%c\n",
         cpu->FR & (1 << C) ? 'C' : ' ',
         cpu->FR & (1 << N) ? 'N' : ' ',
         cpu->FR & (1 << Z) ? 'Z' : ' ');
  printf("SEC: %5d %04X\n", cpu->SEC, cpu->SEC);
  printf("NDX: %5d %04X\n", cpu->NDX, cpu->NDX);
  printf("SP:        %04X\n", cpu->SP);
}

void cpu_dump_ram_range(cpu_t *cpu, uint16_t from, uint16_t to) {
  assert(cpu);
  assert(from <= to);

  printf("RAM:\n");
  for (int i = from - from % 2; i < to + 1 - to % 2; i += 2) {
    printf("\t%04X   %02X %02X %d\n", i, cpu->RAM[i], cpu->RAM[i + 1], cpu_read16(cpu, i));
  }
}

void cpu_dump_stdout(cpu_t *cpu, uint16_t pos) {
  assert(cpu);
  assert(pos % 2 == 0);

  int to = cpu_read16(cpu, pos + 2);
  printf("STDOUT 0x%04X:\n", pos);
  printf("  w: [%d] r: [%d] e: [%d]\n",
         cpu_read16(cpu, pos) - pos - 6,
         cpu_read16(cpu, pos + 2) - pos - 6,
         cpu_read16(cpu, pos + 4) - pos - 6);
  for (int i = pos + 6; i < to; ++i) {
    switch (cpu->RAM[i]) {
      case '\t':
        printf("»");
        break;
      case ' ':
        printf("·");
        break;
      case '\n':
        printf("⏎\n");
        break;
      default:
        putchar(cpu->RAM[i]);
    }
  }
  printf("EOF\n");
}

void cpu_dump_stack(cpu_t *cpu, uint16_t count) {
  assert(cpu);
  printf("STACK:\n");
  for (int i = 0; i < count; ++i) {
    int at = cpu->SP + 2 * (i + 1);
    printf("\t%04X   %04X %d\n", at, cpu_read16(cpu, at), cpu_read16(cpu, at));
  }
}

void compute_screen(cpu_t *cpu) {
  assert(cpu);

  int index = cpu->GPUA & 0x7FFF;
  int x = 8 * (index & 0xFF);
  int y = 8 * ((index >> 8) & 0xFF);
  uint16_t attribute = cpu->ATTRIBUTE_RAM[index];
  uint16_t pattern_index = attribute & 0x0FFF;

  BeginTextureMode(cpu->screen);
  for (int dy = 0; dy < 8; ++dy) {
    for (int dx = 0; dx < 8; ++dx) {
      uint8_t color = (cpu->PATTERN_RAM[pattern_index * 8 + dy] >> (7 - dx)) & 1;
      DrawPixel(x + dx, SCREEN_HEIGHT - (y + dy) - 1, screen_palette[color]);
    }
  }
  EndTextureMode();

  // BeginTextureMode(cpu->screen);
  // ClearBackground(WHITE);
  // for (int y = 0; y < SCREEN_HEIGHT; ++y) {
  //   for (int x = 0; x < SCREEN_WIDTH; ++x) {
  //     uint16_t attribute = cpu->ATTRIBUTE_RAM[((y / 8) << 8) + (x / 8)];
  //     uint16_t pattern_index = attribute & 0x0FFF;
  //     uint8_t palette_index = (attribute >> 12) & 0xF;
  //     int dy = y & 0b111;
  //     int dx = x & 0b111;
  //     uint8_t color = (cpu->PATTERN_RAM[pattern_index * 8 + dy] >> (7 - dx)) & 1;
  // DrawPixel(x, SCREEN_HEIGHT - y - 1, color ? SCREEN_FOREGROUND : SCREEN_BACKGROUND);
  //   }
  // }
  // EndTextureMode();
}

void set_instruction_flags(instruction_t inst, uint8_t step, uint8_t flags, bool invert, microcode_t code) {
  for (int i = 0; i < 1 << 3; ++i) {
    if (invert ? !(flags & i) : flags & i) {
      control_rom[inst | (step << 6) | (i << (6 + 4))] = code;
    }
  }
}

void set_instruction_allflag(instruction_t inst, uint8_t step, microcode_t code) {
  for (int i = 0; i < 1 << 3; ++i) {
    control_rom[inst | (step << 6) | (i << (6 + 4))] = code;
  }
}

void set_control_rom() {
  for (int i = 0; i < 1 << 6; ++i) {
    set_instruction_allflag(i, 0, micro(IPo) | micro(MARi));
    set_instruction_allflag(i, 1, micro(RAM) | micro(RAMo) | micro(IRi) | micro(IPp));
  }
  set_instruction_allflag(NOP, 2, micro(SCr));
  set_instruction_allflag(INCA, 2, micro(Ao) | micro(Yi));
  set_instruction_allflag(INCA, 3, micro(Xi));
  set_instruction_allflag(INCA, 4, micro(Ci) | micro(ALUo) | micro(Ai));
  set_instruction_allflag(INCA, 5, micro(SCr));
  set_instruction_allflag(DECA, 2, micro(Ao) | micro(Yi));
  set_instruction_allflag(DECA, 3, micro(Xi));
  set_instruction_allflag(DECA, 4, micro(_SUB) | micro(ALUo) | micro(Ai));
  set_instruction_allflag(DECA, 5, micro(SCr));
  set_instruction_allflag(INCB, 2, micro(Bo) | micro(Yi));
  set_instruction_allflag(INCB, 3, micro(Xi));
  set_instruction_allflag(INCB, 4, micro(Ci) | micro(ALUo) | micro(Bi));
  set_instruction_allflag(INCB, 5, micro(SCr));
  set_instruction_allflag(RAM_AL, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(RAM_AL, 3, micro(RAM) | micro(RAMo) | micro(Ai) | micro(IPp));
  set_instruction_allflag(RAM_AL, 4, micro(SCr));
  set_instruction_allflag(RAM_BL, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(RAM_BL, 3, micro(RAM) | micro(RAMo) | micro(Bi) | micro(IPp));
  set_instruction_allflag(RAM_BL, 4, micro(SCr));
  set_instruction_allflag(RAM_A, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(RAM_A, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Ai) | micro(IPp));
  set_instruction_allflag(RAM_A, 4, micro(IPp));
  set_instruction_allflag(RAM_A, 5, micro(SCr));
  set_instruction_allflag(RAM_B, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(RAM_B, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Bi) | micro(IPp));
  set_instruction_allflag(RAM_B, 4, micro(IPp));
  set_instruction_allflag(RAM_B, 5, micro(SCr));
  set_instruction_allflag(INCSP, 2, micro(SPu));
  set_instruction_allflag(INCSP, 3, micro(SCr));
  set_instruction_allflag(DECSP, 2, micro(SPu) | micro(SPm));
  set_instruction_allflag(DECSP, 3, micro(SCr));
  set_instruction_allflag(PUSHA, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(PUSHA, 3, micro(Ao) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(PUSHA, 4, micro(SCr));
  set_instruction_allflag(POPA, 2, micro(SPu));
  set_instruction_allflag(POPA, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(POPA, 4, micro(RAM) | micro(RAM16) | micro(RAMo) | micro(Ai));
  set_instruction_allflag(POPA, 5, micro(SCr));
  set_instruction_allflag(PEEKA, 2, micro(SPu));
  set_instruction_allflag(PEEKA, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(PEEKA, 4, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Ai) | micro(SPu) | micro(SPm));
  set_instruction_allflag(PEEKA, 5, micro(SCr));
  set_instruction_allflag(PEEKAR, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(PEEKAR, 3, micro(RAM) | micro(RAMo) | micro(Yi) | micro(IPp));
  set_instruction_allflag(PEEKAR, 4, micro(SPo) | micro(Xi));
  set_instruction_allflag(PEEKAR, 5, micro(ALUo) | micro(MARi));
  set_instruction_allflag(PEEKAR, 6, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Ai));
  set_instruction_allflag(PEEKAR, 7, micro(SCr));
  set_instruction_allflag(PUSHAR, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(PUSHAR, 3, micro(RAM) | micro(RAMo) | micro(Yi) | micro(IPp));
  set_instruction_allflag(PUSHAR, 4, micro(SPo) | micro(Xi));
  set_instruction_allflag(PUSHAR, 5, micro(ALUo) | micro(MARi));
  set_instruction_allflag(PUSHAR, 6, micro(Ao) | micro(RAM) | micro(RAM16));
  set_instruction_allflag(PUSHAR, 7, micro(SCr));
  set_instruction_allflag(PUSHB, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(PUSHB, 3, micro(Bo) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(PUSHB, 4, micro(SCr));
  set_instruction_allflag(POPB, 2, micro(SPu));
  set_instruction_allflag(POPB, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(POPB, 4, micro(RAM) | micro(RAM16) | micro(RAMo) | micro(Bi));
  set_instruction_allflag(POPB, 5, micro(SCr));
  set_instruction_allflag(PEEKB, 2, micro(SPu));
  set_instruction_allflag(PEEKB, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(PEEKB, 4, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Bi) | micro(SPu) | micro(SPm));
  set_instruction_allflag(PEEKB, 5, micro(SCr));
  set_instruction_allflag(SUM, 2, micro(Ao) | micro(Xi));
  set_instruction_allflag(SUM, 3, micro(Bo) | micro(Yi));
  set_instruction_allflag(SUM, 4, micro(ALUo) | micro(Ai));
  set_instruction_allflag(SUM, 5, micro(SCr));
  set_instruction_allflag(SUB, 2, micro(Ao) | micro(Xi));
  set_instruction_allflag(SUB, 3, micro(Bo) | micro(Yi));
  set_instruction_allflag(SUB, 4, micro(ALUo) | micro(Ci) | micro(_SUB) | micro(Ai));
  set_instruction_allflag(SUB, 5, micro(SCr));
  set_instruction_allflag(SHL, 2, micro(Ao) | micro(Xi));
  set_instruction_allflag(SHL, 3, micro(Ao) | micro(Yi));
  set_instruction_allflag(SHL, 4, micro(ALUo) | micro(Ai));
  set_instruction_allflag(SHL, 5, micro(SCr));
  set_instruction_allflag(SHR, 2, micro(Ao) | micro(Xi));
  set_instruction_allflag(SHR, 3, micro(ALUo) | micro(_SHR) | micro(Ai));
  set_instruction_allflag(SHR, 4, micro(SCr));
  set_instruction_allflag(AND, 2, micro(Ao) | micro(Xi));
  set_instruction_allflag(AND, 3, micro(Bo) | micro(Yi));
  set_instruction_allflag(AND, 4, micro(ALUo) | micro(_AND) | micro(Ai));
  set_instruction_allflag(AND, 5, micro(SCr));
  set_instruction_allflag(CMPA, 2, micro(Ao) | micro(Xi));
  set_instruction_allflag(CMPA, 3, micro(Yi));
  set_instruction_allflag(CMPA, 4, micro(ALUo));
  set_instruction_allflag(CMPA, 5, micro(SCr));
  set_instruction_allflag(CMPB, 2, micro(Bo) | micro(Xi));
  set_instruction_allflag(CMPB, 3, micro(Yi));
  set_instruction_allflag(CMPB, 4, micro(ALUo));
  set_instruction_allflag(CMPB, 5, micro(SCr));
  set_instruction_allflag(JMP, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(JMP, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(IPi));
  set_instruction_allflag(JMP, 4, micro(SCr));
  set_instruction_allflag(JMPR, 2, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_allflag(JMPR, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_allflag(JMPR, 4, micro(ALUo) | micro(IPi));
  set_instruction_allflag(JMPR, 5, micro(SCr));
  set_instruction_allflag(JMPRZ, 2, micro(IPp));
  set_instruction_allflag(JMPRZ, 3, micro(IPp));
  set_instruction_allflag(JMPRZ, 4, micro(SCr));
  set_instruction_flags(JMPRZ, 2, 0b001, false, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(JMPRZ, 3, 0b001, false, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRZ, 4, 0b001, false, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRZ, 5, 0b001, false, micro(SCr));
  set_instruction_allflag(JMPRN, 2, micro(IPp));
  set_instruction_allflag(JMPRN, 3, micro(IPp));
  set_instruction_allflag(JMPRN, 4, micro(SCr));
  set_instruction_flags(JMPRN, 2, 0b010, false, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(JMPRN, 3, 0b010, false, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRN, 4, 0b010, false, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRN, 5, 0b010, false, micro(SCr));
  set_instruction_allflag(JMPRC, 2, micro(IPp));
  set_instruction_allflag(JMPRC, 3, micro(IPp));
  set_instruction_allflag(JMPRC, 4, micro(SCr));
  set_instruction_flags(JMPRC, 2, 0b100, false, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(JMPRC, 3, 0b100, false, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRC, 4, 0b100, false, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRC, 5, 0b100, false, micro(SCr));
  set_instruction_allflag(JMPRNZ, 2, micro(IPp));
  set_instruction_allflag(JMPRNZ, 3, micro(IPp));
  set_instruction_allflag(JMPRNZ, 4, micro(SCr));
  set_instruction_flags(JMPRNZ, 2, 0b001, true, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(JMPRNZ, 3, 0b001, true, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRNZ, 4, 0b001, true, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRNZ, 5, 0b001, true, micro(SCr));
  set_instruction_allflag(JMPRNN, 2, micro(IPp));
  set_instruction_allflag(JMPRNN, 3, micro(IPp));
  set_instruction_allflag(JMPRNN, 4, micro(SCr));
  set_instruction_flags(JMPRNN, 2, 0b010, true, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(JMPRNN, 3, 0b010, true, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRNN, 4, 0b010, true, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRNN, 5, 0b010, true, micro(SCr));
  set_instruction_allflag(JMPRNC, 2, micro(IPp));
  set_instruction_allflag(JMPRNC, 3, micro(IPp));
  set_instruction_allflag(JMPRNC, 4, micro(SCr));
  set_instruction_flags(JMPRNC, 2, 0b100, true, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(JMPRNC, 3, 0b100, true, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRNC, 4, 0b100, true, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRNC, 5, 0b100, true, micro(SCr));
  set_instruction_allflag(JMPA, 2, micro(Ao) | micro(IPi));
  set_instruction_allflag(JMPA, 3, micro(SCr));
  set_instruction_allflag(JMPAR, 2, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_allflag(JMPAR, 3, micro(Ao) | micro(Xi));
  set_instruction_allflag(JMPAR, 4, micro(ALUo) | micro(IPi));
  set_instruction_allflag(JMPAR, 5, micro(SCr));
  set_instruction_allflag(A_B, 2, micro(Ao) | micro(Bi));
  set_instruction_allflag(A_B, 3, micro(SCr));
  set_instruction_allflag(B_A, 2, micro(Bo) | micro(Ai));
  set_instruction_allflag(B_A, 3, micro(SCr));
  set_instruction_allflag(B_AH, 2, micro(Bo) | micro(AHi));
  set_instruction_allflag(B_AH, 3, micro(SCr));
  set_instruction_allflag(AL_rB, 2, micro(Bo) | micro(MARi));
  set_instruction_allflag(AL_rB, 3, micro(Ao) | micro(RAM));
  set_instruction_allflag(AL_rB, 4, micro(SCr));
  set_instruction_allflag(A_rB, 2, micro(Bo) | micro(MARi));
  set_instruction_allflag(A_rB, 3, micro(Ao) | micro(RAM) | micro(RAM16));
  set_instruction_allflag(A_rB, 4, micro(SCr));
  set_instruction_allflag(rB_AL, 2, micro(Bo) | micro(MARi));
  set_instruction_allflag(rB_AL, 3, micro(RAM) | micro(RAMo) | micro(Ai));
  set_instruction_allflag(rB_AL, 4, micro(SCr));
  set_instruction_allflag(rB_A, 2, micro(Bo) | micro(MARi));
  set_instruction_allflag(rB_A, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Ai));
  set_instruction_allflag(rB_A, 4, micro(SCr));
  set_instruction_allflag(A_SP, 2, micro(Ao) | micro(SPi));
  set_instruction_allflag(A_SP, 3, micro(SCr));
  set_instruction_allflag(SP_A, 2, micro(SPo) | micro(Ai));
  set_instruction_allflag(SP_A, 3, micro(SCr));
  set_instruction_allflag(A_SEC, 2, micro(Ao) | micro(SECi));
  set_instruction_allflag(A_SEC, 3, micro(SCr));
  set_instruction_allflag(SEC_A, 2, micro(SECo) | micro(Ai));
  set_instruction_allflag(SEC_A, 3, micro(SCr));
  set_instruction_allflag(RAM_NDX, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(RAM_NDX, 3, micro(RAM) | micro(RAMo) | micro(NDXi) | micro(IPp));
  set_instruction_allflag(RAM_NDX, 4, micro(SCr));
  set_instruction_allflag(INCNDX, 2, micro(NDXo) | micro(Yi));
  set_instruction_allflag(INCNDX, 3, micro(Xi));
  set_instruction_allflag(INCNDX, 4, micro(Ci) | micro(ALUo) | micro(NDXi));
  set_instruction_allflag(INCNDX, 5, micro(SCr));
  set_instruction_allflag(NDX_A, 2, micro(NDXo) | micro(Ai));
  set_instruction_allflag(NDX_A, 3, micro(SCr));
  set_instruction_allflag(A_NDX, 2, micro(Ao) | micro(NDXi));
  set_instruction_allflag(A_NDX, 3, micro(SCr));
  set_instruction_allflag(MEM_A, 2, micro(MEMo) | micro(Ai));
  set_instruction_allflag(MEM_A, 3, micro(SCr));
  set_instruction_allflag(MEM_AH, 2, micro(MEMo) | micro(AHi));
  set_instruction_allflag(MEM_AH, 3, micro(SCr));
  set_instruction_allflag(A_MEM, 2, micro(Ao) | micro(MEMi));
  set_instruction_allflag(A_MEM, 3, micro(SCr));
  set_instruction_allflag(CALL, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(CALL, 3, micro(IPo) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(CALL, 4, micro(IPo) | micro(MARi));
  set_instruction_allflag(CALL, 5, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(IPi));
  set_instruction_allflag(CALL, 6, micro(SCr));
  set_instruction_allflag(CALLR, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(CALLR, 3, micro(IPo) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(CALLR, 4, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_allflag(CALLR, 5, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_allflag(CALLR, 6, micro(ALUo) | micro(IPi));
  set_instruction_allflag(CALLR, 7, micro(SCr));
  set_instruction_allflag(RET, 2, micro(SPu));
  set_instruction_allflag(RET, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(RET, 4, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(IPi));
  set_instruction_allflag(RET, 5, micro(IPp));
  set_instruction_allflag(RET, 6, micro(IPp));
  set_instruction_allflag(RET, 7, micro(SCr));
  set_instruction_allflag(_KEY_A, 2, micro(KEYo) | micro(Ai));
  set_instruction_allflag(_KEY_A, 3, micro(SCr));
  set_instruction_allflag(DRW, 2, micro(Bo) | micro(GPUAi));
  set_instruction_allflag(DRW, 3, micro(Ao) | micro(GPUi));
  set_instruction_allflag(DRW, 4, micro(SCr));
  set_instruction_allflag(RAM_DRW, 2, micro(Bo) | micro(GPUAi));
  set_instruction_allflag(RAM_DRW, 3, micro(IPo) | micro(MARi));
  set_instruction_allflag(RAM_DRW, 4, micro(RAM) | micro(RAMo) | micro(GPUi) | micro(IPp));
  set_instruction_allflag(RAM_DRW, 5, micro(SCr));
  set_instruction_allflag(HLT, 2, micro(_HLT));
}

// look at https://www.vetra.com/scancodes.html
// based on https://wiki.osdev.org/PS/2_Keyboard scan code set 1
static int char_to_scancode[256] = {
    ['1'] = 0x02,
    ['2'] = 0x03,
    ['3'] = 0x04,
    ['4'] = 0x05,
    ['5'] = 0x06,
    ['6'] = 0x07,
    ['7'] = 0x08,
    ['8'] = 0x09,
    ['9'] = 0x0A,
    ['0'] = 0x0B,
    ['-'] = 0x0C,
    ['='] = 0x0D,
    ['\t'] = 0x0F,
    ['q'] = 0x10,
    ['w'] = 0x11,
    ['e'] = 0x12,
    ['r'] = 0x13,
    ['t'] = 0x14,
    ['y'] = 0x15,
    ['u'] = 0x16,
    ['i'] = 0x17,
    ['o'] = 0x18,
    ['p'] = 0x19,
    ['['] = 0x1A,
    [']'] = 0x1B,
    ['\n'] = 0x1C,
    ['a'] = 0x1E,
    ['s'] = 0x1F,
    ['d'] = 0x20,
    ['f'] = 0x21,
    ['g'] = 0x22,
    ['h'] = 0x23,
    ['j'] = 0x24,
    ['k'] = 0x25,
    ['l'] = 0x26,
    [';'] = 0x27,
    ['\''] = 0x28,
    ['`'] = 0x29,
    ['\\'] = 0x2B,
    ['z'] = 0x2C,
    ['x'] = 0x2D,
    ['c'] = 0x2E,
    ['v'] = 0x2F,
    ['b'] = 0x30,
    ['n'] = 0x31,
    ['m'] = 0x32,
    [','] = 0x33,
    ['.'] = 0x34,
    ['/'] = 0x35,
    [' '] = 0x39,
    ['!'] = -0x02,
    ['@'] = -0x03,
    ['#'] = -0x04,
    ['$'] = -0x05,
    ['%'] = -0x06,
    ['^'] = -0x07,
    ['&'] = -0x08,
    ['*'] = -0x09,
    ['('] = -0x0A,
    [')'] = -0x0B,
    ['_'] = -0x0C,
    ['+'] = -0x0D,
    ['Q'] = -0x10,
    ['W'] = -0x11,
    ['E'] = -0x12,
    ['R'] = -0x13,
    ['T'] = -0x14,
    ['Y'] = -0x15,
    ['U'] = -0x16,
    ['I'] = -0x17,
    ['O'] = -0x18,
    ['P'] = -0x19,
    ['{'] = -0x1A,
    ['}'] = -0x1B,
    ['A'] = -0x1E,
    ['S'] = -0x1F,
    ['D'] = -0x20,
    ['F'] = -0x21,
    ['G'] = -0x22,
    ['H'] = -0x23,
    ['J'] = -0x24,
    ['K'] = -0x25,
    ['L'] = -0x26,
    [':'] = -0x27,
    ['"'] = -0x28,
    ['~'] = -0x29,
    ['|'] = -0x2B,
    ['Z'] = -0x2C,
    ['X'] = -0x2D,
    ['C'] = -0x2E,
    ['V'] = -0x2F,
    ['B'] = -0x30,
    ['N'] = -0x31,
    ['M'] = -0x32,
    ['<'] = -0x33,
    ['>'] = -0x34,
    ['?'] = -0x35,
};

void load_input_string(cpu_t *cpu, char *string) {
  assert(cpu);
  assert(string);

  for (; *string; ++string) {
    if (*string == '\\') {
      ++string;
      switch (*string) {
        case 'n': *string = '\n'; break;
        case 'D':
          // end of input -> 0xFF
          assert(cpu->key_fifo_i + 4 < 1000);
          cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x1D; // left control pressed
          cpu->KEY_FIFO[cpu->key_fifo_i++] = char_to_scancode['d'];
          cpu->KEY_FIFO[cpu->key_fifo_i++] = char_to_scancode['d'] + 0x80;
          cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x9D; // left control released
          ++string;
          continue;
        default: eprintf("no scancode for '\\%c'", *string);
      }
    }

    int code = char_to_scancode[(int)*string];
    if (code == 0) {
      eprintf("no scancode for '%c'", *string);
    }

    if (code < 0) {
      assert(-code < 0x80);
      assert(cpu->key_fifo_i + 4 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x2A; // left shif pressed
      cpu->KEY_FIFO[cpu->key_fifo_i++] = -code;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = -code + 0x80;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0xAA; // left shift released
    } else {
      assert(code < 0x80);
      assert(cpu->key_fifo_i + 2 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code + 0x80;
    }
  }
  printf("\n");
}

bool check_microcode(microcode_t mc, microcode_flag_t flag) {
  return (mc >> flag) & 1;
}

void tick(cpu_t *cpu, bool *running) {
  assert(cpu);
  assert(running);
  assert(control_rom[cpu->IR & 0b111111]);

  uint16_t bus = 0;
  microcode_t mc = control_rom[(cpu->IR & 0b111111) | (cpu->SC << 6) | (cpu->FR << (6 + 4))];

  // dumpsbit(mc); putchar('\n');

  if (check_microcode(mc, _HLT)) {
    *running = false;
    cpu->SC = 0;
  }
  if (check_microcode(mc, IPo)) {
    bus = cpu->IP;
  }
  if (check_microcode(mc, Ao)) {
    bus = cpu->A;
  }
  if (check_microcode(mc, Bo)) {
    bus = cpu->B;
  }
  if ((check_microcode(mc, RAM)) && check_microcode(mc, RAMo)) {
    if (check_microcode(mc, RAM16)) {
      bus = (cpu->RAM[((cpu->MAR / 2) * 2) + 1] << 8) | cpu->RAM[(cpu->MAR / 2) * 2];
    } else {
      bus = cpu->RAM[cpu->MAR];
    }
  }
  if (check_microcode(mc, ALUo)) {
    if (check_microcode(mc, _AND)) {
      bus = cpu->X & cpu->Y;
      cpu->FR = (bus == 0) | ((bus >> 15) << 1);
    } else if (check_microcode(mc, _SHR)) {
      bus = cpu->X >> 1;
      cpu->FR = (bus == 0) | ((bus >> 15) << 1) | ((cpu->X & 1) << 2);
    } else {
      uint16_t result = cpu->X;
      if (check_microcode(mc, _SUB)) {
        result ^= 0xFFFF;
      }
      if (check_microcode(mc, Ci)) {
        ++result;
      }
      bus = result + cpu->Y;
      cpu->FR = (bus == 0) | ((bus >> 15) << 1) | ((((uint32_t)result + cpu->Y) >> 16) << 2);
    }
  }
  if (check_microcode(mc, SPo)) {
    bus = cpu->SP;
  }
  if (check_microcode(mc, SPu)) {
    cpu->SP += check_microcode(mc, SPm) ? -2 : 2;
  }
  if (check_microcode(mc, NDXo)) {
    bus = cpu->NDX;
  }
  if (check_microcode(mc, MEMo)) {
    bus = cpu->MEM[cpu->NDX | (cpu->SEC << 8)];
  }
  if (check_microcode(mc, SECo)) {
    bus = cpu->SEC;
  }
  if (check_microcode(mc, KEYo)) {
    // if (!cpu->KEY_FIFO[0]) { cpu->KEY_FIFO[0] = getchar(); }
    bus = cpu->KEY_FIFO[0];
    memcpy(cpu->KEY_FIFO, cpu->KEY_FIFO + 1, cpu->key_fifo_i);
    cpu->key_fifo_i--;
  }
  if (check_microcode(mc, IPi)) {
    cpu->IP = bus;
  }
  if (check_microcode(mc, IPp)) {
    ++cpu->IP;
  }
  if (check_microcode(mc, Ai)) {
    cpu->A = bus;
  }
  if (check_microcode(mc, AHi)) {
    cpu->A = (bus << 8) | (cpu->A & 0xFF);
  }
  if (check_microcode(mc, MARi)) {
    cpu->MAR = bus;
  }
  if (check_microcode(mc, Bi)) {
    cpu->B = bus;
  }
  if ((check_microcode(mc, RAM)) && !check_microcode(mc, RAMo)) {
    if (check_microcode(mc, RAM16)) {
      cpu->RAM[((cpu->MAR / 2) * 2) + 1] = bus >> 8;
      cpu->RAM[(cpu->MAR / 2) * 2] = bus & 0xFF;
    } else {
      cpu->RAM[cpu->MAR] = bus;
    }
  }
  if (check_microcode(mc, Xi)) {
    cpu->X = bus;
  }
  if (check_microcode(mc, Yi)) {
    cpu->Y = bus;
  }
  if (check_microcode(mc, SPi)) {
    cpu->SP = bus;
  }
  if (check_microcode(mc, IRi)) {
    cpu->IR = bus & 0xFF;
  }
  if (check_microcode(mc, SCr)) {
    cpu->SC = 0;
    return;
  }
  if (check_microcode(mc, SECi)) {
    cpu->SEC = bus & ((1 << 12) - 1);
  }
  if (check_microcode(mc, NDXi)) {
    cpu->NDX = bus;
  }
  if (check_microcode(mc, MEMi)) {
    cpu->MEM[cpu->NDX | (cpu->SEC << 8)] = bus & 0xFF;
  }
  if (check_microcode(mc, GPUAi)) {
    cpu->GPUA = bus;
  }
  if (check_microcode(mc, GPUi)) {
    if ((cpu->GPUA >> 15) & 1) {
      cpu->PATTERN_RAM[cpu->GPUA & 0x7FFF] = bus & 0xFF;
    } else {
      cpu->ATTRIBUTE_RAM[cpu->GPUA & 0x7FFF] = bus & 0xFF;

      if (cpu->has_screen) {
        compute_screen(cpu);
        BeginDrawing();
        ClearBackground(RED);
        DrawTextureEx(cpu->screen.texture, (Vector2){SCREEN_PAD, SCREEN_PAD}, 0, SCREEN_ZOOM, WHITE);
        EndDrawing();
      }
    }
  }

  cpu->SC = (cpu->SC + 1) % 16;
  cpu->ticks++;
}

void test_init(test_t *test, char *mem_path) {
  assert(test);
  *test = (test_t){0};
  cpu_init(&test->cpu, mem_path);
  test->running = 1;
}

void test_check(test_t *test) {
  assert(test);
  // for (int i = 0; i < 1 << 16; ++i) {
  //   if (test->test_ram[i] >= 0 && test->test_ram[i] != test->cpu.RAM[i]) {
  //     int j = i;
  //     while (
  //         j + 1 < 1 << 16
  //         && ((test->test_ram[j] >= 0 && test->test_ram[j] != test->cpu.RAM[j])
  //             || (test->test_ram[j + 1] >= 0 && test->test_ram[j + 1] != test->cpu.RAM[j + 1]))) {
  //       ++j;
  //     }
  //     if (!fail) {
  //       printf("ERROR:\n");
  //       fail = 1;
  //     }
  //     printf("- %04X |", i - i % 2);
  //     for (int k = i - i % 2; k < j; k += 2) {
  //       printf(" %04X", cpu_read16(&test->cpu, k));
  //     }
  //     printf("\n+ %04X |", i - i % 2);
  //     for (int k = i - i % 2; k < j; k += 2) {
  //       printf(" %04X", test->test_ram[k] | (test->test_ram[k + 1] << 8));
  //     }
  //     printf("\n\n");
  //     i = j;
  //   }
  // }
  // if (fail) {
  //   exit(1);
  // }
  // return;
  int fail = 0;
  for (int i = 0; i < 1 << 16; ++i) {
    if (test->test_ram[i] >= 0 && test->test_ram[i] != test->cpu.RAM[i]) {
      if (!fail) {
        fprintf(stderr, "ERROR:\n");
        fprintf(stderr, "                    expected | found\n");
      }
      fail = 1;
      fprintf(stderr, "  0x%04X [%2d+%-4d]        %02X | %02X", i, i / PAGE_SIZE, i - (i / PAGE_SIZE) * PAGE_SIZE, test->test_ram[i], test->cpu.RAM[i]);
      if (isprint(test->cpu.RAM[i])) {
        fprintf(stderr, "    '%c'", test->cpu.RAM[i]);
      }
      fprintf(stderr, "\n");
      if (i % 2 == 0) {
        fprintf(stderr, "                        %04X | %04X\n", test->test_ram[i] | (test->test_ram[i + 1] << 8), cpu_read16(&test->cpu, i));
      }
    }
  }
  int failgpu = 0;
  for (int i = 0; i < 1 << 15; ++i) {
    if (test->test_gpu[i] != test->cpu.ATTRIBUTE_RAM[i]) {
      if (!failgpu) {
        fprintf(stderr, "ERROR GPU:\n");
        fprintf(stderr, "             expected | found\n");
      }
      failgpu = 1;
      fprintf(stderr, "  %02d:%02d:     %3d  '%c' | %3d '%c'\n", (i >> 8) & 0xFF, i & 0xFF, test->test_gpu[i], isprint(test->test_gpu[i]) ? test->test_gpu[i] : ' ', test->cpu.ATTRIBUTE_RAM[i], isprint(test->cpu.ATTRIBUTE_RAM[i]) ? test->cpu.ATTRIBUTE_RAM[i] : ' ');
    }
  }
  if (fail || failgpu) {
    exit(1);
  }
}

uint16_t test_find_symbol(symbol_t *symbols, uint16_t count, char *image) {
  assert(symbols);
  assert(image);
  for (int i = 0; i < count; i++) {
    if (strcmp(symbols[i].image, image) == 0) {
      return symbols[i].pos;
    }
  }
  for (int i = 0; i < count; i++) {
    if (strlen(symbols[i].image) > 4 && strcmp(symbols[i].image + 4, image) == 0) {
      return symbols[i].pos;
    }
  }
  eprintf("image not found: %s", image);
  assert(0 && "UNREACHABLE");
}

void test_set_range(test_t *test, int ram_start, int count, uint8_t *data) {
  assert(test);
  assert(data);
  assert(count);
  for (int i = 0; i < count; ++i) {
    test->test_ram[i + ram_start] = data[i];
  }
}

void test_set_u16(test_t *test, uint16_t at, uint16_t num) {
  assert(test);
  assert(at % 2 == 0);
  test->test_ram[at] = num & 0xFF;
  test->test_ram[at + 1] = (num >> 8) & 0xFF;
}

void test_unset_range(test_t *test, uint16_t from, uint16_t count) {
  assert(test);
  memset(test->test_ram + from, -1, count * sizeof(test->test_ram[0]));
}

void test_gpu_put_char(test_t *test, char c) {
  assert(test);
  if (c == '\n') {
    test->gpu_x = 0;
    test->gpu_y++;
    return;
  }
  test->test_gpu[test->gpu_x + (test->gpu_y << 8)] = c;
  test->gpu_x++;
  if (test->gpu_x > 0x4A) {
    test->gpu_x = 0;
    test->gpu_y++;
  }
}

void test_gpu_print(test_t *test, char *str) {
  assert(test);
  assert(str);
  for (int i = 0; str[i]; ++i) {
    test_gpu_put_char(test, str[i]);
  }
}

void test_print_ram_range(test_t *test, uint16_t from, uint16_t to) {
  assert(test);
  printf("TEST RAM %d-%d:\n", from, to);
  int L = 32;
  from -= from % L;
  for (int i = from; i < to; i += L) {
    printf("%04X %05d |", i, i);
    for (int j = 0; j < L; ++j) {
      if (j % 8 == 0) {
        printf(" ");
      }
      if (test->test_ram[i + j] >= 0) {
        printf("%02X", test->test_ram[i + j]);
      } else {
        printf("  ");
      }
    }
    printf(" |\n");
  }
  printf("%04X %05d |\n", to, to);
}

void test_set_stdout(test_t *test, uint16_t stdout_pos, char *str) {
  assert(test);
  assert(str);

  assert(test->stdout_size + strlen(str) < 2048);
  strcat(test->stdout, str);
  test->stdout_size += strlen(str);

  test_set_u16(test, stdout_pos, 128 - test->stdout_size);
  test_set_u16(test, stdout_pos + 2, stdout_pos + 4 + test->stdout_size);
  test_set_range(test, stdout_pos + 4, test->stdout_size, (uint8_t *)test->stdout);
}

void exe_reloc(exe_t *exe, uint16_t start_pos, uint16_t stdlib_pos) {
  assert(exe);

  assert(exe->dynamic_count == 1);
  assert(strcmp(exe->dynamics[0].file_name, "\001") == 0);

  for (int i = 0; i < exe->reloc_count; ++i) {
    reloc_entry_t entry = exe->relocs[i];
    entry.what += start_pos;
    exe->code[entry.where] = entry.what & 0xFF;
    exe->code[entry.where + 1] = (entry.what >> 8) & 0xFF;
  }

  for (int i = 0; i < exe->dynamics[0].reloc_count; ++i) {
    reloc_entry_t entry = exe->dynamics[0].relocs[i];
    entry.what += stdlib_pos;
    exe->code[entry.where] = entry.what & 0xFF;
    exe->code[entry.where + 1] = (entry.what >> 8) & 0xFF;
  }
}

void so_reloc(so_t *so, uint16_t start_pos) {
  assert(so);

  for (int i = 0; i < so->reloc_count; ++i) {
    reloc_entry_t entry = so->relocs[i];
    entry.what += start_pos;
    so->code[entry.where] = entry.what & 0xFF;
    so->code[entry.where + 1] = (entry.what >> 8) & 0xFF;
  }
}

void test_run_command(test_t *test, char *command, char *input, char *exe_path, uint16_t sh_input_pos, uint16_t execute_pos, uint16_t exit_pos) {
  assert(test);
  assert(command);
  assert(exe_path);
  cpu_t *cpu = &test->cpu;
  bool running = true;

  uint16_t stdlib_pos = cpu_read16(cpu, 0xF800);

  int len = strlen(command);
  char _command[len];
  memcpy(_command, command, len);
  char *params = strchr(_command, ' ');
  if (params) {
    *params = 0;
  }

  load_input_string(cpu, command);
  load_input_string(cpu, "\n");
  load_input_string(cpu, input);

  printf("  LOAD `%s`\n", command);
  test_run_until((*test), cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);
  test_assert_running(*test);
  test_run_until((*test), cpu->IR == JMPA);
  test_assert_running(*test);

  exe_t exe = exe_decode_file(exe_path);
  exe_reloc(&exe, 3 * PAGE_SIZE, stdlib_pos);
  test_unset_range(test, 3 * PAGE_SIZE, PAGE_SIZE); // unset the stack and the code of the program and then set the code
  test_set_range(test, 3 * PAGE_SIZE, exe.code_size, exe.code);

  test_set_range(test, sh_input_pos, len, (uint8_t *)_command);
  test->test_ram[sh_input_pos + len] = 0;

  test_gpu_print(test, "$ ");
  test_gpu_print(test, command);
  test_gpu_print(test, "\n");

  // os struct:
  test_set_u16(test, 0xF802, 0xF840);
  test_set_u16(test, 0xF804, 0b111 << 13);
  test_set_u16(test, 0xF806, 0b1111 << 12);
  // command process:
  test_set_u16(test, 0xF840, 0xF830);
  test_set_u16(test, 0xF842, cpu_read16(cpu, 0xF832));
  test_set_u16(test, 0xF844, 0);
  test_set_u16(test, 0xF846, cpu_read16(cpu, 0xF836));
  test_set_u16(test, 0xF848, cpu_read16(cpu, 0xF838));
  test_check(test);

  printf("  RUN `%s`\n", command);
  test_run_until(*test, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_pos);
  test_assert_running(*test);
  // while (running && !(cpu->IR == CALL && cpu->SC == 2 && cpu_read16(cpu, cpu->IP) == exit_pos)) {
  //   tick(cpu, &running);
  // }
  test_assert_running(*test);
  int calls = 0;
  while (running && calls != 0) {
    if (cpu->RAM[cpu->IP] == CALL || cpu->RAM[cpu->IP] == CALLR) {
      calls++;
    } else if (cpu->RAM[cpu->IP] == RET) {
      calls--;
    }
    tick(cpu, &running);
  }
  test_assert_running(*test);
  test_assert(cpu->A == 0);

  // os struct:
  test_set_u16(test, 0xF802, 0xF830);
  test_set_u16(test, 0xF804, 0b11 << 14);
  test_set_u16(test, 0xF806, 0b111 << 13);
}

void step_mode(cpu_t *cpu) {
  assert(cpu);

  printf("\nSTEP MODE:\ninput ? or help to see commands\n\n");

  cpu_dump(cpu);

  bool enable_cpu_dump = true;
  bool running = true;
  microcode_flag_t mc = 0;

  char input[512] = {0};

  printf("> ");
  while (fgets(input, 512, stdin)) {

    char *arg1 = input;
    strsep(&arg1, " \n");
    // for new args: arg1 += strlen(arg1) + 1; strsep(arg1, " \n"); more or less

    if (strcmp(input, "end") == 0 || strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0) {
      break;
    } else if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
      printf("commands\n"
             "  end | quit | exit   end step mode\n"
             "  help | ?            print help\n"
             "  read HEX            read u16 at the addr specified\n"
             "  c | continue        continue till the next HLT\n"
             "  stack INT           print INT items from the stack\n"
             "  str HEX             print from HEX as a null terminated string\n"
             "  cpu on|off          enable/disable printing the cpu state\n"
             "  skip                if on a CALL/CALLR skip to the RET\n"
             "if no command, next instruction is run\n\n");

    } else if (strcmp(input, "read") == 0) {
      if (!arg1) {
        printf("ERROR: addr not provided\n");
      } else {
        uint16_t addr = strtol(arg1, NULL, 16);
        if (addr % 2 != 0) {
          printf("ERROR: addr not even\n");
        } else {
          printf("0x%04X\n", cpu_read16(cpu, addr));
        }
      }
      printf("\n");

    } else if (strcmp(input, "c") == 0 || strcmp(input, "continue") == 0) {
      cpu->SC = 0;
      running = true;
      do {
        tick(cpu, &running);
      } while (running);

    } else if (strcmp(input, "stack") == 0) {
      uint16_t count = arg1 ? atoi(arg1) : 0;
      cpu_dump_stack(cpu, count ? count : 5);
      printf("\n");

    } else if (strcmp(input, "str") == 0) {
      if (!arg1) {
        printf("ERROR: addr not provided\n");
      } else {
        uint16_t addr = strtol(arg1, NULL, 16);
        printf("%04X: ", addr);
        for (int i = addr; cpu->RAM[i] != 0; ++i) {
          printf("%c", cpu->RAM[i]);
        }
        printf("\n");
      }
      printf("\n");

    } else if (strcmp(input, "cpu") == 0) {
      if (!arg1 || strcmp(arg1, "off") == 0) {
        enable_cpu_dump = false;
      } else if (strcmp(arg1, "on") == 0) {
        enable_cpu_dump = true;
      }

    } else if (strcmp(input, "skip") == 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        running = true;
        int n = 0;
        do {
          if (mc & (1 << SCr)) {
            if (cpu->IR == CALL || cpu->IR == CALLR) {
              n++;
            } else if (cpu->IR == RET) {
              n--;
            }
          }
          tick(cpu, &running);
          mc = control_rom[cpu->IR | (cpu->SC << 6) | (cpu->FR << (6 + 4))];
        } while (running && n != 0);

      } else {
        printf("ERROR: not CALL or CALLR\n");
      }

    } else {
      if (cpu->IR == HLT) {
        cpu->SC = 0;
      }
      running = true;
      do {
        tick(cpu, &running);
        mc = control_rom[cpu->IR | (cpu->SC << 6) | (cpu->FR << (6 + 4))];
      } while (running && !(mc & (1 << SCr)));
    }

    if (enable_cpu_dump) {
      cpu_dump(cpu);
    } else {
      printf("%s\n", instruction_to_string(cpu->IR));
    }

    memset(input, 0, sizeof(input));
    printf("> ");
  }
}
