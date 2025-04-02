#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "argparse/argparse.h"
#include "files.h"
#include "link.h"

exe_state_t state = {0};

int add_dynamic_file(struct argparse *self, const struct argparse_option *option) {
  (void)self;
  assert(option);

  assert(0 && "TODO");

  char *filename = (char *)self->argv[0];
  assert(filename);
  so_t so = so_decode_file(filename);
  exe_link_so(&state, &so, filename);

  return 0;
}

int main(int argc, char **argv) {
  int flags = 0;
  char *output = "a.out";
  char *dynamic_filename = NULL;
  int no_std_lib_link = 0;
  int debug_info = 0;
  char *stdlib_path = "mem/__stdlib";

  struct argparse_option options[] = {
      OPT_GROUP("Options"),
      OPT_HELP(),
      OPT_STRING('o', "output", &output, "output file name", NULL, 0, 0),
      OPT_BOOLEAN('g', "debug", &debug_info, "include debug info", NULL, 0, 0),
      OPT_STRING(
          'l', NULL, &dynamic_filename, "link dynamically with file name", add_dynamic_file, 0, 0),
      OPT_BOOLEAN(0, "nostdlib", &no_std_lib_link, "not link with std lib", NULL, 0, 0),
      OPT_STRING('l', "stdlib_path", &stdlib_path, "set path of stdlib [default 'mem/__stdlib']", NULL, 0, 0),
      OPT_BIT(0, "bin", &flags, "output bin file", NULL, LINK_FLAG_BIN, 0),
      OPT_BIT(0, "so", &flags, "output so file", NULL, LINK_FLAG_SO, 0),
      OPT_BIT('d', "debug", &flags, "debug info", NULL, LINK_FLAG_EXE_STATE, 0),
      OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse,
                options,
                (const char *const[]){
                    "linker [options] files ...",
                    NULL,
                },
                0);
  argc = argparse_parse(&argparse, argc, (const char **)argv);
  if (argc == 0) {
    fprintf(stderr, "ERROR: expected files\n");
    argparse_usage(&argparse);
    exit(1);
  }

  if (!(flags & LINK_FLAG_BIN) && !(flags & LINK_FLAG_SO)) {
    exe_link_boilerplate(&state, debug_info);
  }

  for (int i = 0; i < argc; ++i) {
    obj_t obj = obj_decode_file(argv[i]);
    exe_link_obj(&state, &obj, debug_info);
  }

  if (!no_std_lib_link) {
    so_t so = so_decode_file(stdlib_path);
    exe_link_so(&state, &so, "\001");
  }

  if (flags & LINK_FLAG_EXE_STATE) {
    exe_state_dump(&state);
  }

  exe_state_check_exe(&state);

  if (flags & LINK_FLAG_SO) {
    so_t out = so_from_exe_state(&state);
    so_encode_file(&out, output);
  } else if (flags & LINK_FLAG_BIN) {
    bin_encode_file(&state.exe, output);
  } else {
    exe_encode_file(&state.exe, output);
  }

  return 0;
}
