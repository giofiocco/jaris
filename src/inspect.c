#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>

#define ERRORS_IMPLEMENTATION
#define SV_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"
#include "files.h"
#include "instructions.h"

typedef enum {
  KUNSET,
  KOBJ,
  KEXE,
  KSO,
  KMEM,
  KBIN,
  KFONT,
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
    eprintf("cannot open file '%s': %s", filename, strerror(errno));
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

void inspect_font(char *filename) {
  assert(filename);

  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': %s", filename, strerror(errno));
  }

  char magic_number[5] = {0};
  assert(fread(magic_number, 1, 4, file) == 4);
  if (strcmp(magic_number, "FONT") != 0) {
    eprintf("%s: expected magic number to be 'FONT': found '%s'", filename, magic_number);
  }

  int count = 0;
  assert(fread(&count, 2, 1, file) == 1);
  assert(count > 0);

  printf("PATTERNS COUNT: %d\n", count);

  char patterns[count][9];
  assert(fread(patterns, 1, 9 * count, file) == (unsigned int)9 * count);

#define INSPECT_FONT_WRAPPING 8

  for (int i = 0; i < count - INSPECT_FONT_WRAPPING + 1; i += INSPECT_FONT_WRAPPING) {
    for (int d = 0; d < INSPECT_FONT_WRAPPING; ++d) {
      char c = patterns[i + d][0];
      printf("%-3d '%c'   ", c, isprint(c) ? c : ' ');
    }
    putchar('\n');

    for (int j = 0; j < 8; ++j) {
      for (int d = 0; d < INSPECT_FONT_WRAPPING; ++d) {
        for (int k = 0; k < 8; ++k) {
          putchar(patterns[i + d][j + 1] >> (7 - k) & 1 ? '#' : ' ');
        }
        putchar('|');
        putchar(' ');
      }
      putchar('\n');
    }
  }

  for (int i = 0; i < count % INSPECT_FONT_WRAPPING; ++i) {
    char c = patterns[count - count % INSPECT_FONT_WRAPPING + i][0];
    printf("%-3d '%c'   ", c, isprint(c) ? c : ' ');
  }
  putchar('\n');

  for (int j = 0; j < 8; ++j) {
    for (int i = 0; i < count % INSPECT_FONT_WRAPPING; ++i) {
      for (int k = 0; k < 8; ++k) {
        putchar(patterns[count - count % INSPECT_FONT_WRAPPING + i][j + 1] >> (7 - k) & 1 ? '#' : ' ');
      }
      putchar('|');
      putchar(' ');
    }
    putchar('\n');
  }
}

void print_help() {
  printf("Usage: inspect [kind] [options] <input>\n\n"
         "Options:\n"
         "  -d           print the disassembled code\n"
         "  -h | --help  show help message\n"
         "\nKinds:\n"
         "if the kind is not specified it's deduced from the file extension\n\n"
         "  --obj   analyse the input as an obj\n"
         "  --exe   analyse the input as an exe\n"
         "  --so    analyse the input as a so\n"
         "  --mem   analyse the input as a memory bin\n"
         "  --bin   analyse the input as a bin (just plain code)\n"
         "  --font  analyse the input as a font\n");
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
    else if (ARG_LFLAG("font")) {
      kind = KFONT;
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
    } else if (strcmp(path + len - 5, ".font") == 0) {
      kind = KFONT;
    } else {
      FILE *file = fopen(path, "rb");
      if (!file) {
        eprintf("cannot open file '%s': '%s'", path, strerror(errno));
      }

#define MAGIC_NUMBER_MAX_LEN 5
      char magic_number[MAGIC_NUMBER_MAX_LEN + 1] = {0};
      assert(fread(magic_number, 1, MAGIC_NUMBER_MAX_LEN, file) == MAGIC_NUMBER_MAX_LEN);
      assert(fclose(file) == 0);

      if (sv_eq((sv_t){magic_number, 3}, sv_from_cstr("EXE"))) {
        kind = KEXE;
      } else if (sv_eq((sv_t){magic_number, 3}, sv_from_cstr("OBJ"))) {
        kind = KOBJ;
      } else if (sv_eq((sv_t){magic_number, 2}, sv_from_cstr("SO"))) {
        kind = KSO;
      } else if (sv_eq((sv_t){magic_number, 4}, sv_from_cstr("FONT"))) {
        kind = KFONT;
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
    case KFONT:
      inspect_font(path);
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

  // char line_labels[code_size][10];
  // memset(line_labels, 0, sizeof(line_labels));
  // for (int i = 0; i < code_size; ++i) {
  //   snprintf(line_labels[i], 10, "%dL", i);
  //   set_labels[i] = line_labels[i];
  // }

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

  char *strings[128][LABEL_MAX_LEN] = {0};
  int stringi = 0;

  int uli = symbols_count;
  for (int i = 0; i < code_size; i++) {
    if (instruction_to_string(code[i])) {
      switch (instruction_stat(code[i]).arg) {
        case INST_NO_ARGS: break;
        case INST_8BITS_ARG:
          assert(i + 1 < code_size);
          i += 1;
          break;
        case INST_16BITS_ARG:
          assert(i + 2 < code_size);
          if (labels[i + 1] == NULL) {
            int num = (int16_t)(code[i + 1] + (code[i + 2] << 8));
            if (set_labels[i + num]) {
              labels[i + 1] = set_labels[i + num];
            }
          }
          i += 2;
          break;
        case INST_LABEL_ARG:
          assert(i + 2 < code_size);
          if (labels[i + 1] == NULL) {
            int num = (int16_t)(code[i + 1] + (code[i + 2] << 8));
            assert(i + num >= 0 && i + num < code_size);
            if (set_labels[i + num]) {
              labels[i + 1] = set_labels[i + num];
            }
          }
          i += 2;
          break;
        case INST_RELLABEL_ARG:
          assert(i + 2 < code_size);
          if (rellabels[i + 1] == NULL) {
            int num = (int16_t)(code[i + 1] + (code[i + 2] << 8));
            assert(i + num >= 0 && i + num < code_size);
            if (set_labels[i + num] == NULL) {
              assert(stringi + 1 < 128);
              snprintf((char *)strings[stringi++], LABEL_MAX_LEN, "%d", uli++);
              rellabels[i + 1] = (char *)strings[stringi - 1];
              set_labels[i + num + 1] = (char *)strings[stringi - 1];
            } else {
              rellabels[i + 1] = set_labels[i + num];
            }
          }
          i += 2;
          break;
      }
    }
  }

  for (int i = 0; i < code_size; ++i) {
    if (set_labels[i] != NULL) {
      printf(">%d %s %d\n", i, set_labels[i], code[i]);
    }
  }

  bytecode_t bcs[code_size];
  uint16_t bc_count = 0;

  printf("DISASSEMBLE:\n");
  for (int i = 0; i < code_size;) {
    if (set_labels[i] != NULL) {
      bcs[bc_count++] = bytecode_with_string(BSETLABEL, 0, set_labels[i]);

      if (code[i] == 0 && i + 1 < code_size && code[i + 1] == 0) {
        int j = i + 1;
        while (j < code_size && code[j] == 0) {
          j++;
        }
        bcs[bc_count++] = (bytecode_t){BDB, 0, {.num = j - i}};
        i = j;
        continue;
      }
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
          if (labels[i + 1]) {
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
      while (j < code_size && code[j] != 0 && code[j] != '\n') {
        j++;
      }
      bcs[bc_count++] = bytecode_with_sv(BSTRING, 0, (sv_t){(char *)(code + i), j - i});
      if (code[j] == '\n') {
        bcs[bc_count++] = (bytecode_t){BHEX, 0, {.num = '\n'}};
        j++;
      }
      assert(code[j] == 0);
      bcs[bc_count++] = (bytecode_t){BHEX, 0, {.num = 0}};
      i = j + 1;
    } else {
      bcs[bc_count++] = (bytecode_t){BHEX, 0, {.num = code[i]}};
      i++;
    }
  }

  for (int i = 0; i < bc_count; ++i) {
    bytecode_to_asm(stdout, bcs[i]);
  }
}
