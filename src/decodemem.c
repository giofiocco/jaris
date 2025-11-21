#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <sys/stat.h>

#define ERRORS_IMPLEMENTATION
#include "../argparse.h"
#include "../mystb/errors.h"

#define SECTOR_SIZE  256
#define SECTOR_COUNT 2048

uint8_t SECTORS[SECTOR_COUNT][SECTOR_SIZE] = {0};

int strleq(char *a, char *b, int len) {
  assert(a);
  assert(b);
  for (int i = 0; i < len; ++i) {
    if (a[i] == 0 && b[i] == 0) {
      return 1;
    }
    if (a[i] == 0 || b[i] == 0) {
      return 0;
    }
    if (a[i] != b[i]) {
      return 0;
    }
  }
  return 1;
}

uint16_t solve_path(char *path) {
  if (!path) {
    return 1;
  }

  uint16_t sec = 1;
  while (*path) {
    char *sector = (char *)SECTORS[sec] + 3;
    char *c = strchrnul(path, '/');
    int len = c - path;
    while (!strleq(sector, path, len)) {
      sector = strchrnul(sector, '\0') + 3;
      if (*sector == 0) {
        return 0;
      }
    }
    sector = strchrnul(sector, '\0') + 1;
    sec = sector[0] | (sector[1] << 8);
    if (*c == 0) {
      break;
    }
    path = c + 1;
  }

  return sec;
}

void decode_file(uint16_t sec, char *outpath) {
  int is_stdout = !outpath || strcmp(outpath, "-") == 0;
  FILE *file = is_stdout ? stdout : fopen(outpath, "wb");
  if (!file) {
    error_fopen(outpath);
  }

  do {
    assert(fwrite(&SECTORS[sec][4], 1, SECTORS[sec][3] - 3, file) == (size_t)SECTORS[sec][3] - 3);
    sec = SECTORS[sec][1] | (SECTORS[sec][2] << 8);
  } while (sec != 0xFFFF);

  if (!is_stdout) {
    assert(fclose(file) == 0);
  }
}

void decode(uint16_t sec, char *outpath);
void decode_dir(uint16_t sec, char *outpath) {
  assert(outpath);
  assert(sec && sec != 0xFFFF);

  if (mkdir(outpath, 0755) != 0) {
    eprintf("cannot create dir %s: %s", outpath, strerror(errno));
  }

  do {
    char *sector = (char *)SECTORS[sec] + 12;
    while (*sector) {
      char *c = strchrnul(sector, '\0');
      if (c[1] == 0) {
        break;
      }
      uint16_t entry_sec = c[1] | (c[2] << 8);
      char path[SECTOR_SIZE] = {0};
      snprintf(path, SECTOR_SIZE, "%s/%s", outpath, sector);
      decode(entry_sec, path);
      sector = c + 3;
    }
    sec = SECTORS[sec][1] | (SECTORS[sec][2] << 8);
  } while (sec != 0xFFFF);
}

void decode(uint16_t sec, char *outpath) {
  if (SECTORS[sec][0] == 'D') {
    if (!outpath) {
      eprintf("output path not provided to decode a dir");
    }
    decode_dir(sec, outpath);
  } else if (SECTORS[sec][0] == 'F') {
    decode_file(sec, outpath);
  } else {
    assert(0);
  }
}

void print_help() {
  printf("Usage: decodemem [options] [path] <mempath>\n"
         "  decode the path inside the mem.bin file\n"
         "  if no path provided the whole root is decoded\n"
         "  a directory is decoded recursiely\n"
         "\n"
         "Options:\n"
         "  -o | --output <path>  the path of the output [default: decoded.out]\n"
         "  -h | --help           show help message\n");
}

int main(int argc, char **argv) {
  char *path = NULL;
  char *mempath = NULL;
  char *outpath = NULL;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                           //
    ARG_PARSE_STRING_ARG("o", "output", outpath) //
        else {
      if (path && mempath) {
        ARG_ERROR_("too many positional args");
      } else if (path) {
        mempath = *argv;
      } else {
        path = *argv;
      }
    }
  }

  if (path && !mempath) {
    mempath = path;
    path = NULL;
  }

  if (!mempath) {
    ARG_ERROR_("no mempath provided")
  }

  FILE *mem = fopen(mempath, "rb");
  if (!mem) {
    error_fopen(mempath);
  }
  assert(fread(SECTORS, 1, 1 << 19, mem) == 1 << 19);
  assert(fclose(mem) == 0);

  uint16_t sec = solve_path(path);
  if (!sec) {
    eprintf("path not found: '%s'", path);
  }

  decode(sec, outpath);

  return 0;
}
