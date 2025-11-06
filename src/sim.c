#include <assert.h>
#include <time.h>

#include "../argparse.h"
#include "../mystb/errors.h"
#include "files.h"
#include "runtime.h"

#define TEST_MEM_PATH "main.mem.bin" // TODO: make test mem

void test() {
  test_t test_ = {0};
  test_t *test = &test_;
  test_init(test, TEST_MEM_PATH);
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
  test_assert_running(test_);

  printf("  OS LOADED\n");
  exe_t os = exe_decode_file("asm/bin/os");
  exe_reloc(&os, 0, os.code_size);
  test_set_range(test, 0, os.code_size, os.code);

  {
    uint16_t font_file_ptr = test_find_symbol(os.symbols, os.symbol_count, "file");
    test_unset_range(test, font_file_ptr, 4);
  }

  printf("  STDLIB LOADED\n");
  uint16_t stdlib_pos = os.code_size;
  so_t stdlib = so_decode_file("asm/bin/stdlib");
  for (int i = 0; i < stdlib.reloc_count; ++i) {
    reloc_entry_t entry = stdlib.relocs[i];
    entry.what += stdlib_pos;
    stdlib.code[entry.where] = entry.what & 0xFF;
    stdlib.code[entry.where + 1] = (entry.what >> 8) & 0xFF;
  }
  test_set_range(test, stdlib_pos, stdlib.code_size, stdlib.code);

  test_set_u16(test, 0xF800, stdlib_pos);
  test_unset_range(test, 0xFE00, 256); // global stack
  test_check(test);

  uint16_t execute_pos = test_find_symbol(stdlib.symbols, stdlib.symbol_count, "execute") + stdlib_pos;
  uint16_t exit_pos = test_find_symbol(stdlib.symbols, stdlib.symbol_count, "exit") + stdlib_pos;

  {
    uint16_t file_ptr = test_find_symbol(stdlib.symbols, stdlib.symbol_count, "file") + stdlib_pos;
    test_unset_range(test, file_ptr, 4);
    uint16_t print_pos_ptr = test_find_symbol(stdlib.symbols, stdlib.symbol_count, "pos_ptr") + stdlib_pos;
    test_unset_range(test, print_pos_ptr, 2); // TODO:
  }

  printf("  OS SETUP\n");
  while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos)) {
    tick(cpu, &running);
  }
  // test_assert_running(test_);
  // test_assert(cpu->RAM[cpu->A] == 's');
  // test_assert(cpu->RAM[cpu->A + 1] == 'h');
  // test_assert(cpu->RAM[cpu->A + 2] == 0);

  // os struct:
  test_set_u16(test, 0xF800, stdlib_pos); // stdlib
  test_set_u16(test, 0xF802, 0xF820);     // current process
  test_set_u16(test, 0xF804, 1 << 15);    // used process map
  test_set_u16(test, 0xF806, 0b11 << 14); // used page map
  test_set_u16(test, 0xF808, 1);          // used page map
  //  os process:
  test_set_u16(test, 0xF820, 0xFFFF); // parent process
  test_set_u16(test, 0xF822, 1);      // cwd sec
  test_set_u16(test, 0xF824, 0xFFFE); //  SP
  test_set_u16(test, 0xF826, 0xFFFF); // stdout redirect
  test_set_u16(test, 0xF828, 0xFFFF); // stdin redirect
  test_check(test);
  test_unset_range(test, 0xF824, 2); // os SP

  printf("  LOAD AND EXECUTE SH\n");
  while (running && cpu->IR != JMPA) {
    tick(cpu, &running);
  }
  test_assert_running(test_);

  uint16_t sh_pos = 2 * PAGE_SIZE;

  exe_t sh = exe_decode_file("asm/bin/sh");
  exe_reloc(&sh, sh_pos, stdlib_pos);
  test_unset_range(test, sh_pos, PAGE_SIZE); // unset sh stack and code then set code
  test_set_range(test, sh_pos, sh.code_size, sh.code);
  // os struct:
  test_set_u16(test, 0xF802, 0xF830);
  test_set_u16(test, 0xF804, 0b11 << 14);
  test_set_u16(test, 0xF806, 0b111 << 13);
  // sh process:
  test_set_u16(test, 0xF830, 0xF820);
  test_set_u16(test, 0xF832, 1);
  test_set_u16(test, 0xF834, 0);
  test_set_u16(test, 0xF836, 0xFFFF);
  test_set_u16(test, 0xF838, 0xFFFF);
  test_check(test);
  test_unset_range(test, 0xF834, 2); // sh SP

  uint16_t sh_input_pos = test_find_symbol(sh.symbols, sh.symbol_count, "input") + sh_pos;

  test_run_command(test, "test_font", "", "asm/bin/test_font", sh_input_pos, execute_pos, exit_pos);
  test_gpu_print(test, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\nabcdefghijklmnopqrstuvwxyz\n0123456789\n!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}\1\n");
  test_check(test);

  test_run_command(test, "echo ab cd ef", "", "asm/bin/echo", sh_input_pos, execute_pos, exit_pos);
  test_gpu_print(test, "ab cd ef\n");
  test_check(test);

  test_run_command(test, "cd dir", "", "asm/bin/cd", sh_input_pos, execute_pos, exit_pos);
  uint16_t dir_sec = mem_sector_find_entry(cpu->MEM + 256, "dir");
  test_set_u16(test, 0xF832, dir_sec);
  test_check(test);

  test_set_u16(test, 0xF842, dir_sec);
  test_run_command(test, "/ls", "", "asm/bin/ls", sh_input_pos, execute_pos, exit_pos);
  test_gpu_print(test, "a a.sk \n");
  test_check(test);

  test_run_command(test, "/cat a", "", "asm/bin/cat", sh_input_pos, execute_pos, exit_pos);
  {
    exe_t cat = exe_decode_file("asm/bin/cat");
    exe_reloc(&cat, 3 * PAGE_SIZE, stdlib_pos);
    uint16_t file_pos = test_find_symbol(cat.symbols, cat.symbol_count, "file") + 3 * PAGE_SIZE;
    test_unset_range(test, file_pos, 4);
  }
  test_gpu_print(test, "culo\n");
  test_check(test);

  test_run_command(test, "/cd ..", "", "asm/bin/cd", sh_input_pos, execute_pos, exit_pos);
  test_set_u16(test, 0xF832, 1);
  test_check(test);

  // test_run_command(test, "stack dir/a.sk", "", "asm/bin/stack", sh_input_pos, execute_pos, exit_pos);
  // test_unset_range(test, 3 * PAGE_SIZE + 3, 4 + 4 + 32 + 2);
  // test_check(test);
  //
  // {
  //   FILE *file = fmemopen(cpu->MEM + 256 * 22 + 4, cpu->MEM[256 * 22 + 3], "r");
  //   assert(file);
  //   exe_t exe = exe_decode(file);
  //   test_assert(exe.reloc_count == 0);
  //   test_assert(exe.dynamic_count == 1);
  //   test_assert(exe.dynamics[0].file_name[0] = 1);
  //   test_assert(exe.symbol_count == 0);
  //   exe_dump(&exe);
  //   assert(fclose(file) == 0);
  // }
  //
  // printf("  LOAD `stack.out`\n");
  // load_input_string(cpu, "stack.out\n");
  // test_gpu_print(test, "$ stack.out\n");
  // test_set_range(test, sh_input_pos, 10, (uint8_t *)"stack.out\0");
  // while (running && !(cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos)) {
  //   tick(cpu, &running);
  // }
  // test_assert_running(test_);
  // while (running && cpu->IR != JMPA) {
  //   tick(cpu, &running);
  // }
  // cpu_dump(cpu);
  // test_assert_running(test_);
  // test_check(test);

  // test_run_command(test, "tee file", "hi\n", "asm/bin/tee", sh_input_pos, execute_pos, exit_pos);
  //{
  //   exe_t tee = exe_decode_file("asm/bin/tee");
  //   exe_reloc(&tee, 3 * PAGE_SIZE, stdlib_pos);
  //   uint16_t file_pos = test_find_symbol(tee.symbols, tee.symbol_count, "file") + 3 * PAGE_SIZE;
  //   test_unset_range(test, file_pos, 4);
  // }
  // test_check(test);

  printf("TODO bfjit?\n");
  printf("TODO expr?\n");
  printf("TODO ERRORS\n");

  printf("  RUN `shutdown`\n");
  load_input_string(cpu, "shutdown\n");
  while (running) {
    tick(cpu, &running);
  }
  test_assert(cpu->A == 0xA0A0);
  test_assert(!running);

  printf("END\n\n");
}

void test_rule110() {
  test_t test = {0};
  test_init(&test, TEST_MEM_PATH);
  cpu_t *cpu = &test.cpu;

  printf("TEST rule110\n");
  printf("  TODO\n");
  return;

  exe_t os = exe_decode_file("asm/bin/os");
  exe_reloc(&os, 0, os.code_size);
  so_t stdlib = so_decode_file("asm/bin/stdlib");
  so_reloc(&stdlib, os.code_size);
  uint16_t execute_pos = test_find_symbol(stdlib.symbols, stdlib.symbol_count, "execute") + os.code_size;
  test_run_until(test, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);
  test_run_until(test, cpu->RAM[cpu->IP] == JMPA);
  test_assert(test.running);
  test_unset_range(&test, 0, PAGE_SIZE * 3);
  test_unset_range(&test, 0xF800, PAGE_SIZE);
  test_check(&test);

  printf("  LOAD\n");
  load_input_string(cpu, "rule110\n");
  test_run_until(test, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);
  test_run_until(test, cpu->IR == JMPA);
  test_step(test);
  test_assert(test.running);

  exe_t rule110 = exe_decode_file("asm/bin/rule110");
  exe_reloc(&rule110, 3 * PAGE_SIZE, os.code_size);
  test_set_range(&test, 3 * PAGE_SIZE, rule110.code_size, rule110.code);
  test_gpu_print(&test, "$ rule110\n");
  test_unset_range(&test, 3 * PAGE_SIZE + rule110.code_size, PAGE_SIZE - rule110.code_size); // stack
  test_check(&test);

  uint16_t iter_pos = test_find_symbol(rule110.symbols, rule110.symbol_count, "iter") + 3 * PAGE_SIZE;

  test_run_until(test, cpu->IP == iter_pos + 1);
  test_assert(test.running);
  test_step(test);
  test_run_until(test, cpu->IP == iter_pos + 1);
  test_assert(test.running);
  test_step(test);
  test_check(&test);
}

void print_help() {
  printf("Usage: sim [options]\n\n"
         "Options:\n"
         "  -i | --input <str>  simulate input for the computer\n"
         "  -s | --step         run in step mode\n"
         "  -t | --test         run the test\n"
         "  -S | --screen       enable screen\n"
         "  -r <from>:<to>      print ram in the range\n"
         "  --mem <path>        specify bin file path for memory\n"
         "  --stdout <num>      print stdout struct in ram from num\n"
         "  --real-time         simulate with ticks at %d MHz\n"
         "  -h | --help         show help message\n",
         CLOCK_FREQ);
}

int main(int argc, char **argv) {
  set_control_rom();

  char *input = "";
  char *mempath = "main.mem.bin";
  int enable_step_mode = 0;
  int screen = 0;
  int real_time = 0;
  int stdout_start = -1;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                             //
        else ARG_PARSE_FLAG("s", "step", enable_step_mode)         //
        else ARG_PARSE_FLAG_(ARG_LFLAG("real-time"), real_time)    //
        else ARG_PARSE_FLAG("S", "screen", screen)                 //
        else ARG_PARSE_STRING_ARG("i", "input", input)             //
        else ARG_PARSE_STRING_ARG_(ARG_LFLAG("mem"), mempath)      //
        else ARG_PARSE_INT_ARG_(ARG_LFLAG("stdout"), stdout_start) //
        else ARG_IF_FLAG("t", "test") {
      test();
      test_rule110();
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

  cpu_t cpu = {0};
  cpu_init(&cpu, mempath);
  cpu.has_screen = screen;

  load_input_string(&cpu, input);

  if (cpu.has_screen) {
    SetTraceLogLevel(LOG_ERROR);
    InitWindow(SCREEN_WIDTH * SCREEN_ZOOM + SCREEN_PAD * 2, SCREEN_HEIGHT * SCREEN_ZOOM + SCREEN_PAD * 2, "Jaris screen");
    cpu.screen = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    BeginTextureMode(cpu.screen);
    ClearBackground(screen_palette[0]);
    EndTextureMode();
  }

  clock_t curr_time, begin_time = clock();
  bool running = true;
  while (running) {
    PollInputEvents();
    if (cpu.has_screen && WindowShouldClose()) {
      break;
    }

    if (real_time) {
      curr_time = clock();
      if ((double)(curr_time - begin_time) / CLOCKS_PER_SEC < 1e-6 / CLOCK_FREQ) {
        continue;
      } else {
        begin_time = curr_time;
      }
    }
    tick(&cpu, &running);
  }
  if (enable_step_mode) {
    step_mode(&cpu);
  }
  if (cpu.has_screen) {
    BeginDrawing();
    ClearBackground(WHITE);
    DrawTextureEx(cpu.screen.texture, (Vector2){SCREEN_PAD, SCREEN_PAD}, 0, SCREEN_ZOOM, WHITE);
    EndDrawing();
    while (!WindowShouldClose()) {
      PollInputEvents();
    }
    UnloadRenderTexture(cpu.screen);
    CloseWindow();
  }

  printf("ticks: %.3fE3 (%.3f ms @ 4 MHz, %.3f ms @ 10 MHz)\n",
         (float)cpu.ticks / 1E3,
         (float)cpu.ticks * 1E3 / 4E6,
         (float)cpu.ticks * 1E3 / 10E6);

  cpu_dump(&cpu);

  if (stdout_start != -1) {
    cpu_dump_stdout(&cpu, stdout_start);
  }

  {
    FILE *file = fopen("sim.mem.out.bin", "wb");
    assert(file);
    assert(fwrite(cpu.MEM, 1, 1 << 19, file) == 1 << 19);
    assert(fclose(file) == 0);
  }

  return 0;
}
