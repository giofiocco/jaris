#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/wait.h>

#define MIN(__a__, __b__) ((__a__) > (__b__) ? (__b__) : (__a__))
#define ROWLEN            40

#define BUFFER_SIZE 2048

typedef struct {
  char stdout[BUFFER_SIZE];
  char stderr[BUFFER_SIZE];
  int exitcode;
} test_result_t;

test_result_t spawn_process_array(char *args[]) {
  printf("RUNNING:");
  for (int i = 0; args[i]; ++i) {
    printf(" %s", args[i]);
  }
  putchar('\n');

  int outpipe[2];
  if (pipe(outpipe) != 0) {
    perror("pipe stdout");
    exit(1);
  }
  int errpipe[2];
  if (pipe(errpipe) != 0) {
    perror("pipe stderr");
    exit(1);
  }

  test_result_t result = {0};

  switch (fork()) {
    case -1:
      perror("fork");
      exit(1);
    case 0:
      close(outpipe[0]);
      dup2(outpipe[1], 1);
      close(errpipe[0]);
      dup2(errpipe[1], 2);
      execvp(args[0], args);
      perror("execvp");
      break;
    default:
      close(outpipe[1]);
      close(errpipe[1]);
      wait(&result.exitcode);
  }

  read(outpipe[0], result.stdout, BUFFER_SIZE);
  close(outpipe[0]);

  read(errpipe[0], result.stderr, BUFFER_SIZE);
  close(errpipe[0]);

  return result;
}

#define MAX_ARGS 64
test_result_t spawn_process(char *command, ...) {
  va_list args_list;
  va_start(args_list, command);
  char *args[MAX_ARGS] = {command, 0};
  char *arg = NULL;
  for (int i = 1; (arg = va_arg(args_list, char *)) != NULL;) {
    assert(i + 1 < MAX_ARGS);
    args[i++] = arg;
  }
  va_end(args_list);

  return spawn_process_array(args);
}

void delete_file(char *path) {
  printf("DELETING %s\n", path);
  assert(remove(path) == 0);
}

void print_to_file(char *path, char *src) {
  printf("CREATING %s\n", path);
  FILE *file = fopen(path, "w");
  assert(file);
  unsigned long len = strlen(src);
  assert(fwrite(src, 1, len, file) == len);
  assert(fclose(file) == 0);
}

test_result_t test(char *args[], int expected_exitcode, char *expected_output, char *expected_error) {
  test_result_t result = spawn_process_array(args);

  int ok = 1;
  if (expected_exitcode != result.exitcode) {
    printf("  EXITCODE DIFFER: expected %d, found %d\n", expected_exitcode, result.exitcode);
    ok = 0;
  }
  if (strcmp(expected_output, result.stdout) != 0) {
    printf("  STDOUT DIFFER\n");
    ok = 0;
  }
  if (strcmp(expected_error, result.stderr) != 0) {
    printf("  STDERR DIFFER\n");
    ok = 0;
  }
  if (ok) {
    printf("  OK\n");
  } else {
    printf("  FAILED\n");
  }

  return result;
}

void test_assembler_tokenizer() {
  char *asmpath = "testfile.asm";
  char *objpath = "testfile.asm.o";
  print_to_file(asmpath,
                "GLOBAL _start\n"
                "EXTERN exit\n"
                "\t{a 0x01}\n"
                "\t{ b 0x00 0x00 }\n"
                "foo:\n"
                "\tJMPR $foo\n"
                "_start:\n"
                "\tRAM_AL 0x00 CALL exit\n"
                "ALIGN\n"
                "str: \"ciao\" 0x0A 0x00\n"
                "db 4\n"
                "0b01010110\n");
  test((char *[]){"./assembler", "-d", "tok", "-o", objpath, asmpath, NULL},
       0,
       "testfile.asm:01:01: GLOBAL 'GLOBAL'\n"
       "testfile.asm:01:08: SYM '_start'\n"
       "testfile.asm:02:01: EXTERN 'EXTERN'\n"
       "testfile.asm:02:08: SYM 'exit'\n"
       "testfile.asm:05:01: SYM 'foo'\n"
       "testfile.asm:05:04: COLON ':'\n"
       "testfile.asm:06:02: INST 'JMPR'\n"
       "testfile.asm:06:07: REL '$'\n"
       "testfile.asm:06:08: SYM 'foo'\n"
       "testfile.asm:07:01: SYM '_start'\n"
       "testfile.asm:07:07: COLON ':'\n"
       "testfile.asm:08:02: INST 'RAM_AL'\n"
       "testfile.asm:08:09: HEX '0x00'\n"
       "testfile.asm:08:14: INST 'CALL'\n"
       "testfile.asm:08:19: SYM 'exit'\n"
       "testfile.asm:09:01: ALIGN 'ALIGN'\n"
       "testfile.asm:10:01: SYM 'str'\n"
       "testfile.asm:10:04: COLON ':'\n"
       "testfile.asm:10:06: STRING '\"ciao\"'\n"
       "testfile.asm:10:13: HEX '0x0A'\n"
       "testfile.asm:10:18: HEX '0x00'\n"
       "testfile.asm:11:01: DB 'db'\n"
       "testfile.asm:11:04: INT '4'\n"
       "testfile.asm:12:01: BIN '0b01010110'\n",
       "");
  delete_file(asmpath);
  delete_file(objpath);
}

void test_linker() {
  char *asmpath = "linkerfile.asm";
  char *objpath = "linkerfile.asm.o";
  char *exepath = "linkerfile";
  print_to_file(asmpath,
                "GLOBAL _start\n"
                "GLOBAL a\n"
                "a: 0x0000\n"
                "b: 0x0000\n"
                "_start:\n"
                "  HLT\n");
  test_result_t result = spawn_process_array((char *[]){"./assembler", "-o", objpath, asmpath, NULL});
  assert(result.exitcode == 0);
  assert(strcmp(result.stderr, "") == 0);
  result = test((char *[]){"./linker", "-d", "-o", exepath, objpath, NULL},
                0,
                "EXE:\n"
                "CODE: 10\n"
                "	00 18 00 00 00 00 00 00 3F 00\n"
                "RELOCS: 0\n"
                "DYNAMIC LINKING: 0\n"
                "SYMBOLS: 2\n"
                "00) _start: 0008\n"
                "	0002 \n"
                "01) a: 0004\n"
                "GLOBALS: 0 1\n"
                "EXTERNS: 0\n"
                "SOS: 1\n"
                "(STDLIB):\n"
                "		div 0000\n"
                "		mul 0044\n"
                "		solve_path 0066\n"
                "		open_file 010C\n"
                "		read_u8 013E\n"
                "		read_u16 01A6\n"
                "		execute 01CE\n"
                "		exit 039E\n"
                "		print 042C\n"
                "		put_char 0440\n"
                "		print_int 04AF\n"
                "		get_char 04D6\n"
                "		str_find_char 0600\n"
                "		is_digit 061A\n",
                "");
  delete_file(asmpath);
  delete_file(objpath);
  delete_file(exepath);
}

void test_linker_debug_info() {
  char *asmpath = "linkerfile.asm";
  char *objpath = "linkerfile.asm.o";
  char *exepath = "linkerfile";
  print_to_file(asmpath,
                "GLOBAL _start\n"
                "GLOBAL a\n"
                "a: 0x0000\n"
                "b: 0x0000\n"
                "_start:\n"
                "  HLT\n");
  test_result_t result = spawn_process_array((char *[]){"./assembler", "-g", "-o", objpath, asmpath, NULL});
  assert(result.exitcode == 0);
  assert(strcmp(result.stderr, "") == 0);
  result = test((char *[]){"./linker", "-g", "-d", "-o", exepath, objpath, NULL},
                0,
                "EXE:\n"
                "CODE: 10\n"
                "	00 18 00 00 00 00 00 00 3F 00\n"
                "RELOCS: 0\n"
                "DYNAMIC LINKING: 0\n"
                "SYMBOLS: 3\n"
                "00) _start: 0008\n"
                "	0002 \n"
                "01) a: 0004\n"
                "02) _003_b: 0006\n"
                "GLOBALS: 0 1\n"
                "EXTERNS: 0\n"
                "SOS: 1\n"
                "(STDLIB):\n"
                "		div 0000\n"
                "		mul 0044\n"
                "		solve_path 0066\n"
                "		open_file 010C\n"
                "		read_u8 013E\n"
                "		read_u16 01A6\n"
                "		execute 01CE\n"
                "		exit 039E\n"
                "		print 042C\n"
                "		put_char 0440\n"
                "		print_int 04AF\n"
                "		get_char 04D6\n"
                "		str_find_char 0600\n"
                "		is_digit 061A\n",
                "");
  delete_file(asmpath);
  delete_file(objpath);
  delete_file(exepath);
}

int main() {
  void (*funcs[])() = {
      test_assembler_tokenizer,
      test_linker,
      test_linker_debug_info,
  };

  int count = sizeof(funcs) / sizeof(funcs[0]);
  for (int i = 0; i < count; ++i) {
    funcs[i]();
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
  }

  printf("TODO: test encodemem\n");
  printf("TODO: test inspect\n");

  return 0;
}
