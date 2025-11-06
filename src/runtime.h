#ifndef RUNTIME_H__
#define RUNTIME_H__

#include <raylib.h>
#include <stdint.h>

#include "../mystb/errors.h"
#include "files.h"

#define KEY_FIFO_SIZE 1024
#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 350
#define SCREEN_ZOOM   2
#define SCREEN_PAD    10
#define PAGE_SIZE     2048
#define CLOCK_FREQ    6 // MHz

// clang-format off
typedef enum {
  _HLT,
  IPi, IPo, IPp,
  Ai, AHi, Ao,
  MARi,
  Bi, Bo,
  RAM, RAMo, RAM16,
  Xi, Yi, ALUo, _SUB, _SHR, _AND, Ci,
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

extern Color screen_palette[];

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
  long long ticks;
} cpu_t;

// TODO: test mem
typedef struct {
  bool running;
  cpu_t cpu;
  int16_t test_ram[1 << 16]; // negative if undef
  uint8_t test_gpu[1 << 15];
  uint8_t gpu_x;
  uint8_t gpu_y;

  char stdout[2048];
  int stdout_size;
} test_t;

#define test_step(test__)                         \
  do {                                            \
    tick(&test__.cpu, &test__.running);           \
  } while (test__.running && test__.cpu.SC != 2);

#define test_run_until(test__, cond__)                        \
  while (test__.running && !(test__.cpu.SC == 2 && cond__)) { \
    tick(&test__.cpu, &test__.running);                       \
  }

#define test_assert_running(test__) \
  do {                              \
    if (!((test__).running)) {      \
      cpu_dump(cpu);                \
      eprintf("expected running");  \
    }                               \
  } while (0);

#define test_assert(cond__)               \
  do {                                    \
    if (!(cond__)) {                      \
      eprintf("failed assert: " #cond__); \
    }                                     \
  } while (0);

void cpu_init(cpu_t *cpu, char *mem_path);
uint16_t cpu_read16(cpu_t *cpu, uint16_t at);
void cpu_write16(cpu_t *cpu, uint16_t at, uint16_t what);
void dumpsbit(uint32_t num, int bits);
void cpu_dump(cpu_t *cpu);
void cpu_dump_ram_range(cpu_t *cpu, uint16_t from, uint16_t to);
void cpu_dump_stdout(cpu_t *cpu, uint16_t pos);

void set_control_rom();
void load_input_string(cpu_t *cpu, char *string);
bool check_microcode(microcode_t mc, microcode_flag_t flag);
void tick(cpu_t *cpu, bool *running);

void test_init(test_t *test, char *mem_path);
void test_check(test_t *test);
uint16_t test_find_symbol(symbol_t *symbols, uint16_t count, char *image);
void test_set_range(test_t *test, int ram_start, int count, uint8_t *data);
void test_set_u16(test_t *test, uint16_t at, uint16_t num);
void test_unset_range(test_t *test, uint16_t from, uint16_t count);
void test_gpu_print(test_t *test, char *str);
void test_print_ram_range(test_t *test, uint16_t from, uint16_t to);
void test_set_stdout(test_t *test, uint16_t stdout_pos, char *str);
void exe_reloc(exe_t *exe, uint16_t start_pos, uint16_t stdlib_pos);
void so_reloc(so_t *so, uint16_t start_pos);
void test_run_command(test_t *test, char *command, char *input, char *exe_path, uint16_t sh_input_pos, uint16_t execute_pos, uint16_t exit_pos);

void step_mode(cpu_t *cpu);

#endif // RUNTIME_H__
