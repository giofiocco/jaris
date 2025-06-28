#include <assert.h>
#include <ctype.h>
#include <errno.h>

#define ERRORS_IMPLEMENTATION
#define SV_IMPLEMENTATION
#include "../argparse.h"
#include "../instructions.h"
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

void disassemble(uint8_t *code, uint16_t code_size, symbol_t *symbols, uint16_t symbols_count);

void inspect_obj(char *filename, int disassemble_flag) {
  assert(filename);

  obj_t obj = obj_decode_file(filename);
  obj_dump(&obj);
  if (disassemble_flag) {
    disassemble(obj.code, obj.code_size, obj.symbols, obj.symbol_count);
  }
}

void inspect_exe(char *filename, int disassemble_flag) {
  assert(filename);

  exe_t exe = exe_decode_file(filename);
  exe_dump(&exe);
  if (disassemble_flag) {
    disassemble(exe.code, exe.code_size, exe.symbols, exe.symbol_count);
  }
}
void inspect_so(char *filename, int disassemble_flag) {
  assert(filename);

  so_t so = so_decode_file(filename);
  so_dump(&so);
  if (disassemble_flag) {
    disassemble(so.code, so.code_size, so.symbols, so.symbol_count);
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
    disassemble(buffer, size, NULL, 0);
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

int compare_symbols(const void *a, const void *b) {
  assert(a);
  assert(b);
  return ((symbol_t *)a)->pos - ((symbol_t *)b)->pos;
}

void disassemble(uint8_t *code, uint16_t code_size, symbol_t *symbols, uint16_t symbols_count) {
  char *set_labels[code_size];
  memset(set_labels, 0, code_size * sizeof(char *));
  char *labels[code_size];
  memset(labels, 0, code_size * sizeof(char *));
  char *rellabels[code_size];
  memset(rellabels, 0, code_size * sizeof(char *));

  for (int i = 0; i < symbols_count; ++i) {
    if (symbols[i].pos != 0xFFFF) {
      assert(symbols[i].pos < code_size);
      set_labels[symbols[i].pos] = symbols[i].image;
    }
    for (int j = 0; j < symbols[i].reloc_count; ++j) {
      assert(symbols[i].relocs[j] < code_size);
      labels[symbols[i].relocs[j]] = symbols[i].image;
    }
    for (int j = 0; j < symbols[i].relreloc_count; ++j) {
      assert(symbols[i].relrelocs[j] < code_size);
      rellabels[symbols[i].relrelocs[j]] = symbols[i].image;
    }
  }

  bytecode_t bcs[code_size];
  uint16_t bc_count = 0;

  printf("DISASSEMBLE:\n");
  for (int i = 0; i < code_size;) {
    if (set_labels[i] != NULL) {
      bcs[bc_count++] = bytecode_with_string(BSETLABEL, 0, set_labels[i]);
      // if (code[i] == 0 && i + 1 < code_size && code[i] == 0) {
      //   int j = i + 1;
      //   while (j < code_size && code[j] == 0 && set_labels[j] == NULL) {
      //     j++;
      //   }
      //    bcs[bc_count++] = (bytecode_t){BDB, 0, {.num = j - i}};
      //    i = j;
      //    continue;
      // }
    }
    if (instruction_to_string(code[i]) != NULL) {
      switch (instruction_stat(code[i]).arg) {
        case INST_NO_ARGS:
          bcs[bc_count++] = (bytecode_t){BINST, code[i], {}};
          i++;
          break;
        case INST_8BITS_ARG:
          assert(i + 1 < code_size);
          bcs[bc_count++] = (bytecode_t){BINSTHEX, code[i], {.num = code[i + 1]}};
          i += 2;
          break;
        case INST_16BITS_ARG:
          assert(i + 2 < code_size);
          if (code[i] == JMP) {
            assert(labels[i + 1]);
            bcs[bc_count++] = bytecode_with_string(BINSTLABEL, code[i], labels[i + 1]);
          } else {
            bcs[bc_count++] = (bytecode_t){BINSTHEX2, code[i], {.num = code[i + 1] | (code[i + 2] << 8)}};
          }
          i += 3;
          break;
        case INST_LABEL_ARG:
          assert(i + 2 < code_size);
          assert(labels[i + 1]);
          bcs[bc_count++] = bytecode_with_string(BINSTLABEL, code[i], labels[i + 1]);
          i += 3;
          break;
        case INST_RELLABEL_ARG:
          assert(i + 2 < code_size);
          assert(rellabels[i + 1]);
          bcs[bc_count++] = bytecode_with_string(BINSTRELLABEL, code[i], rellabels[i + 1]);
          i += 3;
          break;
      }
    } else if (isprint(code[i])) {
      int j = i + 1;
      while (j < code_size && code[j] != 0) {
        assert(set_labels[j] == NULL);
        j++;
      }
      assert(code[j] == 0);
      bcs[bc_count++] = bytecode_with_sv(BSTRING, 0, (sv_t){(char *)(code + i), j - i});
      bcs[bc_count++] = (bytecode_t){BHEX, 0, {.num = 0}};
      i = j + 1;
    } else {
      bcs[bc_count++] = (bytecode_t){BHEX, 0, {.num = code[i]}};
      i++;
    }
  }

  for (int i = 0; i < bc_count; ++i) {
    bytecode_dump(bcs[i]);
  }
}
