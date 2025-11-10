#define _DEFAULT_SOURCE

#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ERRORS_IMPLEMENTATION
#define SV_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"
#include "../mystb/sv.h"

#define TODO assert(0 && "TO IMPLEMENT")

#define SECTOR_SIZE  256
#define SECTOR_COUNT 2048

uint8_t SECTORS[SECTOR_COUNT][SECTOR_SIZE] = {0};
uint16_t SECTORI = 1;

uint16_t os_sec = 0;
uint16_t stdlib_sec = 0;

void sector_push(uint8_t **sector, void *data, int count) {
  assert(sector);
  assert(*sector);
  assert(data);
  assert(count > 0);

  memcpy(*sector, data, count);
  *sector += count;
}

void sector_push_u8(uint8_t **sector, uint8_t data) {
  assert(sector);
  assert(*sector);

  **sector = data;
  *sector += 1;
}

void sector_push_u16(uint8_t **sector, uint16_t data) {
  assert(sector);
  assert(*sector);

  **sector = data & 0xFF;
  *(*sector + 1) = data >> 8;
  *sector += 2;
}

uint16_t encode_file(FILE *file, int offset) {
  assert(file);

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file) - offset;
  fseek(file, offset, SEEK_SET);

  uint16_t sector_ptr = SECTORI;
  uint8_t *sector = SECTORS[SECTORI++];
  uint8_t *sector_start = sector;

  sector_push_u8(&sector, 'F');

  if (size + 4 >= SECTOR_SIZE) {
    assert(fread(sector_start + 4, 1, SECTOR_SIZE - 4, file) == SECTOR_SIZE - 4);
    uint16_t ptr = encode_file(file, offset + SECTOR_SIZE - 4);
    sector_push_u16(&sector, ptr);
    sector_push_u8(&sector, 0xFF);
  } else {
    assert(fread(sector_start + 4, 1, size, file) == size);
    sector_push_u16(&sector, 0xFFFF);
    sector_push_u8(&sector, size + 3);
  }

  return sector_ptr;
}

uint16_t encode_dir(char *path, uint16_t parent, uint16_t head) {
  assert(path);

  uint16_t sector_ptr = SECTORI;
  uint8_t *sector_start = SECTORS[SECTORI++];
  uint8_t *sector = sector_start;

  if (head == 0xFFFF) {
    head = sector_ptr;
  }

  sector_push_u8(&sector, 'D');
  sector_push_u16(&sector, 0xFFFF);
  sector_push(&sector, "..\0", 3);
  sector_push_u16(&sector, parent);
  sector_push(&sector, ".\0", 2);
  sector_push_u16(&sector, head);

  DIR *dir = opendir(path);
  if (!dir) {
    eprintf("cannot open directory '%s': %s", path, strerror(errno));
  }
  struct dirent *d;
  while ((d = readdir(dir))) {
    if (strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0 || strcmp(d->d_name, "__bootloader") == 0) {
      continue;
    }

    int name_len = strlen(d->d_name);
    sector_push(&sector, d->d_name, name_len);
    sector += 1;

    int full_path_len = strlen(path) + name_len + 2;
    char full_path[full_path_len];
    sprintf(full_path, "%s/%s", path, d->d_name);

    if (sector + name_len + 1 + 3 - sector_start > SECTOR_SIZE) {
      TODO; // subsector
    }

    uint16_t ptr = 0;
    if (d->d_type == DT_DIR) {
      ptr = encode_dir(full_path, head, 0xFFFF);
    } else if (d->d_type == DT_REG) {
      FILE *file = fopen(full_path, "rb");
      if (!file) {
        eprintf("cannot open file '%s': %s", full_path, strerror(errno));
      }

      ptr = encode_file(file, 0);
      assert(fclose(file) == 0);

      if (strcmp(d->d_name, "__os") == 0) {
        os_sec = ptr;
      } else if (strcmp(d->d_name, "__stdlib") == 0) {
        stdlib_sec = ptr;
      }
    } else {
      assert(0);
    }

    assert(ptr != 0);
    sector_push_u16(&sector, ptr);
  }

  return sector_ptr;
}

void encode_bootloader(char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    eprintf("cannot open file '%s': %s", filename, strerror(errno));
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size > SECTOR_SIZE - 4) {
    eprintf("bootloader too big: %ld", size);
  }
  if (os_sec == 0) {
    eprintf("__os not found in root");
  }
  if (stdlib_sec == 0) {
    eprintf("__stdlib not found in root");
  }

  assert(fread(SECTORS[0], 1, size, file) == size);
  uint8_t *sector = &SECTORS[0][SECTOR_SIZE - 4];
  sector_push_u16(&sector, os_sec);
  sector_push_u16(&sector, stdlib_sec);

  ++SECTORI;

  assert(fclose(file) == 0);
}

sv_t next_token(char **_buffer) {
  assert(_buffer);
  assert(*_buffer);

  char *buffer = *_buffer;
  if (buffer[0] == 0) {
    return (sv_t){NULL, 0};
  } else if (isspace(buffer[0])) {
    (*_buffer)++;
    return next_token(_buffer);
  }

  int i = 0;
  while (!isspace(buffer[i])) {
    i++;
  }

  *_buffer += i;
  return (sv_t){buffer, i};
}

uint16_t encode_dir_from_file(char **_buffer, uint16_t parent, uint16_t head) {
  assert(_buffer);
  assert(*_buffer);

  uint16_t sector_ptr = SECTORI;
  uint8_t *sector_start = SECTORS[SECTORI++];
  uint8_t *sector = sector_start;

  if (head == 0xFFFF) {
    head = sector_ptr;
  }

  sector_push_u8(&sector, 'D');
  sector_push_u16(&sector, 0xFFFF);
  sector_push(&sector, "..\0", 3);
  sector_push_u16(&sector, parent);
  sector_push(&sector, ".\0", 2);
  sector_push_u16(&sector, head);

  for (sv_t token; (token = next_token(_buffer)).start != NULL;) {
    if (sv_eq(token, sv_from_cstr("newfile"))
        || sv_eq(token, sv_from_cstr("newos"))
        || sv_eq(token, sv_from_cstr("newstdlib"))) {

      int setos = sv_eq(token, sv_from_cstr("newos"));
      int setstdlib = sv_eq(token, sv_from_cstr("newstdlib"));

      sv_t name = next_token(_buffer);
      assert(name.start);
      assert(sector + name.len + 1 + 3 - sector_start < SECTOR_SIZE);
      sector_push(&sector, name.start, name.len);
      sector++;

      uint16_t ptr = 0;

      token = next_token(_buffer);
      if (sv_eq(token, sv_from_cstr("fromfile"))) {

        sv_t _path = next_token(_buffer);
        char path[_path.len];
        memcpy(path, _path.start, _path.len);
        path[_path.len] = 0;
        FILE *file = fopen(path, "r");
        if (file == NULL) {
          eprintf("cannot open file %s: %s\n", path, strerror(errno));
        }
        ptr = encode_file(file, 0);
        assert(fclose(file) == 0);

      } else if (sv_eq(token, sv_from_cstr("fromtext"))) {

        assert(token.start[token.len] == '\n');

        char *start = token.start + token.len + 1;
        char *nl = start;
        while (!sv_eq((sv_t){nl, 9}, sv_from_cstr("__endtext"))) {
          nl = strchr(nl, '\n') + 1;
        }

        sv_t text = {start, nl - start};

        ptr = SECTORI;
        uint8_t *sector = SECTORS[SECTORI++];

        assert(text.len + 4 < SECTOR_SIZE);
        sector_push_u8(&sector, 'F');
        sector_push_u16(&sector, 0xFFFF);
        sector_push_u8(&sector, text.len + 3);
        if (text.len > 0) {
          sector_push(&sector, text.start, text.len);
        }

        *_buffer = text.start + text.len + 9;

      } else {
        eprintf("cannot parse " SV_FMT, SV_UNPACK(token));
      }

      assert((!setos && !setstdlib) || !(setos && setstdlib));

      if (setos) {
        os_sec = ptr;
      }
      if (setstdlib) {
        stdlib_sec = ptr;
      }

      assert(ptr);
      sector_push_u16(&sector, ptr);

    } else if (sv_eq(token, sv_from_cstr("newdir"))) {

      sv_t name = next_token(_buffer);
      assert(name.start);
      assert(sector + name.len + 1 + 3 - sector_start < SECTOR_SIZE);
      sector_push(&sector, name.start, name.len);
      sector++;

      // TODO: from dir

      uint16_t ptr = encode_dir_from_file(_buffer, sector_ptr, 0xFFFF);
      assert(ptr);
      sector_push_u16(&sector, ptr);

    } else if (sv_eq(token, sv_from_cstr("enddir"))) {
      return sector_ptr;

    } else if (sv_eq(token, sv_from_cstr("setbootloader"))) {
      sv_t _path = next_token(_buffer);
      char path[_path.len];
      memcpy(path, _path.start, _path.len);
      path[_path.len] = 0;

      encode_bootloader(path);

    } else {
      eprintf("cannot parse " SV_FMT, SV_UNPACK(token));
    }
  }

  return 1;
}

void print_help() {
  printf("Usage: encodemem [options] <input>...\n"
         "encode a memory bin file as defined by input file\n"
         "\n"
         "Options:\n"
         "  -o | --output <str>  output file\n"
         "  -d | --dir           interpret <input> as dir\n"
         "  -h | --help          show help message\n");
}

int main(int argc, char **argv) {
  int is_dir = 0;
  char *output = NULL;
  char *path = NULL;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                   //
        else ARG_PARSE_FLAG("d", "dir", is_dir)          //
        else ARG_PARSE_STRING_ARG("o", "output", output) //
        else {
      if (path != NULL) {
        eprintf("input already provided: %s", path);
      }
      path = *argv;
    }
  }

  if (is_dir) {
    eprintf("encodemem for a dir is unimplemented");
  }

  if (!path) {
    ARG_ERROR_("no file provided")
  }

  FILE *file = fopen(path, "r");
  if (file == NULL) {
    eprintf("cannot open file %s: %s\n", path, strerror(errno));
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  unsigned int size = ftell(file);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size];
  assert(fread(buffer, 1, size, file) == size);
  assert(fclose(file) == 0);
  buffer[size] = 0;

  char *_buffer = buffer;
  encode_dir_from_file(&_buffer, 0xFFFF, 0xFFFF);

  uint8_t zero[SECTOR_SIZE] = {0};
  if (!stdlib_sec || !os_sec || memcmp(SECTORS[0], zero, SECTOR_SIZE) == 0) {
    eprintf("bootloader not encoded");
  }

  if (output == NULL) {
    int len = strlen(path);
    output = malloc(len + 4);
    strcpy(output, path);
    output[len] = '.';
    output[len + 1] = 'b';
    output[len + 2] = 'i';
    output[len + 3] = 'n';
    output[len + 4] = 0;
  }

  file = fopen(output, "wb");
  assert(file);
  assert(fwrite(SECTORS, 1, sizeof(SECTORS), file) == sizeof(SECTORS));
  assert(fclose(file) == 0);

  return 0;
}
