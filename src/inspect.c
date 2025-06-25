#include <assert.h>
#include <errno.h>

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"
#include "files.h"

typedef enum {
  KUNSET,
  KOBJ,
  KEXE,
  KSO,
  KMEM,
  KBIN,
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

  so_t so = so_decode_file(filename);
  so_dump(&so);
  if (disassemble_flag) {
    disassemble(so.code, so.code_size);
  }
}

void inspect_bin(char *filename, int disassemble_flag) {
  assert(filename);

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  size_t size = ftell(file);
  assert(fseek(file, 0, SEEK_SET) == 0);
  uint8_t buffer[size];
  assert(fread(buffer, 1, size, file) == size);
  assert(fclose(file) == 0);

  printf("CODE:\n\t");
  for (size_t i = 0; i < size; ++i) {
    if (i != 0) {
      printf(" ");
    }
    printf("%02X", buffer[i]);
  }
  printf("\n");
  if (disassemble_flag) {
    disassemble(buffer, size);
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

  printf("0 BOOTLOADER:\n");
  printf("\tOS SEC: %d\n", sectors[0][252] | (sectors[0][253] << 8));
  printf("\tSTDLIB SEC: %d\n", sectors[0][254] | (sectors[0][255] << 8));

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
        printf("\t\t%s ", sector + a);
        if (sector[a] == 1 && sector[a + 1] == 0) {
          printf("(OS) ");
        } else if (sector[a] == 2 && sector[a + 1] == 0) {
          printf("(STDLIB) ");
        }
        printf("%d\n", sector[a + len + 1] | (sector[a + len + 2] << 8));
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

void print_help() {
  printf("Usage: inspect [kind] [options] <input>\n\n"
         "Options:\n"
         "  -d           print the disassembled code\n"
         "  -h | --help  show help message\n"
         "\nKinds:\n"
         "if the kind is not specified it's deduced from the file extension\n\n"
         "  --obj  analyse the input as an obj\n"
         "  --exe  analyse the input as an exe\n"
         "  --so   analyse the input as a so\n"
         "  --mem  analyse the input as a memory bin\n"
         "  --bin  analyse the input as a bin (just plain code)\n");
}

int main(int argc, char **argv) {
  char *path = NULL;
  int kind = KUNSET;
  int disassemble = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                       //
        else ARG_PARSE_FLAG("d", "disassemble", disassemble) //
        else if (ARG_LFLAG("obj")) {
      kind = KOBJ;
    }
    else if (ARG_LFLAG("exe")) {
      kind = KEXE;
    }
    else if (ARG_LFLAG("so")) {
      kind = KSO;
    }
    else if (ARG_LFLAG("mem")) {
      kind = KMEM;
    }
    else if (ARG_LFLAG("bin")) {
      kind = KBIN;
    }
    else {
      if (path != NULL) {
        eprintf("file already provided: %s", path);
      }
      path = *argv;
    }
  }

  unsigned int len = strlen(path);

  if (kind == KUNSET) {
    if (strcmp(path + len - 2, ".o") == 0) {
      kind = KOBJ;
    } else if (strcmp(path + len - 4, ".exe") == 0) {
      kind = KEXE;
    } else if (strcmp(path + len - 3, ".so") == 0) {
      kind = KSO;
    } else if (strcmp(path, "mem.bin") == 0) {
      kind = KMEM;
    } else {
      FILE *file = fopen(path, "rb");
      if (!file) {
        eprintf("cannot open file '%s': '%s'", path, strerror(errno));
      }
      char magic_number[4] = {0};
      assert(fread(magic_number, 1, 3, file) == 3);
      assert(fclose(file) == 0);

      if (strcmp(magic_number, "EXE") == 0) {
        kind = KEXE;
      } else if (strcmp(magic_number, "OBJ") == 0) {
        kind = KOBJ;
      } else if (magic_number[0] == 'S' && magic_number[1] == 'O') {
        kind = KSO;
      } else {
        fprintf(stderr, "ERROR: cannot deduce file kind from '%s'\n", path);
        exit(1);
      }
    }
  }

  switch (kind) {
    case KUNSET:
      assert(0);
      break;
    case KOBJ:
      inspect_obj(path, disassemble);
      break;
    case KEXE:
      inspect_exe(path, disassemble);
      break;
    case KSO:
      inspect_so(path, disassemble);
      break;
    case KMEM:
      inspect_mem(path);
      break;
    case KBIN:
      inspect_bin(path, disassemble);
      break;
    default:
      assert(0 && "UNREACHABLE");
  }

  return 0;
}

void disassemble(uint8_t *code, uint16_t code_size) {
  (void)code;
  (void)code_size;
  assert(0 && "TODO");
}
