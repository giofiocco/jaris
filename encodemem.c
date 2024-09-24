#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TODO assert(0 && "TO IMPLEMENT")

#define SECTOR_COUNT 2048
#define SECTOR_SIZE  256

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

  if (size + 4 > SECTOR_SIZE) {
    assert(fread(sector_start + 4, 1, SECTOR_SIZE - 4, file) == SECTOR_SIZE - 4);
    uint16_t ptr = encode_file(file, offset + SECTOR_SIZE - 4);
    sector_push_u16(&sector, ptr);
    sector_push_u8(&sector, 0xFF);
  } else {
    assert(fread(sector_start + 4, 1, size, file) == size);
    sector_push_u16(&sector, 0xFFFF);
    sector_push_u8(&sector, size + 4);
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
    fprintf(stderr, "ERROR: cannot open directory '%s': %s\n", path, strerror(errno));
    exit(1);
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
      TODO;  // subsector
    }

    uint16_t ptr = 0;
    if (d->d_type == DT_DIR) {
      ptr = encode_dir(full_path, head, 0xFFFF);
    } else if (d->d_type == DT_REG) {
      FILE *file = fopen(full_path, "rb");
      if (!file) {
        fprintf(stderr, "ERROR: cannot open file '%s': %s\n", full_path, strerror(errno));
        exit(1);
      }

      ptr = encode_file(file, 0);

      if (strcmp(full_path, "mem/__os") == 0) {
        os_sec = ptr;
      } else if (strcmp(full_path, "mem/__stdlib") == 0) {
        stdlib_sec = ptr;
      }

      assert(fclose(file) == 0);
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
    fprintf(stderr, "ERROR: cannot open file '%s': %s\n", filename, strerror(errno));
    exit(1);
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (size > SECTOR_SIZE - 4) {
    fprintf(stderr, "ERROR: bootloader too big: %ld\n", size);
    exit(1);
  }
  if (os_sec == 0) {
    fprintf(stderr, "ERROR: __os not found in root\n");
    exit(1);
  }
  if (stdlib_sec == 0) {
    fprintf(stderr, "ERROR: __stdlib not found in root\n");
    exit(1);
  }

  assert(fread(SECTORS[0], 1, size, file) == size);
  uint8_t *sector = &SECTORS[0][SECTOR_SIZE - 4];
  sector_push_u16(&sector, os_sec);
  sector_push_u16(&sector, stdlib_sec);

  ++SECTORI;

  assert(fclose(file) == 0);
}

int main() {
  encode_dir("mem", 0xFFFF, 0xFFFF);
  encode_bootloader("mem/__bootloader");

  FILE *file = fopen("mem.bin", "wb");
  assert(file);
  assert(fwrite(SECTORS, 1, sizeof(SECTORS), file) == sizeof(SECTORS));
  assert(fclose(file) == 0);

  return 0;
}
