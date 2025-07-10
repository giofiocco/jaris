#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse/argparse.h"

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
      TODO; // subsector
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

char *sep = " \n";

uint16_t encode_dir_from_file(char *buffer, uint16_t parent, uint16_t head) {
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

  for (char *token = strtok(buffer, sep); token; token = strtok(NULL, sep)) {
    if (strcmp(token, "newfile") == 0) {
      char *name = strtok(NULL, sep);
      assert(name);

      assert(sector + strlen(name) + 1 + 3 - sector_start < SECTOR_SIZE);
      sector_push(&sector, name, strlen(name));
      sector++;

      uint16_t ptr = 0;

      token = strtok(NULL, sep);
      if (strcmp(token, "fromfile") == 0) {
        char *path = strtok(NULL, sep);
        assert(path);

        FILE *file = fopen(path, "rb");
        if (!file) {
          fprintf(stderr, "ERROR: cannot open file: '%s'\n", path);
          exit(1);
        }
        ptr = encode_file(file, 0);
        assert(fclose(file) == 0);

      } else if (strcmp(token, "text") == 0) {
        char *text = token + strlen(token) + 1;
        char *line = NULL;
        while ((line = strtok(NULL, "\n"))) {
          if (strcmp(line, "endtext") == 0) {
            break;
          }
          line[strlen(line)] = '\n';
        }
        *(line - 1) = 0;

        int size = strlen(text);

        ptr = SECTORI;
        uint8_t *sector = SECTORS[SECTORI++];

        assert(size + 4 < SECTOR_SIZE);
        sector_push_u8(&sector, 'F');
        sector_push_u16(&sector, 0xFFFF);
        sector_push_u8(&sector, size + 3);
        sector_push(&sector, text, size);

      } else {
        printf("TODO at %d: '%s'\n", __LINE__, token);
        exit(1);
      }
      assert(ptr);

      sector_push_u16(&sector, ptr);

      if (strcmp(name, "__os") == 0) {
        os_sec = ptr;
      } else if (strcmp(name, "__stdlib") == 0) {
        stdlib_sec = ptr;
      }

    } else if (strcmp(token, "newdir") == 0) {
      char *name = strtok(NULL, sep);
      assert(name);
      int name_size = strlen(name);

      assert(sector + name_size + 1 + 3 - sector_start < SECTOR_SIZE);
      sector_push(&sector, name, name_size);
      sector++;

      // TODO: dir from dir
      uint16_t ptr = encode_dir_from_file(name + name_size + 1, sector_ptr, 0xFFFF);
      assert(ptr);
      sector_push_u16(&sector, ptr);

    } else if (strcmp(token, "enddir") == 0) {
      return sector_ptr;

    } else if (strcmp(token, "setbootloader") == 0) {
      assert(strcmp(strtok(NULL, sep), "fromfile") == 0);
      char *name = strtok(NULL, sep);
      assert(name);
      encode_bootloader(name);

    } else {
      printf("TODO at %d: '%s'\n", __LINE__, token);
      exit(1);
    }
  }

  return 1;
}

void parse_mem_file(char *file_path) {
  assert(file_path);
  FILE *file = fopen(file_path, "r");
  assert(file);
  assert(fseek(file, 0, SEEK_END) == 0);
  unsigned int size = ftell(file);
  rewind(file);
  char buffer[size];
  assert(fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  encode_dir_from_file(buffer, 0xFFFF, 0xFFFF);
}

int main(int argc, char **argv) {
  char *dir_path = NULL;
  char *file_path = NULL;
  char *output_path = NULL;

  struct argparse_option options[] = {
      OPT_GROUP("Options"),
      OPT_HELP(),
      OPT_STRING('d', "dir", &dir_path, "path of dir to encode", NULL, 0, 0),
      OPT_STRING('f', "file", &file_path, "path of file .mem to encode", NULL, 0, 0),
      OPT_STRING('o', "output", &output_path, "path of output file", NULL, 0, 0),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse, options, (const char *const[]){"encodemem [options] file", NULL}, 0);
  argc = argparse_parse(&argparse, argc, (const char **)argv);

  if (!dir_path && !file_path) {
    fprintf(stderr, "ERROR: expected directory path or file path\n");
    argparse_usage(&argparse);
    exit(1);
  }
  if (dir_path && file_path) {
    fprintf(stderr, "ERROR: expected only one of directory path or file path\n");
    argparse_usage(&argparse);
    exit(1);
  }
  if (!output_path) {
    char *filename = dir_path ? dir_path : file_path;
    int len = strlen(filename);
    output_path = calloc(len + 5, sizeof(char));
    snprintf(output_path, len + 5, "%s.bin", filename);
  }

  if (dir_path) {
    encode_dir(dir_path, 0xFFFF, 0xFFFF);

    char bootloader_path[1024] = {0};
    snprintf(bootloader_path, 1024, "%s/__bootloader", dir_path);
    encode_bootloader(bootloader_path);
  } else {
    parse_mem_file(file_path);
  }

  FILE *file = fopen(output_path, "wb");
  assert(file);
  assert(fwrite(SECTORS, 1, sizeof(SECTORS), file) == sizeof(SECTORS));
  assert(fclose(file) == 0);

  return 0;
}
