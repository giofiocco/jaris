#ifndef FILES_H__
#define FILES_H__

#include <stdint.h>

#include "instructions.h"

#define GLOBAL_MAX_COUNT       32
#define EXTERN_MAX_COUNT       32
#define RELOC_MAX_COUNT        128
#define INTERN_RELOC_MAX_COUNT 128
#define DYNAMIC_MAX_COUNT      4
#define SYMBOL_MAX_COUNT       256
#define FILE_NAME_MAX_LEN      64

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

typedef struct {
  exe_t exe;
  uint8_t global_count;
  uint16_t globals[GLOBAL_MAX_COUNT];
  uint8_t extern_count;
  uint16_t externs[EXTERN_MAX_COUNT];
  int so_count;
  char so_names[DYNAMIC_MAX_COUNT][FILE_NAME_MAX_LEN];
  int so_global_counts[DYNAMIC_MAX_COUNT];
  char so_gloabals_images[DYNAMIC_MAX_COUNT][GLOBAL_MAX_COUNT][LABEL_MAX_LEN];
  uint16_t so_globals_pos[DYNAMIC_MAX_COUNT][GLOBAL_MAX_COUNT];
} exe_state_t;

uint16_t encode_char(char c);
char decode_char(uint16_t code);

void symbol_list_dump(symbol_t *symbols, uint16_t count);
void symbols_list_decode(symbol_t *symbols, uint16_t *count, FILE *file);
void symbols_list_encode(symbol_t *symbols, uint16_t count, FILE *file);

void obj_dump(obj_t *obj);
uint16_t obj_add_symbol(obj_t *obj, char *name, uint16_t pos);
symbol_t *obj_find_symbol(obj_t *obj, char *name);
uint16_t obj_find_symbol_pos(obj_t *obj, char *name);
void obj_add_symbol_reloc(obj_t *obj, char *name, uint16_t where);
void obj_add_instruction(obj_t *obj, instruction_t inst);
void obj_add_hex(obj_t *obj, uint8_t num);
void obj_add_hex2(obj_t *obj, uint16_t num);
void obj_compile_bytecode(obj_t *obj, bytecode_t bc);
void obj_check(obj_t *obj, int debug_info);

obj_t obj_decode_file(char *filename);
void obj_encode_file(obj_t *obj, char *filename);

void exe_dump(exe_t *exe);
void exe_add_symbol_offset(exe_t *exe, symbol_t *s, uint16_t offset);

uint16_t exe_state_find_global(exe_state_t *state, char *name);
uint16_t exe_state_find_so_global(exe_state_t *state, int so_index, char *name);
void exe_state_check_exe(exe_state_t *state);

exe_t exe_decode_file(char *filename);
void exe_encode_file(exe_t *exe, char *filename);

void bin_encode_file(exe_t *exe, char *filename);

void so_dump(so_t *so);
so_t so_decode_file(char *filename);
void so_encode_file(so_t *so, char *filename);
so_t so_from_exe_state(exe_state_t *exes);

#endif // FILES_H__
