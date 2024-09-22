#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SV_IMPLEMENTATION
#include "argparse/argparse.h"
#include "files.h"
#include "link.h"

#define STD_LIB_PATH "mem/__stdlib"

char *dynamic_files[DYNAMIC_COUNT] = {0};
int dynamic_file_count = 0;

int add_dynamic_file(struct argparse *self, const struct argparse_option *option) {
  (void)self;
  assert(option);

  dynamic_files[dynamic_file_count++] = (char *)self->argv[0];

  return 0;
}

int main(int argc, char **argv) {
  int flags = 0;
  char *output = "a.out";
  char *dynamic_filename = NULL;
  int no_std_lib_link = 0;

  struct argparse_option options[] = {
    OPT_GROUP("Options"),
    OPT_HELP(),
    OPT_STRING('o', "output", &output, "output file name", NULL, 0, 0),
    OPT_STRING('l', NULL, &dynamic_filename, "link dynamically with file name", add_dynamic_file, 0, 0),
    OPT_BOOLEAN(0, "nostdlib", &no_std_lib_link, "not link with std lib", NULL, 0, 0),
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

  exe_state_t exes = {0};

  if (!(flags & LINK_FLAG_BIN) && !(flags & LINK_FLAG_SO)) {
    exe_link_boilerplate(&exes);
  }

  for (int i = 0; i < argc; ++i) {
    obj_t obj = obj_decode_file(argv[i]);
    exe_link_obj(&exes, &obj);
  }

  if (!no_std_lib_link) {
    so_t so = so_decode_file(STD_LIB_PATH);
    exes.sos[0] = so;
    exes.so_names[0] = "\x01";
    exes.so_num = 1;
  }

  for (int i = 0; i < dynamic_file_count; ++i) {
    so_t so = so_decode_file(dynamic_files[i]);
    assert(exes.so_num + 1 < DYNAMIC_COUNT);
    exes.sos[exes.so_num] = so;
    exes.so_names[exes.so_num] = dynamic_files[i];
    ++exes.so_num;
  }

  if (flags & LINK_FLAG_EXE_STATE) {
    exe_state_dump(&exes);
  }

  exe_state_check_exe(&exes);

  if (flags & LINK_FLAG_SO) {
    so_encode_file(&exes, output);
  } else if (flags & LINK_FLAG_BIN) {
    bin_encode_file(&exes.exe, output);
  } else {
    exe_encode_file(&exes.exe, output);
  }

  return 0;
}
