#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define KEY_FIFO_COUNT 1024

#define SV_IMPLEMENTATION
#include "errors.h"
#include "files.h"
#include "instructions.h"

// #define SCREEN_WIDTH  800
// #define SCREEN_HEIGHT 60
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 350
#define ZOOM          2
#define SCREEN_PAD    10

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

  uint8_t KEY_FIFO[KEY_FIFO_COUNT];
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
    eprintf("cannot open file: 'mem.bin': %s\n", strerror(errno));
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

void dumpsbit(uint32_t num, int bits) {
  putchar('0');
  putchar('b');
  for (int i = 0; i < bits; ++i) {
    putchar((num >> (bits - 1 - i)) & 1 ? '1' : '0');
  }
}

void compute_screen(cpu_t *cpu) {
  assert(cpu);

  BeginTextureMode(cpu->screen);
  ClearBackground(WHITE);
  for (int y = 0; y < SCREEN_HEIGHT; ++y) {
    for (int x = 0; x < SCREEN_WIDTH; ++x) {
      uint16_t attribute = cpu->ATTRIBUTE_RAM[((y / 8) << 8) + (x / 8)];
      uint16_t pattern_index = attribute & 0x0FFF;
      uint8_t palette_index = (attribute >> 12) & 0xF;
      int dy = y & 0b111;
      int dx = x & 0b111;
      uint8_t color = (cpu->PATTERN_RAM[pattern_index * 8 + dy] >> (7 - dx)) & 1;
      DrawPixel(x, SCREEN_HEIGHT - y - 1, color ? WHITE : BLACK);
    }
  }

  EndTextureMode();
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
        DrawTextureEx(cpu->screen.texture, (Vector2){SCREEN_PAD, SCREEN_PAD}, 0, ZOOM, WHITE);
        EndDrawing();
      }
    }
  }

  cpu->SC = (cpu->SC + 1) % 16;
}

// look at https://www.vetra.com/scancodes.html
// based on https://wiki.osdev.org/PS/2_Keyboard scan code set 1
static char scancodeset[256] = {
    0,
    0 /*escape*/,
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '0',
    '-',
    '=',
    0 /*backspace*/,
    '\t',
    'q',
    'w',
    'e',
    'r',
    't',
    'y',
    'u',
    'i',
    'o',
    'p',
    '[',
    ']',
    '\n',
    0 /*left ctrl*/,
    'a',
    's',
    'd',
    'f',
    'g',
    'h',
    'j',
    'k',
    'l',
    ';',
    '\'',
    '`',
    0 /*left shift*/,
    0 /*backslash*/,
    'z',
    'x',
    'c',
    'v',
    'b',
    'n',
    'm',
    ',',
    '.',
    '/',
    0 /*right shift*/,
    0 /*keypad * */,
    0 /*left alt*/,
    ' ',
};

uint8_t find_scan_code(char c) {
  uint8_t code = 0;
  for (int i = 0; i < 256; i++) {
    if (scancodeset[i] == c) {
      code = i;
      break;
    }
  }
  if (code == 0) {
    eprintf("no scancode for '%c'\n", c);
  }
  return code;
}

void load_input_string(cpu_t *cpu, char *string) {
  assert(cpu);
  assert(string);

  for (; *string; ++string) {
    if ('A' <= *string && *string <= 'Z') {
      uint8_t code = find_scan_code(*string);
      assert(code < 0x80);
      assert(cpu->key_fifo_i + 4 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x2A; // left shift pressed
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x2A + 0x80; // left shift released
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code + 0x80;
    } else if (*string == '\\' && *(string + 1) == 'n') {
      ++string;
      assert(cpu->key_fifo_i + 2 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x1C;

      cpu->KEY_FIFO[cpu->key_fifo_i++] = 0x1C + 0x80;
    } else {
      uint8_t code = find_scan_code(*string);
      assert(code < 0x80);
      assert(cpu->key_fifo_i + 2 < 1000);
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code;
      cpu->KEY_FIFO[cpu->key_fifo_i++] = code + 0x80;
    }
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
  set_instruction_allflag(DECB, 2, micro(Bo) | micro(Yi));
  set_instruction_allflag(DECB, 3, micro(Xi));
  set_instruction_allflag(DECB, 4, micro(_SUB) | micro(ALUo) | micro(Bi));
  set_instruction_allflag(DECB, 5, micro(SCr));
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

symbol_t *find_sybol(symbol_t *symbols, int count, char *name) {
  assert(symbols);
  assert(name);

  for (int i = 0; i < count; ++i) {
    if (strcmp(symbols[i].image, name) == 0) {
      return &symbols[i];
    }
  }
  return NULL;
}

typedef struct {
  cpu_t cpu;
  int16_t test_ram[1 << 16]; // negative if undef
} test_t;

void test_init(test_t *test) {
  assert(test);
  *test = (test_t){0};
  cpu_init(&test->cpu, "mem.bin");
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

void test_set_range(test_t *test, int ram_start, int count, uint8_t *data) {
  assert(test);
  assert(data);
  assert(count);
  for (int i = 0; i < count; ++i) {
    test->test_ram[i + ram_start] = data[i];
  }
}

void test_unset_range(test_t *test, int start, int count) {
  assert(test);
  assert(count);
  memset(&test->test_ram[start], -1, count * sizeof(test->test_ram[0]));
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

void test() {
  test_t test_ = {0};
  test_t *test = &test_;
  test_init(test);
  cpu_t *cpu = &test->cpu;
  bool running = true;

  printf("TEST:\n");
  printf("\tRun bootloader\n");

  while (running && !(cpu->IR == JMPA && cpu->A == 0)) {
    tick(cpu, &running);
  }
  test_assert(running);

  printf("\tCheck if os is loaded\n");

  exe_t os = exe_decode_file("mem/__os");
  exe_reloc(&os, 0, os.code_size);

  test_set_range(test, 0, os.code_size, os.code);
  test_check(test);

  {
    symbol_t *file = find_sybol(os.symbols, os.symbol_count, "file");
    test_assert(file);
    test_unset_range(test, file->pos, 4);
  }

  printf("\tCheck if stdlib is loaded\n");

  so_t stdlib = so_decode_file("mem/__stdlib");
  for (int i = 0; i < stdlib.reloc_count; ++i) {
    reloc_entry_t entry = stdlib.relocs[i];
    entry.what += os.code_size;
    stdlib.code[entry.where] = entry.what & 0xFF;
    stdlib.code[entry.where + 1] = (entry.what >> 8) & 0xFF;
  }

  // TODO: rewrite with find_symbol
  uint16_t execute_ptr = 0;
  uint16_t open_file_ptr = 0;
  uint16_t exit_ptr = 0;
  for (int i = 0; i < stdlib.global_count; ++i) {
    if (strcmp(stdlib.symbols[stdlib.globals[i]].image, "execute") == 0) {
      execute_ptr = stdlib.symbols[stdlib.globals[i]].pos + os.code_size;
    } else if (strcmp(stdlib.symbols[stdlib.globals[i]].image, "open_file") == 0) {
      open_file_ptr = stdlib.symbols[stdlib.globals[i]].pos + os.code_size;
    } else if (strcmp(stdlib.symbols[stdlib.globals[i]].image, "exit") == 0) {
      exit_ptr = stdlib.symbols[stdlib.globals[i]].pos + os.code_size;
    }
  }
  assert(execute_ptr != 0);
  assert(open_file_ptr != 0);
  assert(exit_ptr != 0);

  test_set_range(test, os.code_size, stdlib.code_size, stdlib.code);
  test_check(test);

  {
    symbol_t *file = find_sybol(stdlib.symbols, stdlib.symbol_count, "file");
    test_assert(file);
    test_unset_range(test, file->pos + os.code_size, 4);
  }

  printf("\tRun os till CALL execute sh\n");

  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_ptr)) {
    tick(cpu, &running);
  }
  test_assert(running);

  test_assert(cpu->RAM[cpu->A] == 's');
  test_assert(cpu->RAM[cpu->A + 1] == 'h');
  test_assert(cpu->RAM[cpu->A + 2] == 0);

  printf("\tCheck os struct, process and stdout\n");

  uint16_t stdout_pos = 0xFFFF;
  {
    symbol_t *stdout_symbol = find_sybol(os.symbols, os.symbol_count, "stdout");
    test_assert(stdout_symbol);
    stdout_pos = stdout_symbol->pos;
  }
  test_assert(stdout_pos);
  test_set_u16(test, stdout_pos, 128);
  test_set_u16(test, stdout_pos + 2, stdout_pos + 4);

  // os struct:
  test_set_u16(test, 0xF800, os.code_size); // stdlib
  test_set_u16(test, 0xF802, 0xF820);       // current process
  test_set_u16(test, 0xF804, 0x8000);       // used process map
  test_set_u16(test, 0xF806, 0x8000);       // used page map
  test_set_u16(test, 0xF808, 0);            // used page map
  test_set_u16(test, 0xF80A, 0);            // screen text row - col
  //  os process:
  test_set_u16(test, 0xF820, 0xFFFF);     // parent process
  test_set_u16(test, 0xF822, 1);          // cwd sec
  test_set_u16(test, 0xF824, 0xFFFE);     //  SP
  test_set_u16(test, 0xF826, stdout_pos); // stdout redirect

  test_check(test);

  test_unset_range(test, 0xF824, 2); // os.SP

  printf("\tRun execute sh\n");

  while (running && cpu->IR != JMPA) {
    tick(cpu, &running);
  }
  test_assert(running);

  printf("\tCheck if sh is loaded and processes\n");

  exe_t sh = exe_decode_file("mem/sh");
  exe_reloc(&sh, 2048, os.code_size);

  test_set_range(test, 2048, sh.code_size, sh.code);

  test_set_u16(test, 0xF802, 0xF830); // current process
  test_set_u16(test, 0xF804, 0xC000); // used process map
  test_set_u16(test, 0xF806, 0xC000); // used page map

  // sh process:
  test_set_u16(test, 0xF830, 0xF820);     // parent process
  test_set_u16(test, 0xF832, 1);          // cwd
  test_set_u16(test, 0xF834, 0);          // SP
  test_set_u16(test, 0xF836, stdout_pos); // stdout redirect

  test_check(test);

  test_unset_range(test, 0xF80A, 2); // screen text row and col
  test_unset_range(test, 0xF834, 2); // process.SP

  printf("\tInput 'ls'\n");

  symbol_t *sh_input_s = find_sybol(sh.symbols, sh.symbol_count, "input");
  test_assert(sh_input_s);
  uint16_t sh_input = sh_input_s->pos + 2048;

  load_input_string(cpu, "ls\n");
  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_ptr)) {
    tick(cpu, &running);
  }
  test_assert(running);

  test_set_u16(test, stdout_pos, 128 - 2 - 3);
  test_set_u16(test, stdout_pos + 2, stdout_pos + 4 + 5);
  test_unset_range(test, stdout_pos + 4, stdout_pos + 128 - 4); // TODO: test stdout
  test_set_range(test, sh_input, 3, (uint8_t *)"ls\0");
  test_check(test);

  test_unset_range(test, stdout_pos, stdout_pos + 128); // TODO: test stdout

  printf("\tRun ls command till exit\n");

  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_ptr)) {
    tick(cpu, &running);
  }
  test_assert(running);
  while (running && cpu->RAM[cpu->IP] != RET) {
    tick(cpu, &running);
  }
  test_assert(running);

  test_check(test);

  printf("\tRun `cd dir`\n");

  load_input_string(cpu, "cd dir\n");
  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_ptr)) {
    tick(cpu, &running);
  }
  test_assert(running);
  while (running && cpu->RAM[cpu->IP] != RET) {
    tick(cpu, &running);
  }
  test_assert(running);

  uint16_t dir_sec = 0;
  // TODO: put in decode_dir ... in file
  {
    FILE *mem = fopen("mem.bin", "rb");
    test_assert(mem);
    test_assert(fseek(mem, 256, SEEK_SET) == 0);
    test_assert(fgetc(mem) == 'D');
    test_assert(fgetc(mem) == 0xFF);
    test_assert(fgetc(mem) == 0xFF);
    for (int i = 3; i < 256;) {
      uint16_t sec = 0;
      char name[256] = {0};
      for (int j = 0; j < 256; ++j) {
        name[j] = fgetc(mem);
        test_assert(name[j] != -1);
        if (name[j] == 0) {
          break;
        }
      }
      test_assert(fread(&sec, 2, 1, mem) == 1);
      test_assert(sec != 0);
      if (strcmp(name, "dir") == 0) {
        dir_sec = sec;
        break;
      }
      i += strlen(name) + 3;
    }
    fclose(mem);
  }
  test_assert(dir_sec != 0);

  test_set_range(test, sh_input, 7, (uint8_t *)"cd\0dir\0");
  test_set_u16(test, 0xF832, dir_sec);
  test_check(test);

  test_unset_range(test, sh_input, 128);

  printf("\tRun `/ls`\n");

  load_input_string(cpu, "/ls\n");
  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_ptr)) {
    tick(cpu, &running);
  }
  test_assert(running);
  while (running && cpu->RAM[cpu->IP] != RET) {
    tick(cpu, &running);
  }
  test_assert(running);

  // test success
  test_check(test);

  printf("End\n");
}

void help(int exitcode) {
  printf("Usage: sim [options]\n\n"
         "Options:\n"
         " -i | --input <string>            input <string> to computer [max %d char]\n"
         " -r | --ram-range <start>:<end>   print ram from <start> to <end> when HLTed (example "
         "2:0xFA)\n"
         " -t | --test                      run test and exit\n"
         " -s | --step                      enable step mode after the cpu is HLTed\n"
         "      --real-time                 sleeps each tick to simulate a 4MHz clock\n"
         "      --mem <mem-path>            set the binary file for MEM [default mem.bin]\n"
         "      --screen                    enable screen\n"
         "      --stdout <start>:<end>      print ram from <start> to <end> as a stdout struct\n"
         " -h | --help                      print this page and exit\n",
         KEY_FIFO_COUNT);
  exit(exitcode);
}

void parse_range(char *range, int *start_out, int *end_out) {
  assert(range);
  assert(start_out);
  assert(end_out);

  char *c = strchr(range, ':');
  if (!c) {
    printf("ERROR: range expects a ':'\n");
    help(1);
  }
  if (c - range == 0) {
    printf("ERROR: range expects a start number\n");
    help(1);
  }
  if (*(c + 1) == 0) {
    printf("ERROR: range expects an end number\n");
    help(1);
  }
  *c = 0;
  if (range[0] == '0' && (range[1] == 'x' || range[1] == 'X')) {
    *start_out = strtol(range + 2, NULL, 16);
  } else {
    *start_out = atoi(range);
  }
  if (c[1] == '0' && (c[2] == 'x' || c[2] == 'X')) {
    *end_out = strtol(c + 3, NULL, 16);
  } else {
    *end_out = atoi(c + 1);
  }
  if (*start_out > *end_out) {
    printf("ERROR: range start bigger than end\n");
    exit(1);
  }
}

int main(int argc, char **argv) {
  (void)argc;

  set_control_rom();

  int step_mode = 0;
  int real_time_mode = 0;
  char input[KEY_FIFO_COUNT] = {0};
  int inputi = 0;
  int ram_range_start = 0;
  int ram_range_end = 0;
  char *mem_path = "mem.bin";
  int screen = 0;
  int stdout_pos = 0;

  ++argv;
  char *arg = *argv;
  while (*argv) {
    arg = *argv;
    if (arg[0] == '-' && arg[1] == '-') {
      if (strcmp(arg + 2, "help") == 0) {
        help(0);
      } else if (strcmp(arg + 2, "step") == 0) {
        step_mode = 1;
      } else if (strcmp(arg + 2, "real-time") == 0) {
        real_time_mode = 1;
      } else if (strcmp(arg + 2, "input") == 0) {
        arg[1] = 'i';
        arg[2] = 0;
        continue;
      } else if (strcmp(arg + 2, "ram-range") == 0) {
        arg[1] = 'r';
        arg[2] = 0;
        continue;
      } else if (strcmp(arg + 2, "stdout") == 0) {
        ++argv;
        if (*argv == NULL) {
          printf("ERROR: --stdout expects an int\n");
          help(1);
        }
        stdout_pos = atoi(*argv);
      } else if (strcmp(arg + 2, "mem") == 0) {
        ++argv;
        if (*argv == NULL) {
          printf("ERROR: --mem expects a string\n");
          help(1);
        }
        mem_path = *argv;
      } else if (strcmp(arg + 2, "screen") == 0) {
        screen = 1;
      } else if (strcmp(arg + 2, "test") == 0) {
        test();
        exit(0);
      } else {
        printf("unknown arg '%s'\n", arg);
        help(1);
      }
    } else if (arg[0] == '-' && arg[2] == 0) {
      switch (arg[1]) {
        case 's':
          step_mode = 1;
          break;
        case 'h':
          help(0);
          break;
        case 't':
          test();
          exit(0);
          break;
        case 'i':
        {
          ++argv;
          if (*argv == NULL) {
            printf("ERROR: --input expects a string\n");
            help(1);
          }
          int len = strlen(*argv);
          if (len + inputi > KEY_FIFO_COUNT) {
            printf("ERROR: input buffer full\n");
            exit(1);
          }
          strcpy(input + inputi, *argv);
          inputi += len;
        } break;
        case 'r':
          ++argv;
          if (*argv == NULL) {
            printf("ERROR: --ram-range expects a range\n");
            help(1);
          }
          parse_range(*argv, &ram_range_start, &ram_range_end);
          break;
        default:
          printf("unknown arg '%s'\n", arg);
          help(1);
      }
    } else {
      printf("ERROR: unknown arg '%s'\n", arg);
      help(1);
    }
    ++argv;
  }

  cpu_t cpu = {0};
  cpu_init(&cpu, mem_path);
  cpu.has_screen = screen;

  load_input_string(&cpu, input);

  if (cpu.has_screen) {
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(SCREEN_WIDTH * ZOOM + SCREEN_PAD * 2, SCREEN_HEIGHT * ZOOM + SCREEN_PAD * 2, "Jaris screen");
    cpu.screen = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);
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
    if (real_time_mode) {
      sleep(1.0 / 4.0E6);
    }
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

  if (step_mode) {
    char input[100] = {0};
    cpu.SC = 0;
    printf("STEP MODE\n");
    cpu_dump(&cpu);
    printf("> ");
    while (fgets(input, 100, stdin)) {
      cpu_dump(&cpu);

      microcode_t mc = 0;
      if (strcmp(input, "end\n") == 0 || strcmp(input, "quit\n") == 0
          || strcmp(input, "exit\n") == 0) {
        break;
      } else if (strcmp(input, "help\n") == 0 || strcmp(input, "?\n") == 0) {
        printf("commands:\n"
               "  quit | end | exit    quit step mode\n"
               "  help | ?             print commands avaiable\n"
               "  stdout               print the stdout\n"
               "  skip                 skip to the RET inst\n"
               "  next                 run to the next HLT\n");
      } else if (strcmp(input, "skip\n") == 0) {
        running = true;
        do {
          printf("-\n");
          tick(&cpu, &running);
          if (cpu.IR == RET) {
            break;
          }
        } while (running);
      } else if (strcmp(input, "next\n") == 0) {
        running = true;
        while (running) {
          tick(&cpu, &running);
        }
        cpu.SC = 0;
      }

      running = true;
      do {
        tick(&cpu, &running);
        mc = control_rom[cpu.IR | (cpu.SC << 6) | (cpu.FR << (6 + 4))];
      } while (running && !(mc & (1 << SCr)));

      printf("> ");
    };
  }

  cpu_dump(&cpu);
  if (ram_range_end != ram_range_start) {
    cpu_dump_ram_range(&cpu, ram_range_start, ram_range_end);
  }

  cpu_dump_stdout(&cpu, stdout_pos);

  return 0;
}
