#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SECTOR_COUNT 2048
#define SECTOR_SIZE  256

uint8_t SECTORS[SECTOR_COUNT][SECTOR_SIZE] = {0};

char *dir_name = NULL;

void decode_entry(char *name, uint16_t ptr) {
  // printf("%s %d\n", name, ptr);
  if (strcmp(name, "__os") == 0) {
    for (int i = 4; i <= SECTORS[ptr][3]; ++i) {
      printf("%c", SECTORS[ptr][i]);
    }
    int next = (SECTORS[ptr][2] << 8) | SECTORS[ptr][1];
    if (next != 0xFFFF) {
      decode_entry(name, next);
    }
  }
}

int main(int argc, char **argv) {
  assert(argc == 2 && "expected the path of the directory for decoding");
  dir_name = argv[1];

  FILE *file = fopen("mem.bin", "rb");
  assert(file);
  assert(fread(SECTORS, 1, sizeof(SECTORS), file) == sizeof(SECTORS));

  for (int i = 3; i < SECTOR_SIZE;) {
    if (SECTORS[1][i] == 0) {
      break;
    }
    int size = strlen((char *)SECTORS[1] + i);
    decode_entry((char *)SECTORS[1] + i, (SECTORS[1][i + size + 2] << 8) | SECTORS[1][i + size + 1]);
    i += size + 3;
  }

  assert(fclose(file) == 0);
}
