#ifndef FILES_H__
#define FILES_H__

#include <stdint.h>

#include "instructions.h"

#define GLOBAL_COUNT       256
#define EXTERN_COUNT       256
#define RELOC_COUNT        1024
#define DYNAMIC_COUNT      16
#define LABEL_COUNT        256
#define INTERN_RELOC_COUNT 256
#define SYMBOL_COUNT       256

typedef struct {
  uint16_t where;
  uint16_t what;
} reloc_entry_t;

typedef struct {
  char name[LABEL_MAX_LEN];
  uint16_t pos;
} global_entry_t;

typedef struct {
  char name[LABEL_MAX_LEN];
  uint8_t pos_num;
  uint16_t pos[256];
} extern_entry_t;

typedef struct {
  char file_name[LABEL_MAX_LEN];
  uint16_t reloc_num;
  reloc_entry_t reloc_table[RELOC_COUNT];
} dynamic_entry_t;

typedef struct {
  char image[LABEL_MAX_LEN];
  uint16_t pos;
  uint16_t relocs[RELOC_COUNT];
  uint8_t reloc_num;
  uint16_t relrelocs[RELOC_COUNT];
  uint8_t relreloc_num;
} symbol_t;

typedef struct {
  uint16_t global_num;
  global_entry_t globals[GLOBAL_COUNT];
  uint16_t extern_num;
  extern_entry_t externs[EXTERN_COUNT];
  uint16_t reloc_num;
  reloc_entry_t reloc_table[RELOC_COUNT];
  uint16_t code_size;
  uint8_t code[1 << 16];
  symbol_t symbols[SYMBOL_COUNT];
  uint16_t symbol_num;
} obj_t;

typedef struct {
  uint16_t code_size;
  uint8_t code[1 << 16];
  uint16_t reloc_num;
  reloc_entry_t reloc_table[RELOC_COUNT];
  int dynamic_num;
  dynamic_entry_t dynamics_table[DYNAMIC_COUNT];
} exe_t;

typedef struct {
  uint8_t global_num;
  global_entry_t globals[GLOBAL_COUNT];
  uint16_t code_size;
  uint8_t code[1 << 16];
  uint16_t reloc_num;
  reloc_entry_t reloc_table[RELOC_COUNT];
} so_t;

typedef struct {
  char image[LABEL_MAX_LEN];
  uint16_t pos;
} label_t;

typedef struct {
  exe_t exe;
  uint16_t global_num;
  global_entry_t globals[GLOBAL_COUNT];
  uint16_t extern_num;
  extern_entry_t externs[EXTERN_COUNT];
  int so_num;
  so_t sos[DYNAMIC_COUNT];
  char *so_names[DYNAMIC_COUNT];
} exe_state_t;

void extern_entry_add_pos(extern_entry_t *e, uint16_t pos);

void symbol_dump(symbol_t *symbol);

void obj_dump(obj_t *obj);
void obj_add_symbol(obj_t *obj, char *name, uint16_t pos);
symbol_t *obj_find_symbol(obj_t *obj, char *name);
uint16_t obj_find_symbol_pos(obj_t *obj, char *name);
void obj_add_symbol_reloc(obj_t *obj, char *name, uint16_t where);
void obj_add_reloc(obj_t *obj, uint16_t where, uint16_t what);
void obj_add_global(obj_t *obj, char *image);
void obj_add_extern(obj_t *obj, char *image);
extern_entry_t *obj_find_extern(obj_t *obj, char *image);
void obj_add_instruction(obj_t *obj, instruction_t inst);
void obj_add_hex(obj_t *obj, uint8_t num);
void obj_add_hex2(obj_t *obj, uint16_t num);
void obj_compile_bytecode(obj_t *obj, bytecode_t bc);
void obj_check(obj_t *obj);

obj_t obj_decode_file(char *filename);
void obj_encode_file(obj_t *obj, char *filename, int debug_info);

void exe_dump(exe_t *exe);

uint16_t exe_state_find_global(exe_state_t *exes, char *name);
void exe_state_add_global(exe_state_t *exes, global_entry_t global, uint16_t offset);
void exe_state_add_extern(exe_state_t *exes, extern_entry_t extern_, uint16_t offset);
void exe_state_check_exe(exe_state_t *exes);

void exe_add_reloc_offset(exe_t *exe, reloc_entry_t reloc, uint16_t offset);
void exe_add_reloc(exe_t *exe, reloc_entry_t reloc);

exe_t exe_decode_file(char *filename);
void exe_encode_file(exe_t *exe, char *filename);

void bin_encode_file(exe_t *exe, char *filename);

void so_dump(so_t *so);
so_t so_decode_file(char *filename);
void so_encode_file(exe_state_t *exes, char *filename);
uint16_t so_find_global(so_t *so, char *name);

#endif // FILES_H__
