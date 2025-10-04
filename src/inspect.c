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

void print_disassemble(uint8_t *code, uint16_t code_size, symbol_t *symbols, uint16_t symbols_count) {
  int count = 0;
  bytecode_t *bcs = disassemble(code, code_size, symbols, symbols_count, &count);
  printf("DISASSEMBLE:\n");
  for (int i = 0; i < count; ++i) {
    if (bcs[i].kind == BSETLABEL) {
      printf("\n");
      bytecode_to_asm(stdout, bcs[i]);
      printf("\n\t");
    } else {
      bytecode_to_asm(stdout, bcs[i]);
      printf(" ");
    }
  }
  printf("\n");
}

void inspect_obj(char *filename, int disassemble_flag) {
  assert(filename);

  obj_t obj = obj_decode_file(filename);
  obj_dump(&obj);
  if (disassemble_flag) {
    print_disassemble(obj.code, obj.code_size, obj.symbols, obj.symbol_count);
  }
}

void inspect_exe(char *filename, int disassemble_flag) {
  assert(filename);

  exe_t exe = exe_decode_file(filename);
  exe_dump(&exe);
  if (disassemble_flag) {
    print_disassemble(exe.code, exe.code_size, exe.symbols, exe.symbol_count);
  }
}
void inspect_so(char *filename, int disassemble_flag) {
  assert(filename);

  so_t so = so_decode_file(filename);
  so_dump(&so);
  if (disassemble_flag) {
    print_disassemble(so.code, so.code_size, so.symbols, so.symbol_count);
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
    print_disassemble(buffer, size, NULL, 0);
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

  for (int i = 1; i < 2048; ++i) {
    if (sectors[i][0] == 0) {
      continue;
    }
    uint8_t *sector = sectors[i];
    printf("%d ", i);
    mem_sector_dump(sector);
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
         "  -h | --help  show help message\n");
  print_file_kind_list();
  printf("\nif input is '-' it read from the stdin\n");
}

int main(int argc, char **argv) {
  char *path = NULL;
  file_kind_t kind = F_NONE;
  int disassemble = 0;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                       //
        else ARG_PARSE_FLAG("d", "disassemble", disassemble) //
        else if (kind == F_NONE && (kind = parse_argument_file_kind(*argv))) {
    }
    else {
      if (path != NULL) {
        eprintf("file already provided: %s", path);
      }
      path = *argv;
    }
  }

  if (!path) {
    eprintf("file not provided");
  }

  if (kind == F_NONE) {
    kind = file_deduce_kind(path);
  }

  switch (kind) {
    case F_NONE: assert(0); break;
    case F_ASM: eprintf("unvalid file kind: asm"); break;
    case F_OBJ: inspect_obj(path, disassemble); break;
    case F_EXE: inspect_exe(path, disassemble); break;
    case F_SO: inspect_so(path, disassemble); break;
    case F_MEM: inspect_mem(path); break;
    case F_BIN: inspect_bin(path, disassemble); break;
    case F_FONT: inspect_font(path); break;
  }

  return 0;
}
