#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <raylib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ERRORS_IMPLEMENTATION
#define SV_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"
#include "files.h"
#include "instructions.h"

#define TEST_MEM_PATH "test.mem.bin"

#define KEY_FIFO_SIZE 1024

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 350
#define SCREEN_ZOOM   2
#define SCREEN_PAD    10

Color screen_palette[] = {
    {0x34, 0x68, 0x56, 0xFF},
    {0x08, 0x18, 0x20, 0xFF},

    // {0x61, 0xA6, 0xB4, 0xFF},
    // {0x01, 0x46, 0x54, 0xFF},

    // {0xE2, 0x7E, 0x80, 0xFF},
    // {0x82, 0x1E, 0x20, 0xFF},
};

// clang-format off
typedef enum {
  _HLT,
  IPi, IPo, IPp,
  Ai, AHi, Ao,
  MARi,
  Bi, Bo,
  RAM, RAMo, RAM16,
  Xi, Yi, ALUo, _SUB, _SHR, Ci,
  SPi, SPo, SPu, SPm,
  IRi,
  SCr,
  SECi, SECo, NDXi, NDXo, MEMi, MEMo,
  KEYo,
  GPUAi, GPUi,
  MICROCODE_FLAG_COUNT
} microcode_flag_t;
// clang-format on

#define micro(__flag__) ((long long)1 << __flag__)

typedef uint64_t microcode_t;

microcode_t control_rom[1 << (6 + 4 + 3)] = {0};

typedef enum {
  Z,
  N,
  C
} flag_t;

typedef struct {
  uint16_t A;
  uint16_t B;
  uint16_t SP;
  uint16_t IP;
  uint16_t X;
  uint16_t Y;
  uint16_t MAR;
  uint16_t SEC;
  uint8_t NDX;
  uint8_t FR;
  uint8_t SC;
  uint8_t IR;
  uint8_t RAM[1 << 16];
  uint8_t MEM[1 << 20];

  uint8_t KEY_FIFO[KEY_FIFO_SIZE];
  int key_fifo_i;

  uint16_t GPUA;
  uint16_t ATTRIBUTE_RAM[1 << 15];
  uint8_t PATTERN_RAM[1 << 15];
  RenderTexture2D screen;
  bool has_screen;
} cpu_t;

void cpu_init(cpu_t *cpu, char *mem_path) {
  assert(cpu);

  FILE *file = fopen(mem_path, "rb");
  if (!file) {
    eprintf("cannot open file: '%s': %s\n", mem_path, strerror(errno));
  }
  assert(fread(cpu->MEM, 1, 1 << 19, file) == 1 << 19);
  assert(fclose(file) == 0);

  memcpy(cpu->RAM + 0xFF00, cpu->MEM, 256);
  cpu->IP = 0xFF00;
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

  printf("A:   %5d %04X %c\n", cpu->A, cpu->A, isprint(cpu->A) ? cpu->A : ' ');
  printf("B:   %5d %04X %c\n", cpu->B, cpu->B, isprint(cpu->B) ? cpu->B : ' ');
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
  printf("STDOUT 0x%04X: (%d chars)\n", pos, to - pos - 4);
  for (int i = pos + 4; i < to; ++i) {
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
  set_instruction_allflag(
      RAM_A, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Ai) | micro(IPp));
  set_instruction_allflag(RAM_A, 4, micro(IPp));
  set_instruction_allflag(RAM_A, 5, micro(SCr));
  set_instruction_allflag(RAM_B, 2, micro(IPo) | micro(MARi));
  set_instruction_allflag(
      RAM_B, 3, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Bi) | micro(IPp));
  set_instruction_allflag(RAM_B, 4, micro(IPp));
  set_instruction_allflag(RAM_B, 5, micro(SCr));
  set_instruction_allflag(INCSP, 2, micro(SPu));
  set_instruction_allflag(INCSP, 3, micro(SCr));
  set_instruction_allflag(DECSP, 2, micro(SPu) | micro(SPm));
  set_instruction_allflag(DECSP, 3, micro(SCr));
  set_instruction_allflag(PUSHA, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(
      PUSHA, 3, micro(Ao) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(PUSHA, 4, micro(SCr));
  set_instruction_allflag(POPA, 2, micro(SPu));
  set_instruction_allflag(POPA, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(POPA, 4, micro(RAM) | micro(RAM16) | micro(RAMo) | micro(Ai));
  set_instruction_allflag(POPA, 5, micro(SCr));
  set_instruction_allflag(PEEKA, 2, micro(SPu));
  set_instruction_allflag(PEEKA, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(
      PEEKA, 4, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Ai) | micro(SPu) | micro(SPm));
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
  set_instruction_allflag(
      PUSHB, 3, micro(Bo) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(PUSHB, 4, micro(SCr));
  set_instruction_allflag(POPB, 2, micro(SPu));
  set_instruction_allflag(POPB, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(POPB, 4, micro(RAM) | micro(RAM16) | micro(RAMo) | micro(Bi));
  set_instruction_allflag(POPB, 5, micro(SCr));
  set_instruction_allflag(PEEKB, 2, micro(SPu));
  set_instruction_allflag(PEEKB, 3, micro(SPo) | micro(MARi));
  set_instruction_allflag(
      PEEKB, 4, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Bi) | micro(SPu) | micro(SPm));
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
  set_instruction_flags(
      JMPRZ, 3, 0b001, false, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRZ, 4, 0b001, false, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRZ, 5, 0b001, false, micro(SCr));
  set_instruction_allflag(JMPRN, 2, micro(IPp));
  set_instruction_allflag(JMPRN, 3, micro(IPp));
  set_instruction_allflag(JMPRN, 4, micro(SCr));
  set_instruction_flags(JMPRN, 2, 0b010, false, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(
      JMPRN, 3, 0b010, false, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRN, 4, 0b010, false, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRN, 5, 0b010, false, micro(SCr));
  set_instruction_allflag(JMPRC, 2, micro(IPp));
  set_instruction_allflag(JMPRC, 3, micro(IPp));
  set_instruction_allflag(JMPRC, 4, micro(SCr));
  set_instruction_flags(JMPRC, 2, 0b100, false, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(
      JMPRC, 3, 0b100, false, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRC, 4, 0b100, false, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRC, 5, 0b100, false, micro(SCr));
  set_instruction_allflag(JMPRNZ, 2, micro(IPp));
  set_instruction_allflag(JMPRNZ, 3, micro(IPp));
  set_instruction_allflag(JMPRNZ, 4, micro(SCr));
  set_instruction_flags(JMPRNZ, 2, 0b001, true, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(
      JMPRNZ, 3, 0b001, true, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRNZ, 4, 0b001, true, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRNZ, 5, 0b001, true, micro(SCr));
  set_instruction_allflag(JMPRNN, 2, micro(IPp));
  set_instruction_allflag(JMPRNN, 3, micro(IPp));
  set_instruction_allflag(JMPRNN, 4, micro(SCr));
  set_instruction_flags(JMPRNN, 2, 0b010, true, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(
      JMPRNN, 3, 0b010, true, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
  set_instruction_flags(JMPRNN, 4, 0b010, true, micro(ALUo) | micro(IPi));
  set_instruction_flags(JMPRNN, 5, 0b010, true, micro(SCr));
  set_instruction_allflag(JMPRNC, 2, micro(IPp));
  set_instruction_allflag(JMPRNC, 3, micro(IPp));
  set_instruction_allflag(JMPRNC, 4, micro(SCr));
  set_instruction_flags(JMPRNC, 2, 0b100, true, micro(IPo) | micro(MARi) | micro(Yi));
  set_instruction_flags(
      JMPRNC, 3, 0b100, true, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(Xi));
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
  set_instruction_allflag(CALL, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(
      CALL, 3, micro(IPo) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
  set_instruction_allflag(CALL, 4, micro(IPo) | micro(MARi));
  set_instruction_allflag(CALL, 5, micro(RAM) | micro(RAMo) | micro(RAM16) | micro(IPi));
  set_instruction_allflag(CALL, 6, micro(SCr));
  set_instruction_allflag(CALLR, 2, micro(SPo) | micro(MARi));
  set_instruction_allflag(
      CALLR, 3, micro(IPo) | micro(RAM) | micro(RAM16) | micro(SPu) | micro(SPm));
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
int char_to_scancode[256] = {0};
void set_char_to_scancode() {
  char_to_scancode['1'] = 0x02;
  char_to_scancode['2'] = 0x03;
  char_to_scancode['3'] = 0x04;
  char_to_scancode['4'] = 0x05;
  char_to_scancode['5'] = 0x06;
  char_to_scancode['6'] = 0x07;
  char_to_scancode['7'] = 0x08;
  char_to_scancode['8'] = 0x09;
  char_to_scancode['9'] = 0x0A;
  char_to_scancode['0'] = 0x0B;
  char_to_scancode['-'] = 0x0C;
  char_to_scancode['='] = 0x0D;
  char_to_scancode['\t'] = 0x0F;
  char_to_scancode['q'] = 0x10;
  char_to_scancode['w'] = 0x11;
  char_to_scancode['e'] = 0x12;
  char_to_scancode['r'] = 0x13;
  char_to_scancode['t'] = 0x14;
  char_to_scancode['y'] = 0x15;
  char_to_scancode['u'] = 0x16;
  char_to_scancode['i'] = 0x17;
  char_to_scancode['o'] = 0x18;
  char_to_scancode['p'] = 0x19;
  char_to_scancode['['] = 0x1A;
  char_to_scancode[']'] = 0x1B;
  char_to_scancode['\n'] = 0x1C;
  char_to_scancode['a'] = 0x1E;
  char_to_scancode['s'] = 0x1F;
  char_to_scancode['d'] = 0x20;
  char_to_scancode['f'] = 0x21;
  char_to_scancode['g'] = 0x22;
  char_to_scancode['h'] = 0x23;
  char_to_scancode['j'] = 0x24;
  char_to_scancode['k'] = 0x25;
  char_to_scancode['l'] = 0x26;
  char_to_scancode[';'] = 0x27;
  char_to_scancode['\''] = 0x28;
  char_to_scancode['`'] = 0x29;
  char_to_scancode['\\'] = 0x2B;
  char_to_scancode['z'] = 0x2C;
  char_to_scancode['x'] = 0x2D;
  char_to_scancode['c'] = 0x2E;
  char_to_scancode['v'] = 0x2F;
  char_to_scancode['b'] = 0x30;
  char_to_scancode['n'] = 0x31;
  char_to_scancode['m'] = 0x32;
  char_to_scancode[','] = 0x33;
  char_to_scancode['.'] = 0x34;
  char_to_scancode['/'] = 0x35;
  char_to_scancode[' '] = 0x39;

  char_to_scancode['!'] = -0x02;
  char_to_scancode['@'] = -0x03;
  char_to_scancode['#'] = -0x04;
  char_to_scancode['$'] = -0x05;
  char_to_scancode['%'] = -0x06;
  char_to_scancode['^'] = -0x07;
  char_to_scancode['&'] = -0x08;
  char_to_scancode['*'] = -0x09;
  char_to_scancode['('] = -0x0A;
  char_to_scancode[')'] = -0x0B;
  char_to_scancode['_'] = -0x0C;
  char_to_scancode['+'] = -0x0D;
  char_to_scancode['Q'] = -0x10;
  char_to_scancode['W'] = -0x11;
  char_to_scancode['E'] = -0x12;
  char_to_scancode['R'] = -0x13;
  char_to_scancode['T'] = -0x14;
  char_to_scancode['Y'] = -0x15;
  char_to_scancode['U'] = -0x16;
  char_to_scancode['I'] = -0x17;
  char_to_scancode['O'] = -0x18;
  char_to_scancode['P'] = -0x19;
  char_to_scancode['{'] = -0x1A;
  char_to_scancode['}'] = -0x1B;
  char_to_scancode['A'] = -0x1E;
  char_to_scancode['S'] = -0x1F;
  char_to_scancode['D'] = -0x20;
  char_to_scancode['F'] = -0x21;
  char_to_scancode['G'] = -0x22;
  char_to_scancode['H'] = -0x23;
  char_to_scancode['J'] = -0x24;
  char_to_scancode['K'] = -0x25;
  char_to_scancode['L'] = -0x26;
  char_to_scancode[':'] = -0x27;
  char_to_scancode['"'] = -0x28;
  char_to_scancode['~'] = -0x29;
  char_to_scancode['|'] = -0x2B;
  char_to_scancode['Z'] = -0x2C;
  char_to_scancode['X'] = -0x2D;
  char_to_scancode['C'] = -0x2E;
  char_to_scancode['V'] = -0x2F;
  char_to_scancode['B'] = -0x30;
  char_to_scancode['N'] = -0x31;
  char_to_scancode['M'] = -0x32;
  char_to_scancode['<'] = -0x33;
  char_to_scancode['>'] = -0x34;
  char_to_scancode['?'] = -0x35;
}

void load_input_string(cpu_t *cpu, char *string) {
  assert(cpu);
  assert(string);

  for (; *string; ++string) {
    if (*string == '\\' && *(string + 1) == 'n') {
      ++string;
      *string = '\n';
    }

    int code = char_to_scancode[(int)*string];
    if (code == 0) {
      eprintf("no scancode for '%c'", *string);
    }

    if (code < 0) {
      assert(-code < 0x80);
      assert(cpu->key_fifo_i + 6 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0xE0;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0xAA;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = -code;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = -code + 0x80;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0xE0;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x2A;
    } else {
      assert(code < 0x80);
      assert(cpu->key_fifo_i + 2 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code + 0x80;
    }
  }
}

bool check_microcode(microcode_t mc, microcode_flag_t flag) {
  return (mc >> flag) & 1;
}

void tick(cpu_t *cpu, bool *running) {
  assert(cpu);
  assert(running);

  uint16_t bus = 0;
  microcode_t mc = control_rom[cpu->IR | (cpu->SC << 6) | (cpu->FR << (6 + 4))];

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
    if (check_microcode(mc, _SHR)) {
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
    assert(0);
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
        ClearBackground(WHITE);
        DrawTextureEx(cpu->screen.texture, (Vector2){SCREEN_PAD, SCREEN_PAD}, 0, SCREEN_ZOOM, WHITE);
        EndDrawing();
      }
    }
  }

  cpu->SC = (cpu->SC + 1) % 16;
}

// TODO: test mem
typedef struct {
  cpu_t cpu;
  int16_t test_ram[1 << 16]; // negative if undef

  char stdout[2048];
  int stdout_size;
} test_t;

void test_init(test_t *test) {
  assert(test);
  *test = (test_t){0};
  cpu_init(&test->cpu, TEST_MEM_PATH);
  memset(test->test_ram, -1, (1 << 16) * sizeof(test->test_ram[0]));
}

void test_check(test_t *test) {
  assert(test);
  int fail = 0;

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

  for (int i = 0; i < 1 << 16; ++i) {
    if (test->test_ram[i] >= 0 && test->test_ram[i] != test->cpu.RAM[i]) {
      if (!fail) {
        fprintf(stderr, "ERROR:\n");
      }
      fail = 1;
      fprintf(stderr, "  at 0x%04X %05d expected %02X,   found %02X\n", i, i, test->test_ram[i], test->cpu.RAM[i]);
      if (i % 2 == 0) {
        fprintf(stderr, "                  expected %04X, found %04X\n", test->test_ram[i] | (test->test_ram[i + 1] << 8), cpu_read16(&test->cpu, i));
      }
    }
  }
  if (fail) {
    exit(1);
  }
}

#define test_assert(cond__)               \
  do {                                    \
    if (!(cond__)) {                      \
      eprintf("failed assert: " #cond__); \
    }                                     \
  } while (0);

uint16_t test_find_symbol(symbol_t *symbols, uint16_t count, char *image) {
  assert(symbols);
  assert(image);
  for (int i = 0; i < count; i++) {
    if (strcmp(symbols[i].image, image) == 0) {
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

void step_mode(cpu_t *cpu) {
  assert(cpu);

  bool running = true;
  microcode_flag_t mc = 0;

  char input[100] = {0};

  cpu_dump(cpu);
  printf("> ");
  while (fgets(input, 100, stdin)) {
    cpu_dump(cpu);

    if (strcmp(input, "end\n") == 0 || strcmp(input, "quit\n") == 0 || strcmp(input, "exit\n") == 0) {
      break;
    } else if (strcmp(input, "help\n") == 0 || strcmp(input, "?\n") == 0) {
      printf("commands\n"
             "  end | quit | exit   end step mode\n"
             "  help | ?            print help\n"
             "if no command, next instruction is run\n");
    } else {
      running = true;
      do {
        tick(cpu, &running);
        mc = control_rom[cpu->IR | (cpu->SC << 6) | (cpu->FR << (6 + 4))];
      } while (running && !(mc & (1 << SCr)));
    }
    printf("> ");
  }
}

void test() {
  test_t test_ = {0};
  test_t *test = &test_;
  test_init(test);
  cpu_t *cpu = &test->cpu;
  bool running = true;

  printf("TEST:\n");

  printf("  BOOTLOADER LOADED\n");
  test_set_range(test, 0xFF00, 256, cpu->MEM);
  test_check(test);

  printf("  BOOTLOADER RUN\n");
  while (running && !(cpu->IR == JMPA && cpu->A == 0)) {
    tick(cpu, &running);
  }
  test_assert(running);

  printf("  OS LOADED\n");
  exe_t os = exe_decode_file("asm/bin/os");
  exe_reloc(&os, 0, os.code_size);
  test_set_range(test, 0, os.code_size, os.code);
  test_check(test);

  printf("  STDLIB LOADED\n");
  so_t stdlib = so_decode_file("asm/bin/stdlib");
  for (int i = 0; i < stdlib.reloc_count; ++i) {
    reloc_entry_t entry = stdlib.relocs[i];
    entry.what += os.code_size;
    stdlib.code[entry.where] = entry.what & 0xFF;
    stdlib.code[entry.where + 1] = (entry.what >> 8) & 0xFF;
  }
  test_set_range(test, os.code_size, stdlib.code_size, stdlib.code);
  test_check(test);

  uint16_t execute_ptr = test_find_symbol(stdlib.symbols, stdlib.symbol_count, "execute") + os.code_size;

  printf("  OS SETUP\n");
  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_ptr)) {
    tick(cpu, &running);
  }
  test_assert(running);
  test_assert(cpu->RAM[cpu->A] == 's');
  test_assert(cpu->RAM[cpu->A + 1] == 'h');
  test_assert(cpu->RAM[cpu->A + 2] == 0);

  printf("TODO STDOUT\n");
  // os struct:
  test_set_u16(test, 0xF800, os.code_size); // stdlib
  test_set_u16(test, 0xF802, 0xF820);       // current process
  test_set_u16(test, 0xF804, 0x8000);       // used process map
  test_set_u16(test, 0xF806, 0x8000);       // used page map
  test_set_u16(test, 0xF808, 0);            // used page map
  test_set_u16(test, 0xF80A, 0);            // screen text row - col
  //  os process:
  test_set_u16(test, 0xF820, 0xFFFF); // parent process
  test_set_u16(test, 0xF822, 1);      // cwd sec
  test_set_u16(test, 0xF824, 0xFFFE); //  SP
  // test_set_u16(test, 0xF826, stdout_pos); // stdout redirect
  test_check(test);

  printf("END\n");
}

void print_help() {
  printf("Usage: sim [options]\n\n"
         "Options:\n"
         "  -i | --input <str>  simulate input for the computer\n"
         "  -s | --step         run in step mode\n"
         "  -t | --test         run the test\n"
         "  -r <from>:<to>      print ram in the range\n"
         "  --screen            enable screen\n"
         "  --mem <path>        specify bin file path for memory\n"
         "  --stdout <num>      print stdout struct in ram from num\n"
         "  -h | --help         show help message\n");
}

int main(int argc, char **argv) {
  set_control_rom();

  char *input = "";
  char *mempath = "main.mem.bin";
  int step_mode = 0;
  int screen = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                        //
        else ARG_PARSE_FLAG("s", "step", step_mode)           //
        else ARG_PARSE_FLAG_(ARG_LFLAG("screen"), screen)     //
        else ARG_PARSE_STRING_ARG("i", "input", input)        //
        else ARG_PARSE_STRING_ARG_(ARG_LFLAG("mem"), mempath) //
        else ARG_IF_FLAG("t", "test") {
      test();
      return 0;
    }
    else if (ARG_SFLAG("r")) {
      if (*(argv + 1) == NULL) {
        eprintf("arg expected range: '%s'", *argv);
      }
      argv++;
      char *delim = strchr(*argv, ':');
      *delim = 0;
      int from = atoi(*argv);
      int to = atoi(delim + 1);
      printf("%d %d\n", from, to);
      exit(101);
    }
  }

  if (step_mode) {
    eprintf("TODO step mode");
  }

  cpu_t cpu = {0};
  cpu_init(&cpu, mempath);
  cpu.has_screen = screen;

  set_char_to_scancode();
  load_input_string(&cpu, input);

  if (cpu.has_screen) {
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(SCREEN_WIDTH * SCREEN_ZOOM + SCREEN_PAD * 2, SCREEN_HEIGHT * SCREEN_ZOOM + SCREEN_PAD * 2, "Jaris screen");
    cpu.screen = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    BeginTextureMode(cpu.screen);
    ClearBackground(screen_palette[0]);
    EndTextureMode();
  }

  bool running = true;
  int ticks = 0;
  while (running) {
    PollInputEvents();
    if (cpu.has_screen && WindowShouldClose()) {
      break;
    }
    tick(&cpu, &running);
    ++ticks;
  }
  if (cpu.has_screen) {
    while (!WindowShouldClose()) {
      PollInputEvents();
    }
    UnloadRenderTexture(cpu.screen);
    CloseWindow();
  }

  printf("ticks: %.3fE3 (%.3f ms @ 4 MHz, %.3f ms @ 10 MHz)\n",
         (float)ticks / 1E3,
         (float)ticks * 1E3 / 4E6,
         (float)ticks * 1E3 / 10E6);

  cpu_dump(&cpu);

  return 0;
}
