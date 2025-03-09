#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("ERROR: expected output file path\n");
    exit(1);
  }

  FILE *file = fopen(argv[1], "wb");
  if (!file) {
    printf("ERROR: cannot open file '%s': '%s'", argv[1], strerror(errno));
    exit(1);
  }

  uint16_t count = 0;
  assert(fwrite(&count, 2, 1, file) == 1);

  ++count;
  assert(putc('a', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('b', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('c', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('d', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('e', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('f', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('g', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01011100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('h', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('i', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('j', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b01100000, file) != -1);
  assert(putc(0, file) != -1);

  rewind(file);
  assert(fwrite(&count, 2, 1, file) == 1);

  assert(fclose(file) == 0);

  return 0;
}
