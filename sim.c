#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "argparse/argparse.h"

#define SV_IMPLEMENTATION
#include "instructions.h"

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

  MICROCODE_FLAG_COUNT
} microcode_flag_t;
// clang-format on

#define micro(__flag__) (1 << __flag__)

typedef uint64_t microcode_t;

microcode_t control_rom[1 << (6 + 4 + 3)] = {0};

typedef enum {
  Z,
  N,
  C,
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

  uint8_t KEY_FIFO[1000];

  uint16_t ATTRIBUTE_RAM[1 << 14];
  uint8_t PATTERN_RAM[1 << 14];
  uint8_t PALETTE_RAM[1 << 8];
  // RenderTexture2D screen;
} cpu_t;

void set_control_rom();
void cpu_init(cpu_t *cpu) {
  assert(cpu);

  set_control_rom();

  FILE *file = fopen("mem.bin", "rb");
  if (!file) {
    fprintf(stderr, "ERROR: cannot open file: 'mem.bin': %s\n", strerror(errno));
    exit(1);
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
  for (int i = from - from % 2; i < to + to % 2; i += 2) {
    printf("\t%04X   %02X %02X %d\n", i, cpu->RAM[i], cpu->RAM[i + 1], cpu_read16(cpu, i));
  }
}

void cpu_dump_stdout(cpu_t *cpu) {
  assert(cpu);

  uint16_t stdout_ptr = cpu_read16(cpu, 0xF80A);
  uint16_t stdout_next_char = cpu_read16(cpu, stdout_ptr);

  printf("STDOUT: %d\n", stdout_next_char - stdout_ptr - 4);
  for (uint16_t i = stdout_ptr + 4; i < stdout_next_char; ++i) {
    printf("%c", cpu->RAM[i]);
  }
  printf("\n");
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

void dumpsbit(uint32_t num) {
  putchar('0');
  putchar('b');
  for (int i = 0; i < 32; ++i) {
    putchar((num >> (31 - i)) & 1 ? '1' : '0');
  }
}

void tick(cpu_t *cpu, bool *running) {
  assert(cpu);
  assert(running);

  uint16_t bus = 0;
  microcode_t mc = control_rom[cpu->IR | (cpu->SC << 6) | (cpu->FR << (6 + 4))];

  // dumpsbit(mc); putchar('\n');

  if (mc & (1 << _HLT)) {
    *running = false;
  }
  if (mc & (1 << IPo)) {
    bus = cpu->IP;
  }
  if (mc & (1 << Ao)) {
    bus = cpu->A;
  }
  if (mc & (1 << Bo)) {
    bus = cpu->B;
  }
  if ((mc & (1 << RAM)) && (mc & (1 << RAMo))) {
    if (mc & (1 << RAM16)) {
      bus = (cpu->RAM[((cpu->MAR / 2) * 2) + 1] << 8) | cpu->RAM[(cpu->MAR / 2) * 2];
    } else {
      bus = cpu->RAM[cpu->MAR];
    }
  }
  if (mc & (1 << ALUo)) {
    if (mc & (1 << _SHR)) {
      bus = cpu->X >> 1;
      cpu->FR = (bus == 0) | ((bus >> 15) << 1) | ((cpu->X & 1) << 2);
    } else {
      uint16_t result = cpu->X;
      if (mc & (1 << _SUB)) {
        result ^= 0xFFFF;
      }
      if (mc & (1 << Ci)) {
        ++result;
      }
      bus = result + cpu->Y;
      cpu->FR = (bus == 0) | ((bus >> 15) << 1) | ((((uint32_t)result + cpu->Y) >> 16) << 2);
    }
  }
  if (mc & (1 << SPo)) {
    bus = cpu->SP;
  }
  if (mc & (1 << SPu)) {
    cpu->SP += mc & (1 << SPm) ? -2 : 2;
  }
  if (mc & (1 << NDXo)) {
    bus = cpu->NDX;
  }
  if (mc & (1 << MEMo)) {
    bus = cpu->MEM[cpu->NDX | (cpu->SEC << 8)];
  }
  if (mc & (1 << SECo)) {
    bus = cpu->SEC;
  }
  if (mc & (1 << KEYo)) {
    // if (!cpu->KEY_FIFO[0]) { cpu->KEY_FIFO[0] = getchar(); }
    bus = cpu->KEY_FIFO[0];
    memcpy(cpu->KEY_FIFO, cpu->KEY_FIFO + 1, sizeof(cpu->KEY_FIFO) - sizeof(cpu->KEY_FIFO[0]));
  }
  if (mc & (1 << IPi)) {
    cpu->IP = bus;
  }
  if (mc & (1 << IPp)) {
    ++cpu->IP;
  }
  if (mc & (1 << Ai)) {
    cpu->A = bus;
  }
  if (mc & (1 << AHi)) {
    cpu->A = (bus << 8) | (cpu->A & 0xFF);
  }
  if (mc & (1 << MARi)) {
    cpu->MAR = bus;
  }
  if (mc & (1 << Bi)) {
    cpu->B = bus;
  }
  if ((mc & (1 << RAM)) && !(mc & (1 << RAMo))) {
    if (mc & (1 << RAM16)) {
      cpu->RAM[((cpu->MAR / 2) * 2) + 1] = bus >> 8;
      cpu->RAM[(cpu->MAR / 2) * 2] = bus & 0xFF;
    } else {
      cpu->RAM[cpu->MAR] = bus;
    }
  }
  if (mc & (1 << Xi)) {
    cpu->X = bus;
  }
  if (mc & (1 << Yi)) {
    cpu->Y = bus;
  }
  if (mc & (1 << SPi)) {
    cpu->SP = bus;
  }
  if (mc & (1 << IRi)) {
    cpu->IR = bus & 0xFF;
  }
  if (mc & (1 << SCr)) {
    cpu->SC = 0;
    return;
  }
  if (mc & (1 << SECi)) {
    cpu->SEC = bus & ((1 << 12) - 1);
  }
  if (mc & (1 << NDXi)) {
    cpu->NDX = bus;
  }
  if (mc & (1 << MEMi)) {
    assert(0);
  }

  cpu->SC = (cpu->SC + 1) % 16;
}

void load_input_string(cpu_t *cpu, char *string) {
  assert(cpu);
  assert(string);

  uint8_t table[256] = {0};
  uint8_t letters[] = {0x1c, 0x32, 0x21, 0x23, 0x24, 0x2b, 0x34, 0x33, 0x43, 0x3b, 0x42, 0x4b, 0x3a,
                       0x31, 0x44, 0x4d, 0x15, 0x2d, 0x1b, 0x2c, 0x3c, 0x2a, 0x1d, 0x22, 0x35, 0x1a};
  memcpy(table + (int)'a', letters, sizeof(letters));
  memcpy(table + (int)'A', letters, sizeof(letters));
  uint8_t digits[] = {0x45, 0x16, 0x1e, 0x26, 0x25, 0x2e, 0x36, 0x3d, 0x3e, 0x46, 0x79};
  memcpy(table + (int)'0', digits, sizeof(digits));

  table[' '] = 0x29;
  table['-'] = 0x7b;

  int i = 0;
  while (*string) {
    // clang-format off
    switch (*string) {
      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z': case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': case ' ': case '-':
        assert(i + 3 < 1000);
        cpu->KEY_FIFO[i++] = table[(int)*string];
        cpu->KEY_FIFO[i++] = 0xf0;
        cpu->KEY_FIFO[i++] = table[(int)*string];
        break;
      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        assert(i + 6 < 1000);
        cpu->KEY_FIFO[i++] = 0x12;
        cpu->KEY_FIFO[i++] = table[(int)*string];
        cpu->KEY_FIFO[i++] = 0xf0;
        cpu->KEY_FIFO[i++] = table[(int)*string];
        cpu->KEY_FIFO[i++] = 0xf0;
        cpu->KEY_FIFO[i++] = 0x12;
        break;
      case '\n':
        assert(i + 5 < 1000);
        cpu->KEY_FIFO[i++] = 0xe0;
        cpu->KEY_FIFO[i++] = 0x5a;
        cpu->KEY_FIFO[i++] = 0xe0;
        cpu->KEY_FIFO[i++] = 0xf0;
        cpu->KEY_FIFO[i++] = 0x5a;
        break;
      default:
        printf("load_input_string not implemented for char: '%c'\n", *string);
        exit(1);
    }
    // clang-format on
    ++string;
  }
}

int main(int argc, char **argv) {
  int step_mode = 0;
  int real_time_mode = 0;

  struct argparse_option options[] = {
    OPT_GROUP("Options"),
    OPT_HELP(),
    OPT_BOOLEAN('s', "step", &step_mode, "enable step mode after the cpu is HLTed", NULL, 0, 0),
    OPT_BOOLEAN('r', "realtime", &real_time_mode, "sleeps each tick to simulate a 4MHz clock", NULL, 0, 0),
    OPT_END(),
  };
  struct argparse argparse;
  argparse_init(&argparse,
                options,
                (const char *const[]){
                  "sim [options]",
                  NULL,
                },
                0);
  argparse_parse(&argparse, argc, (const char **)argv);

  cpu_t cpu = {0};
  cpu_init(&cpu);
  bool running = true;

  load_input_string(&cpu, "diocane");

  int ticks = 0;
  while (running) {
    tick(&cpu, &running);
    ++ticks;
    if (real_time_mode) {
      sleep(1.0 / 4.0E6);
    }
  }
  printf("ticks: %.3fE3 (%.3f ms @ 4 MHz, %.3f ms @ 10 MHz)\n",
         (float)ticks / 1E3,
         (float)ticks * 1E3 / 4E6,
         (float)ticks * 1E3 / 10E6);
  if (step_mode) {
    char input[100] = {0};
    cpu.SC = 0;
    do {
      if (strcmp(input, "end\n") == 0) {
        break;
      }
      // printf("\e[1;1H\e[2J");
      printf("STEP MODE (enter `end` to quit)\n");
      cpu_dump(&cpu);

      microcode_t mc = 0;
      do {
        tick(&cpu, &running);
        mc = control_rom[cpu.IR | (cpu.SC << 6) | (cpu.FR << (6 + 4))];
      } while (!(mc & (1 << SCr)));
    } while (fgets(input, 100, stdin));
  }

  // printf("\e[1;1H\e[2J");
  cpu_dump(&cpu);
  cpu_dump_ram_range(&cpu, 0x0800, 0x0820);

  cpu_dump_stdout(&cpu);

  return 0;
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

  set_instruction_allflag(KEY_A, 2, micro(KEYo) | micro(Ai));
  set_instruction_allflag(KEY_A, 3, micro(SCr));

  set_instruction_allflag(HLT, 2, micro(_HLT));
}
