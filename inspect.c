#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "argparse/argparse.h"
#include "files.h"
#include "mystb/errors.h"

typedef enum {
  KUNSET,
  KOBJ,
  KEXE,
  KSO,
  KMEM,
} kind_t;

void disassemble(uint8_t *code, uint16_t code_size);

void inspect_obj(char *filename, int disassemble_flag) {
  assert(filename);

  obj_t obj = obj_decode_file(filename);
  obj_dump(&obj);
  if (disassemble_flag) {
    disassemble(obj.code, obj.code_size);
  }
}

void inspect_exe(char *filename, int disassemble_flag) {
  assert(filename);

  exe_t exe = exe_decode_file(filename);
  exe_dump(&exe);
  if (disassemble_flag) {
    disassemble(exe.code, exe.code_size);
  }
}
void inspect_so(char *filename, int disassemble_flag) {
  assert(filename);

  exe_state_t exes = so_decode_file(filename);
  so_dump(&exes);
  if (disassemble_flag) {
    disassemble(exes.exe.code, exes.exe.code_size);
  }
}

void inspect_mem(char *filename) {
  assert(filename);

  uint8_t sectors[2048][256] = {0};

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }
  assert(fread(sectors, 1, 1 << 19, file) == 1 << 19);
  assert(fclose(file) == 0);

  for (int i = 1; sectors[i][0] != 0; ++i) {
    uint8_t *sector = sectors[i];
    printf("%d ", i);
    if (sector[0] == 'D') {
      printf("DIR:\n");
      printf("\tNEXT: %d\n", sector[1] | (sector[2] << 8));
      printf("\tENTRIES:\n");
      int a = 3;
      while (sector[a] != 0) {
        int len = strlen((char *)(sector + a));
        printf("\t\t%s %d\n", sector + a, sector[a + len + 1] | (sector[a + len + 2] << 8));
        a += len + 3;
      }
    } else if (sector[0] == 'F') {
      printf("FILE:\n");
      printf("\tNEXT: %d\n", sector[1] | (sector[2] << 8));
      printf("\tMAX INDEX: %d\n", sector[3]);
    } else {
      assert(0);
    }
  }
}

int main(int argc, char **argv) {
  int kind = 0;
  int disassemble_flag = 0;

  struct argparse_option options[] = {
    OPT_GROUP("Options:"),
    OPT_BOOLEAN('d', "disassemble", &disassemble_flag, "dump disassembled code", NULL, 0, 0),
    OPT_HELP(),
    OPT_GROUP("Kinds:"),
    OPT_BIT(0, "obj", &kind, "analyse the file as an obj", NULL, KOBJ, 0),
    OPT_BIT(0, "exe", &kind, "analyse the file as an exe", NULL, KEXE, 0),
    OPT_BIT(0, "so", &kind, "analyse the file as a so", NULL, KSO, 0),
    OPT_BIT(0, "mem", &kind, "analyse the file as a mem.bin file", NULL, KMEM, 0),
    OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse,
                options,
                (const char *const[]){
                  "inspect [options] [kind] file",
                  NULL,
                },
                0);
  argparse_describe(&argparse, NULL, "if the kind is not specified it's deduced from the file extension");
  argc = argparse_parse(&argparse, argc, (const char **)argv);
  if (argc != 1) {
    fprintf(stderr, "ERROR: expected ONE file\n");
    argparse_usage(&argparse);
    exit(1);
  }

  char *filename = argv[0];
  int len = strlen(filename);

  switch (kind) {
    case KUNSET:
      if (strcmp(filename + len - 2, ".o") == 0) {
        inspect_obj(filename, disassemble_flag);
      } else if (strcmp(filename + len - 4, ".exe") == 0) {
        inspect_exe(filename, disassemble_flag);
      } else if (strcmp(filename + len - 3, ".so") == 0) {
        inspect_so(filename, disassemble_flag);
      } else if (strcmp(filename, "mem.bin") == 0) {
        inspect_mem(filename);
      } else {
        fprintf(stderr, "ERROR: cannot deduce file kind from '%s'\n", filename);
        exit(1);
      }
      break;
    case KOBJ:
      inspect_obj(filename, disassemble_flag);
      break;
    case KEXE:
      inspect_exe(filename, disassemble_flag);
      break;
    case KSO:
      inspect_so(filename, disassemble_flag);
      break;
    case KMEM:
      inspect_mem(filename);
      break;
    default:
      fprintf(stderr, "ERROR: expected ONE kind specification\n");
      argparse_usage(&argparse);
      exit(1);
  }

  return 0;
}

void disassemble(uint8_t *code, uint16_t code_size) {
  assert(code);

  char *inst = NULL;
  int first_inst = 1;

  printf("DISASSEMBLED CODE:\n\t");
  for (int i = 0; i < code_size;) {
    inst = instruction_to_string(code[i]);
    if (code[i] != 0 && inst != NULL) {
      if (first_inst) {
        printf("\n\t");
        first_inst = 0;
      }
      switch (instruction_stat(code[i]).arg) {
        case INST_NO_ARGS:
          printf("%-*s ", INSTRUCTION_MAX_LEN, inst);
          ++i;
          break;
        case INST_8BITS_ARG:
          printf("%-*s %02X ", INSTRUCTION_MAX_LEN, inst, code[i + 1]);
          i += 2;
          break;
        case INST_16BITS_ARG:
        case INST_LABEL_ARG:
          printf("%-*s %04X ", INSTRUCTION_MAX_LEN, inst, code[i + 1] | (code[i + 2] << 8));
          i += 3;
          break;
        case INST_RELLABEL_ARG:
          printf("%-*s %d ", INSTRUCTION_MAX_LEN, inst, (int16_t)(code[i + 1] | (code[i + 2] << 8)));
          i += 3;
          break;
      }
      printf("\n\t");
    } else {
      printf("%02X ", code[i]);
      ++i;
    }
  }
  printf("\n");
}
