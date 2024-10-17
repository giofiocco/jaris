#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "argparse/argparse.h"

#define MAX_TOKEN_LEN 256

int is_id(char c) {
  return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_';
}

int next_token(char *token, char **bufferi) {
  assert(token);
  assert(bufferi);
  memset(token, 0, MAX_TOKEN_LEN);

  char *buffer = *bufferi;
  int len = strlen(buffer);

  if (len == 0) {
    return 0;
  }
  if (**bufferi == ' ' || **bufferi == '\t' || **bufferi == '\r' || **bufferi == '\n') {
    ++*bufferi;
    return next_token(token, bufferi);
  } else if (buffer[0] == '-' && buffer[1] == '-') {
    int i = 0;
    while (i < len && buffer[i] != '\n') {
      ++i;
    }
    assert(i < MAX_TOKEN_LEN);
    memcpy(token, buffer, i);
    *bufferi += i;
    return 1;
  } else if (is_id(**bufferi)) {
    int i = 0;
    while (i < len && (is_id(buffer[i]) || buffer[i] == ':')) {
      ++i;
    }
    assert(i < MAX_TOKEN_LEN);
    memcpy(token, buffer, i);
    *bufferi += i;
    return 1;
  } else {
    memcpy(token, buffer, 1);
    ++*bufferi;
    return 1;
  }
}

int main(int argc, char **argv) {
  struct argparse_option options[] = {
    OPT_END(),
  };

  struct argparse argparse;
  argparse_init(&argparse,
                options,
                (const char *const[]){
                  "makemddocs [options] files ...",
                  NULL,
                },
                0);
  argc = argparse_parse(&argparse, argc, (const char **)argv);

  if (argc == 0) {
    fprintf(stderr, "ERROR: expected files\n");
    argparse_usage(&argparse);
    exit(1);
  }

  for (int i = 0; i < argc; ++i) {
    char *filename = argv[i];
    FILE *file = fopen(filename, "r");
    if (!file) {
      fprintf(stderr, "ERROR: cannot open file '%s': '%s'", filename, strerror(errno));
    }
    assert(fseek(file, 0, SEEK_END) == 0);
    int size = ftell(file);
    assert(size >= 0);
    assert(fseek(file, 0, SEEK_SET) == 0);
    char buffer[size];
    assert((long int)fread(buffer, 1, size, file) == size);
    buffer[size] = 0;
    assert(fclose(file) == 0);

    char globals[256][MAX_TOKEN_LEN] = {0};
    int global_count = 0;

    int collect_comment = 0;
    char comments[MAX_TOKEN_LEN * 256] = {0};
    char *commentsi = comments;

    char *bufferi = buffer;
    char token[MAX_TOKEN_LEN] = {0};
    while (next_token(token, &bufferi)) {
      int len = strlen(token);
      if (strcmp(token, "GLOBAL") == 0) {
        collect_comment = 0;
        assert(next_token(token, &bufferi));
        assert(global_count + 1 < 256);
        strcpy(globals[global_count++], token);
      } else if (len > 2 && token[0] == '-' && token[1] == '-') {
        if (collect_comment == 0) {
          memset(comments, 0, MAX_TOKEN_LEN * 256);
          commentsi = comments;
        }
        assert(comments - commentsi + len + 1 < MAX_TOKEN_LEN * 256);
        strcpy(commentsi, token + 2);
        commentsi += len - 2;
        *(commentsi++) = '\n';
        collect_comment = 1;
      } else if (token[len - 1] == ':') {
        collect_comment = 0;
        token[len - 1] = 0;
        for (int i = 0; i < global_count; ++i) {
          if (strcmp(globals[i], token) == 0) {
            printf("# %s\n\n%s\n", token, comments);
          }
        }
      } else {
        collect_comment = 0;
      }
    }
  }
}
