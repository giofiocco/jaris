#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arg_parser.h/arg_parser.h"
#include "errors.h"
#include "files.h"

int main(int argc, char **argv) {
  char *kind = NULL;

  arg_parser_t parser = {0};
  parser.program_name = "inspect";
  arg_parser_allow_unflag(&parser, "file");
  arg_parser_add_arg(&parser, (arg_t){ARG_STRING, "kind", 'k', "kind [obj, exe or mem]", &kind});
  arg_parser_parse(&parser, argc, argv);

  if (parser.unflagi != 1) {
    fprintf(stderr, "ERROR: expected one file\n");
    arg_parser_print_help(&parser);
    exit(1);
  }

  char *filename = parser.unflag[0];

  if (strcmp(kind, "obj") == 0) {
    sv_allocator_t alloc = {0};
    obj_t obj = obj_decode_file(filename, &alloc);
    obj_dump(&obj);

  } else if (strcmp(kind, "exe") == 0) {
    exe_t exe = exe_decode_file(filename);
    exe_dump(&exe);

  } else if (strcmp(kind, "mem") == 0) {
    uint8_t sectors[2048][256] = {0};

    FILE *file = fopen(filename, "rb");
    if (!file) {
      eprintf("cannot open file '%s': '%s'", filename, strerror(errno));
    }
    assert(fread(sectors, 1, 1 << 19, file) == 1 << 19);
    assert(fclose(file) == 0);

    for (int i = 0; sectors[i][0] != 0; ++i) {
      uint8_t *sector = sectors[i];
      printf("%d ", i);
      if (sector[0] == 'D') {
        printf("DIR:\n");
        printf("\tNEXT: %d\n", sector[1] | (sector[2] << 8));
        printf("\tENTRIES:\n");
        int a = 3;
        while (sector[a] != 0) {
          int len = strlen((char *)(sector + a));
          printf("\t\t%s %d\n", sector + a, sector[a + len + 1] | (sector[a + len + 2] << 8));
          a += len + 3;
        }
      } else if (sector[0] == 'F') {
        printf("FILE:\n");
        printf("\tNEXT: %d\n", sector[1] | (sector[2] << 8));
        printf("\tMAX INDEX: %d\n", sector[3]);
      } else {
        assert(0);
      }
    }

  } else {
    assert(0);
  }

  return 0;
}
