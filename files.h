#ifndef FILES_H__
#define FILES_H__

#include <stdint.h>

#include "instructions.h"

typedef struct {
  uint16_t where;
  uint16_t what;
} reloc_entry_t;

typedef struct {
  sv_t name;
  uint16_t pos;
} global_entry_t;

typedef struct {
  sv_t name;
  uint8_t pos_num;
  uint16_t pos[256];
} extern_entry_t;

#define GLOBAL_COUNT 256
#define EXTERN_COUNT 256
#define RELOC_COUNT 1024

typedef struct {
  uint16_t global_num;
  global_entry_t globals[GLOBAL_COUNT];
  uint16_t extern_num;
  extern_entry_t externs[EXTERN_COUNT];
  uint16_t reloc_num;
  reloc_entry_t reloc_table[RELOC_COUNT];
  uint16_t code_size;
  uint8_t code[1 << 16];
} obj_t;

typedef struct {
  uint16_t code_size;
  uint8_t code[1 << 16];
  uint16_t reloc_num;
  reloc_entry_t reloc_table[RELOC_COUNT];
} exe_t;

#define SYMBOL_COUNT 256
#define INTERN_RELOC_COUNT 256

typedef struct {
  sv_t image;
  uint16_t pos;
} symbol_t;

typedef struct {
  obj_t obj;
  symbol_t symbols[SYMBOL_COUNT];
  int symbol_num;
  symbol_t relocs[INTERN_RELOC_COUNT];
  int reloc_num;
 symbol_t relrelocs[RELOC_COUNT];
  int relreloc_num;
} obj_state_t;

typedef struct {
  exe_t exe;
  uint16_t global_num;
  global_entry_t globals[GLOBAL_COUNT];
  uint16_t extern_num;
  extern_entry_t externs[EXTERN_COUNT];
} exe_state_t;

void obj_dump(obj_t *obj);

void obj_state_add_symbol(obj_state_t *objs, sv_t sv, uint16_t pos);
uint16_t obj_state_find_symbol(obj_state_t *objs, sv_t sv);
void obj_state_add_reloc(obj_state_t *objs, sv_t sv, uint16_t pos);
void obj_state_add_relreloc(obj_state_t *objs, sv_t sv, uint16_t pos);
void obj_state_check_obj(obj_state_t *objs);
void obj_compile_bytecode(obj_state_t *objs, bytecode_t bc);

void obj_add_reloc(obj_t *obj, uint16_t where, uint16_t what);
void obj_add_global(obj_t *obj, sv_t image);
void obj_add_instruction(obj_t *obj, instruction_t inst);
void obj_add_hex(obj_t *obj, uint8_t num);
void obj_add_hex2(obj_t *obj, uint16_t num);

obj_t obj_decode_file(char *filename, sv_allocator_t *alloc);
void obj_encode_file(obj_t *obj, char *filename);

uint16_t exe_state_find_global(exe_state_t *exes,  sv_t name);
void exe_state_add_global(exe_state_t *exes, global_entry_t global, uint16_t offset);
void exe_state_add_extern(exe_state_t *exes, extern_entry_t extern_, uint16_t offset);
void exe_state_check_exe(exe_state_t *exes);

void exe_add_reloc_offset(exe_t *exe, reloc_entry_t reloc, uint16_t offset);
void exe_add_reloc(exe_t *exe, reloc_entry_t reloc);

#endif // FILES_H__
