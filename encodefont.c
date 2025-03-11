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
  assert(putc('A', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('B', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('C', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('D', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('E', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('F', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('G', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01011100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('H', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('I', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('J', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b01100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('K', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('L', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('M', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01101100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('N', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01100100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01001100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('O', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('P', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('Q', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00110100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('R', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01010000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('S', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('T', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('U', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('V', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('W', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('X', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('Y', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('Z', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('0', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('1', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b01010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('2', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('3', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('4', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00001100, file) != -1);
  assert(putc(0b00010100, file) != -1);
  assert(putc(0b00100100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01111110, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('5', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('6', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('7', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('8', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('9', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('!', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('"', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('#', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00100100, file) != -1);
  assert(putc(0b01111110, file) != -1);
  assert(putc(0b00100100, file) != -1);
  assert(putc(0b00100100, file) != -1);
  assert(putc(0b01111110, file) != -1);
  assert(putc(0b00100100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('$', file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0b01010000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b00010100, file) != -1);
  assert(putc(0b00010100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b00010000, file) != -1);

  ++count;
  assert(putc('%', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01100010, file) != -1);
  assert(putc(0b01100100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00101100, file) != -1);
  assert(putc(0b01001100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('&', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00110100, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00110100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('\'', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('(', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc(')', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('*', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('+', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc(',', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);

  ++count;
  assert(putc('-', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('.', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('/', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc(':', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc(';', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);

  ++count;
  assert(putc('<', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('=', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('>', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('?', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('@', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01011100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01010000, file) != -1);
  assert(putc(0b00111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('[', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01100000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('\\', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc(']', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00001100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00001100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('^', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('_', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01111100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('`', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('{', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00010000, file) != -1);

  ++count;
  assert(putc('|', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('}', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00000100, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00010000, file) != -1);

  ++count;
  assert(putc('~', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00101000, file) != -1);
  assert(putc(0b01010000, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('a', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('b', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('c', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('d', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('e', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('f', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00011000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('g', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b01110000, file) != -1);

  ++count;
  assert(putc('h', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('i', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('j', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);

  ++count;
  assert(putc('k', file) != -1);
  assert(putc(0, file) != -1);
  // assert(putc(0b01000000, file) != -1);
  // assert(putc(0b01001000, file) != -1);
  // assert(putc(0b01110000, file) != -1);
  // assert(putc(0b01001000, file) != -1);
  // assert(putc(0b01001000, file) != -1);
  // assert(putc(0b01001000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('l', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('m', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01101000, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('n', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('o', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('p', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);

  ++count;
  assert(putc('q', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00001000, file) != -1);

  ++count;
  assert(putc('r', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('s', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b01000000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('t', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b00011000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('u', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('v', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01110000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('w', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01000100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01010100, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('x', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b00110000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0, file) != -1);

  ++count;
  assert(putc('y', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b01001000, file) != -1);
  assert(putc(0b00111000, file) != -1);
  assert(putc(0b00001000, file) != -1);
  assert(putc(0b00110000, file) != -1);

  ++count;
  assert(putc('z', file) != -1);
  assert(putc(0, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b00000000, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0b00010000, file) != -1);
  assert(putc(0b00100000, file) != -1);
  assert(putc(0b01111000, file) != -1);
  assert(putc(0, file) != -1);

  rewind(file);
  assert(fwrite(&count, 2, 1, file) == 1);

  assert(fclose(file) == 0);

  return 0;
}
