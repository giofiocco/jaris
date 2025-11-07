#ifndef FILES_H__
#define FILES_H__

#include <stdint.h>

#include "instructions.h"

#define GLOBAL_MAX_COUNT       64
#define EXTERN_MAX_COUNT       32
#define RELOC_MAX_COUNT        128
#define INTERN_RELOC_MAX_COUNT 128
#define DYNAMIC_MAX_COUNT      2
#define SYMBOL_MAX_COUNT       128
#define FILE_NAME_MAX_LEN      64
#define MAGIC_NUMBER_MAX_LEN   (4 + 1)

typedef struct {
  uint16_t where;
  uint16_t what;
} reloc_entry_t;

typedef struct {
  char file_name[FILE_NAME_MAX_LEN];
  uint8_t reloc_count;
  reloc_entry_t relocs[INTERN_RELOC_MAX_COUNT];
} dynamic_entry_t;

// TODO: image as char*
typedef struct {
  char image[LABEL_MAX_LEN];
  uint16_t pos;
  uint16_t relocs[INTERN_RELOC_MAX_COUNT];
  uint8_t reloc_count;
  uint16_t relrelocs[INTERN_RELOC_MAX_COUNT];
  uint8_t relreloc_count;
} symbol_t;

typedef struct {
  uint16_t code_size;
  uint8_t code[1 << 16];
  uint16_t reloc_count;
  reloc_entry_t relocs[RELOC_MAX_COUNT];
  uint8_t global_count;
  uint16_t globals[GLOBAL_MAX_COUNT];
  uint8_t extern_count;
  uint16_t externs[EXTERN_MAX_COUNT];
  uint16_t symbol_count;
  symbol_t symbols[SYMBOL_MAX_COUNT];
} obj_t;

typedef struct {
  uint16_t code_size;
  uint8_t code[1 << 16];
  uint16_t reloc_count;
  reloc_entry_t relocs[RELOC_MAX_COUNT];
  uint8_t global_count;
  uint16_t globals[GLOBAL_MAX_COUNT];
  uint16_t symbol_count;
  symbol_t symbols[SYMBOL_MAX_COUNT];
} so_t;

typedef struct {
  uint16_t code_size;
  uint8_t code[1 << 16];
  uint16_t reloc_count;
  reloc_entry_t relocs[RELOC_MAX_COUNT];
  int dynamic_count;
  dynamic_entry_t dynamics[DYNAMIC_MAX_COUNT];
  uint16_t symbol_count;
  symbol_t symbols[SYMBOL_MAX_COUNT];
} exe_t;

typedef enum {
  F_NONE,
  F_ASM,
  F_OBJ,
  F_EXE,
  F_SO,
  F_MEM,
  F_BIN,
  F_FONT,
} file_kind_t;

void print_file_kind_list();
char *file_kind_to_string(file_kind_t kind);
file_kind_t parse_argument_file_kind(char *arg);
file_kind_t file_deduce_kind(char *filename);

void symbol_list_dump(symbol_t *symbols, uint16_t count);
void symbols_list_decode(symbol_t *symbols, uint16_t *count, FILE *file);
void symbols_list_encode(symbol_t *symbols, uint16_t count, FILE *file);
symbol_t *symbols_list_find(symbol_t *symbols, uint16_t count, char *image);

void obj_dump(obj_t *obj);
obj_t obj_decode(FILE *file);
obj_t obj_decode_file(char *filename);
void obj_encode_file(obj_t *obj, char *filename);

void exe_dump(exe_t *exe);
exe_t exe_decode(FILE *file);
exe_t exe_decode_file(char *filename);
void exe_encode_file(exe_t *exe, char *filename);

void bin_encode_file(exe_t *exe, char *filename);

void so_dump(so_t *so);
so_t so_decode(FILE *file);
so_t so_decode_file(char *filename);
void so_encode_file(so_t *so, char *filename);

void mem_sector_dump(uint8_t *sector);
uint16_t mem_sector_find_entry(uint8_t *sector, char *entry);

// bytecode list to free
bytecode_t *disassemble(uint8_t *code, uint16_t code_size, symbol_t *symbols, uint16_t symbols_count, int *out_bytecode_count);

#endif // FILES_H__
