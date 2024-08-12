#ifndef FILES_H__
#define FILES_H__

#include <stdint.h>

#include "sv.h"

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

#endif // FILES_H__
