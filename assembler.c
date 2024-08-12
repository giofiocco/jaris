#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "assemble.h"
#include "errors.h"

int main() {
  char *filename = "bash.asm";
  FILE *file = fopen(filename, "r");
  if (!file) {
    eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  int size = ftell(file);
  assert(size > 0);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size];
  assert((long int)fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  assemble(buffer, filename, 0);

  return 0;
}
