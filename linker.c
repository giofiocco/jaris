#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "files.h"
#include "instructions.h"
#include "mystb/arg_parser.h"

extern exe_t link(obj_t *objs_list, int objs_count);

int main(int argc, char **argv) {
  int debug = 0;
  char *output = "a.out";

  arg_parser_t parser = {0};
  parser.program_name = "linker";
  arg_parser_allow_unflag(&parser, "file");
  arg_parser_add_arg(&parser, (arg_t){ARG_BOOL, NULL, 'd', "enable debug info", &debug});
  arg_parser_add_arg(&parser, (arg_t){ARG_STRING, NULL, 'o', "output file name", &output});
  arg_parser_parse(&parser, argc, argv);

  sv_allocator_t alloc = {0};

  obj_t objs[parser.unflagi];

  for (int i = 0; i < parser.unflagi; ++i) {
    objs[i] = obj_decode_file(parser.unflag[i], &alloc);
  }

  exe_t exe = link(objs, parser.unflagi);
  exe_encode_file(&exe, output);

  return 0;
}
