#include <assert.h>
#include <stdio.h>
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
  test_run_until(test_, cpu->IR == JMPA && cpu->A == 0);
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
  test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);

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

  {
    printf("  EXECUTE `test_font`\n");
    load_input_string(cpu, "test_font\n");

    test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);
    test_assert_running(test_);
    test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_pos);
    test_assert_running(test_);

    exe_t exe = exe_decode_file("asm/bin/test_font");
    exe_reloc(&exe, 3 * PAGE_SIZE, stdlib_pos);
    test_unset_range(test, 3 * PAGE_SIZE, PAGE_SIZE);
    test_set_range(test, 3 * PAGE_SIZE, exe.code_size, exe.code);
    test_gpu_print(test, "$ test_font\n");
    test_set_range(test, sh_input_pos, 10, (uint8_t *)"test_font\0");

    test_set_u16(test, 0xF802, 0xF840);             // os.current_process
    test_set_u16(test, 0xF804, 0b111 << (16 - 3));  // os.process_map
    test_set_u16(test, 0xF806, 0b1111 << (16 - 4)); // os.page_map
    test_set_u16(test, 0xF840, 0xF830);             // process.parent
    test_set_u16(test, 0xF842, 1);                  // process.cwd
    test_set_u16(test, 0xF844, 0);                  // process.SP
    test_set_u16(test, 0xF846, 0xFFFF);             // process.stdout
    test_set_u16(test, 0xF848, 0xFFFF);             // process.stdin

    test_gpu_print(test, "ABCDEFGHIJKLMNOPQRSTUVWXYZ\n"
                         "abcdefghijklmnopqrstuvwxyz\n"
                         "0123456789\n"
                         "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}\1\n");
    test_check(test);
  }

  {
    printf("  EXIT\n");
    int calls = 1;
    test_step(test_);
    while (test->running && calls != 0) {
      if (cpu->RAM[cpu->IP] == CALL || cpu->RAM[cpu->IP] == CALLR) {
        calls++;
      } else if (cpu->RAM[cpu->IP] == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_step(test_);
    test_step(test_);

    test_set_u16(test, 0xF802, 0xF830);            // os.current_process
    test_set_u16(test, 0xF804, 0b11 << (16 - 2));  // os.process_map
    test_set_u16(test, 0xF806, 0b111 << (16 - 3)); // os.page_map
    test_check(test);
  }

  {
    printf("  EXECUTE `echo ab cd ef`\n");
    load_input_string(cpu, "echo ab cd ef\n");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_unset_range(test, 3 * PAGE_SIZE, PAGE_SIZE);
    test_gpu_print(test, "$ echo ab cd ef\n");
    test_gpu_print(test, "ab cd ef\n");
    test_set_range(test, sh_input_pos, 14, (uint8_t *)"echo\0ab cd ef\0");

    test_check(test);
  }

  {
    printf("  EXECUTE `cd dir`\n");

    load_input_string(cpu, "cd dir\n");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_gpu_print(test, "$ cd dir\n");
    test_set_range(test, sh_input_pos, 7, (uint8_t *)"cd\0dir\0");

    test_set_u16(test, 0xF832, mem_sector_find_entry(cpu->MEM + 256, "dir"));

    test_check(test);
  }

  {
    printf("  EXECUTE `/ls`\n");

    load_input_string(cpu, "/ls\n");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_gpu_print(test, "$ /ls\n");
    test_set_range(test, sh_input_pos, 4, (uint8_t *)"/ls\0");

    test_set_u16(test, 0xF842, mem_sector_find_entry(cpu->MEM + 256, "dir"));
    test_gpu_print(test, "a a.sk \n");

    test_check(test);
  }

  {
    printf("  EXECUTE `/cd`\n");

    load_input_string(cpu, "/cd\n");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_gpu_print(test, "$ /cd\n");
    test_set_range(test, sh_input_pos, 4, (uint8_t *)"/cd\0");

    test_set_u16(test, 0xF832, 1);

    test_check(test);
  }

  {
    printf("  EXECUTE `cat dir/a`\n");

    load_input_string(cpu, "cat dir/a\n");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_gpu_print(test, "$ cat dir/a\n");
    test_set_range(test, sh_input_pos, 10, (uint8_t *)"cat\0dir/a\0");
    uint16_t sec = mem_sector_find_entry(cpu->MEM + 256 * mem_sector_find_entry(cpu->MEM + 256, "dir"), "a");
    test_assert(cpu->MEM[256 * sec] == 'F');
    test_assert(cpu->MEM[256 * sec + 1] == 0xFF);
    test_assert(cpu->MEM[256 * sec + 2] == 0xFF);
    for (int i = 4; i <= cpu->MEM[256 * sec + 3]; ++i) {
      test_gpu_put_char(test, cpu->MEM[256 * sec + i]);
    }
    test_set_u16(test, 0xF842, 1);

    test_check(test);
  }

  {
    printf("  EXECUTE `tee out`\n");

    load_input_string(cpu, "tee out\n");
    load_input_string(cpu, "ciao\\D");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_gpu_print(test, "$ tee out\n");
    test_set_range(test, sh_input_pos, 8, (uint8_t *)"tee\0out\0");
    test_gpu_print(test, "ciao\n");

    test_check(test);

    printf("  TEST FILE `out`\n");

    uint16_t sec = mem_sector_find_entry(cpu->MEM + 256, "out");
    test_assert(sec != 0);
    test_assert(cpu->MEM[256 * sec] == 'F');
    test_assert(cpu->MEM[256 * sec + 1] == 0xFF);
    test_assert(cpu->MEM[256 * sec + 2] == 0xFF);
    test_assert(cpu->MEM[256 * sec + 3] == 7);
    test_assert(cpu->MEM[256 * sec + 4] == 'c');
    test_assert(cpu->MEM[256 * sec + 5] == 'i');
    test_assert(cpu->MEM[256 * sec + 6] == 'a');
    test_assert(cpu->MEM[256 * sec + 7] == 'o');
    test_assert(cpu->MEM[256 * sec + 8] == 0);
  }

  {
    printf("  EXECUTE `clear`\n");

    load_input_string(cpu, "clear\n");

    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    memset(test->test_gpu, 0, sizeof(test->test_gpu));
    test->gpu_x = 0;
    test->gpu_y = 0;
    test_set_range(test, sh_input_pos, 6, (uint8_t *)"clear\0");

    test_check(test);
  }

  if (0) {
    printf("  EXECUTE `rule110`\n");

    load_input_string(cpu, "rule110\n");
    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == exit_pos);
    test_assert_running(test_);
    int calls = 1;
    while (test->running && calls != 0) {
      if (cpu->IR == CALL || cpu->IR == CALLR) {
        calls++;
      } else if (cpu->IR == RET) {
        calls--;
      }
      tick(cpu, &test->running);
    }
    test_assert_running(test_);

    test_gpu_print(test, "$ rule110\n");
    test_set_range(test, sh_input_pos, 8, (uint8_t *)"rule110\0");
    char *result = "         #|\n"
                   "        ##|\n"
                   "       ###|\n"
                   "      ## #|\n"
                   "     #####|\n"
                   "    ##   #|\n"
                   "   ###  ##|\n"
                   "  ## # ###|\n"
                   " ####### #|\n"
                   "##     ###|\n"
                   " #    ##  |\n"
                   "##   ###  |\n"
                   "##  ## # #|\n"
                   " # #######|\n"
                   "####     #|\n"
                   "   #    ##|\n";
    for (int j = 0; j < 16 * (10 + 1); ++j) {
      switch (result[j]) {
        case ' ': test_gpu_put_char(test, 0); break;
        case '#': test_gpu_put_char(test, 1); break;
        default: test_gpu_put_char(test, result[j]); break;
      }
    }

    test_check(test);
  }

  {
    printf("  EXECUTE `pipe echo ... tee`\n");

    load_input_string(cpu, "pipe echo ciao | tee out\n");
    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == execute_pos);
    test_assert_running(test_);
    test_run_until(test_, cpu->IR == JMPA);
    test_assert_running(test_);
    test_run_until(test_, cpu->IR == CALL && cpu_read16(cpu, cpu->IP) == execute_pos);
    test_assert_running(test_);

    exe_t pipe = exe_decode_file("asm/bin/pipe");
    exe_reloc(&pipe, 3 * PAGE_SIZE, stdlib_pos);

    test_gpu_print(test, "$ pipe echo ciao | tee out\n");
    test_set_range(test, sh_input_pos, 25, (uint8_t *)"pipe\0echo ciao | tee out\0");

    test_check(test);

    printf("  EXECUTE 1st command\n");

    test_set_range(test, sh_input_pos, 25, (uint8_t *)"pipe\0echo\0ciao \0 tee out\0");
    test_unset_range(test, 4 * PAGE_SIZE, PAGE_SIZE);
    test_unset_range(test, 0xF850, 16);

    test_check(test);
  }

  printf("TODO brainfuck/bfjit\n");
  printf("TODO stack\n");

  {
    printf("  EXECUTE `shutdown`\n");

    load_input_string(cpu, "shutdown\n");

    while (test->running) {
      tick(cpu, &test->running);
    }

    test_assert(!test->running);
    test_assert(cpu->A == 0xA0A0);
    test_gpu_print(test, "$ shutdown\n");
    test_set_range(test, sh_input_pos, 9, (uint8_t *)"shutdown\0");
    test_gpu_print(test, "BYE\n");

    test_set_u16(test, 0xF802, 0xF840);             // os.current_process
    test_set_u16(test, 0xF804, 0b111 << (16 - 3));  // os.process_map
    test_set_u16(test, 0xF806, 0b1111 << (16 - 4)); // os.page_map

    test_check(test);
  }

  // {
  //   load_input_string(cpu, "pipe echo ciao | tee out\n");
  //   printf("  LOAD `pipe echo ciao | tee out`\n");
  //   // test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);
  //   // test_assert_running(test_);
  //   // test_run_until(test_, cpu->IR != JMPA);
  //   // test_assert_running(test_);
  //   exe_t pipe = exe_decode_file("asm/bin/pipe");
  //   exe_reloc(&pipe, 3 * PAGE_SIZE, stdlib_pos);
  //   test_set_range(test, 3 * PAGE_SIZE, pipe.code_size, pipe.code);
  //   test_gpu_print(test, "$ pipe echo ciao | tee out\n");
  //   test_set_range(test, sh_input_pos, 25, (uint8_t *)"pipe\0echo ciao | tee out\0");
  //   // test_set_u16(test, 0xF802, 0xF840);
  //   // test_set_u16(test, 0xF804, 0xE000);
  //   // test_set_u16(test, 0xF806, 0xF000);
  //   // test_set_u16(test, 0xF842, 1);
  //   test_check(test);
  //
  //   printf("  RUN `echo`\n");
  //   test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == execute_pos);
  //   test_assert_running(test_);
  //   test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_pos);
  //   test_assert_running(test_);
  //   test_run_until(test_, cpu->IR == JMPA);
  //   test_assert_running(test_);
  //
  //   test_set_range(test, sh_input_pos, 25, (uint8_t *)"pipe\0echo\0ciao \0 tee out\0");
  //   uint16_t stdout_pos = test_find_symbol(pipe.symbols, pipe.symbol_count, "stdout") + 3 * PAGE_SIZE;
  //   test_set_u16(test, stdout_pos, stdout_pos + 6);               // setup stdout
  //   test_set_u16(test, stdout_pos + 2, stdout_pos + 6);           // setup stdout
  //   test_set_u16(test, stdout_pos + 4, 4 * PAGE_SIZE - 0x30 - 8); // setup stdout
  //   test_set_u16(test, 0xF846, stdout_pos);                       // stdout redirect
  //   test_check(test);
  //
  //   printf("  RUN `echo`\n");
  //   test_run_until(test_, cpu->RAM[cpu->IP] == CALL && cpu_read16(cpu, cpu->IP + 1) == exit_pos);
  //   test_assert_running(test_);
  //   exe_t echo = exe_decode_file("asm/bin/echo");
  //   exe_reloc(&echo, 4 * PAGE_SIZE, stdlib_pos);
  //   test_unset_range(test, 4 * PAGE_SIZE, PAGE_SIZE); // unset the stack and the code of the program and then set the code
  //   test_set_range(test, 4 * PAGE_SIZE, echo.code_size, echo.code);
  //
  //   test_check(test);

  // test_unset_range(test, 3 * PAGE_SIZE, PAGE_SIZE); // unset the stack and the code of the program and then set the code
  //}

  printf("END\n\n");
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
