#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "../argparse.h"
#define ERRORS_IMPLEMENTATION
#include "../mystb/errors.h"
#define SV_IMPLEMENTATION
#include "../mystb/sv.h"

typedef struct {
  int count;
  int cap;
  sv_t *svs;
} svlist_t;

int sv_eq_char(sv_t a, char c) {
  return a.len == 1 && a.start[0] == c;
}

// zero-inited list is valid
void svlist_add(svlist_t *list, sv_t sv) {
  assert(list);

  if (list->count + 1 > list->cap) {
    list->cap += 2;
    list->svs = realloc(list->svs, list->cap * sizeof(sv_t));
    assert(list->svs);
  }

  list->svs[list->count++] = sv;
}

sv_t consume_sv(char **buffer, int len) {
  assert(buffer);
  assert(*buffer);
  sv_t sv = {*buffer, len};
  *buffer += len;
  return sv;
}

sv_t next_sv(char **_buffer) {
  assert(_buffer);
  assert(*_buffer);

  char *buffer = *_buffer;

  switch (*buffer) {
    case '\0':
      return (sv_t){0};
    case ' ':
    case '\t':
    case '\r':
      consume_sv(_buffer, 1);
      return next_sv(_buffer);
    case '\n':
    case '{':
    case '}':
    case ':':
    case '$':
      return consume_sv(_buffer, 1);
    case '-':
      if (buffer[1] == '-') {
        int i = 2;
        while (buffer[i] != '\n') {
          i++;
        }
        sv_t sv = consume_sv(_buffer, i);
        while (sv.len > 0 && (sv.start[sv.len - 1] == ' ' || sv.start[sv.len - 1] == '\t' || sv.start[sv.len - 1] == '\r')) {
          sv.len--;
        }
        return sv;
      }
      break;
    case '"':
    {
      int i = 1;
      while (buffer[i] != '"') {
        i++;
      }
      return consume_sv(_buffer, i + 1);
    } break;
    case '0':
      if (buffer[1] == 'x' || buffer[1] == 'X') {
        int i = 2;
        while (isdigit(buffer[i])
               || ('a' <= buffer[i] && buffer[i] <= 'f')
               || ('A' <= buffer[i] && buffer[i] <= 'F')) {
          i++;
        }
        return consume_sv(_buffer, i);
      } else if (buffer[1] == 'b' || buffer[1] == 'B') {
        int i = 2;
        while (buffer[i] == '1' || buffer[i] == '0') {
          i++;
        }
        return consume_sv(_buffer, i);
      }
      __attribute__((fallthrough));
    default:
    {
      if (isalpha(*buffer) || *buffer == '_') {
        int i = 1;
        while (isalnum(buffer[i]) || buffer[i] == '_') {
          i++;
        }
        return consume_sv(_buffer, i);
      } else if (isdigit(*buffer)) {
        int i = 1;
        while (isdigit(*buffer)) {
          i++;
        }
        return consume_sv(_buffer, i);
      }
    } break;
  }
  eprintf("unespected '%c'", *buffer);
  assert(0);
}

void print_help() {
  printf("Usage: asm_formatter [options] <file>\n"
         "outputs the formatted code\n\n"
         "Options\n"
         "  -o | --output <str>  output path [if not provided: stdout]\n"
         "  -h | --help          show help message\n");
}

int main(int argc, char **argv) {
  char *path = NULL;
  char *outpath = NULL;

  ARG_PARSE {
    ARG_PARSE_HELP_ARG                                    //
        else ARG_PARSE_STRING_ARG("o", "output", outpath) //
        else {
      if (path != NULL) {
        ARG_ERROR("file already provided: %s", path);
      }
      path = *argv;
    }
  }

  if (!path) {
    ARG_ERROR_("no source file provided");
  }

  FILE *file = strcmp(path, "-") == 0 ? stdin : fopen(path, "r");
  if (!file) {
    error_fopen(path);
  }
  assert(fseek(file, 0, SEEK_END) == 0);
  int size = ftell(file);
  assert(size >= 0);
  assert(fseek(file, 0, SEEK_SET) == 0);
  char buffer[size + 1];
  buffer[size] = 0;
  assert((long int)fread(buffer, 1, size, file) == size);
  buffer[size] = 0;
  assert(fclose(file) == 0);

  svlist_t globals = {0};
  svlist_t externs = {0};
  svlist_t code = {0};
  svlist_t macros = {0};

  sv_t sv = {0};
  char *_buffer = buffer;
  while ((sv = next_sv(&_buffer)).start != NULL) {

    if (sv_eq(sv, sv_from_cstr("EXTERN"))) {
      sv = next_sv(&_buffer);
      svlist_add(&externs, sv);

    } else if (sv_eq(sv, sv_from_cstr("GLOBAL"))) {
      sv = next_sv(&_buffer);
      svlist_add(&globals, sv);

    } else if (sv_eq_char(sv, '{')) {
      svlist_add(&macros, sv);
      while ((sv = next_sv(&_buffer)).start != NULL && !sv_eq_char(sv, '}')) {
        svlist_add(&macros, sv);
      }
      svlist_add(&macros, sv);

    } else {
      svlist_add(&code, sv);
    }
  }

  FILE *out = outpath == NULL || strcmp(outpath, "-") == 0 ? stdout : fopen(outpath, "w");
  if (!out) {
    error_fopen(outpath);
  }

  if (globals.count > 0) {
    for (int i = 0; i < globals.count; ++i) {
      fprintf(out, "GLOBAL " SV_FMT "\n", SV_UNPACK(globals.svs[i]));
    }
    fprintf(out, "\n");
  }

  if (externs.count > 0) {
    for (int i = 0; i < externs.count; ++i) {
      fprintf(out, "EXTERN " SV_FMT "\n", SV_UNPACK(externs.svs[i]));
    }
    fprintf(out, "\n");
  }

  if (macros.count > 0) {
    for (int i = 0; i < macros.count; ++i) {
      if (sv_eq_char(macros.svs[i], '{')) {
        fprintf(out, "  ");
      }
      fprintf(out, SV_FMT, SV_UNPACK(macros.svs[i]));
      if (sv_eq_char(macros.svs[i], '}')) {
        fprintf(out, "\n");
      } else {
        fprintf(out, " ");
      }
    }
    fprintf(out, "\n");
  }

  int new_line = 1;
  for (int i = 0; i < code.count; ++i) {
    if (sv_eq_char(code.svs[i], '\n') && new_line) {
      while (i < code.count && sv_eq_char(code.svs[i], '\n')) {
        i++;
      }
      if (i == code.count) {
        break;
      }
      i--;
      fprintf(out, "\n");
      new_line = 1;
    } else {
      fprintf(out, SV_FMT, SV_UNPACK(code.svs[i]));
      new_line = sv_eq_char(code.svs[i], '\n');
    }

    if (!((i + 2 < code.count && sv_eq_char(code.svs[i + 2], ':'))
          || (i + 1 < code.count && sv_eq_char(code.svs[i + 1], '\n'))
          || (i + 1 < code.count && sv_eq_char(code.svs[i + 1], ':'))
          || sv_eq_char(code.svs[i], '$'))) {
      fprintf(out, " ");
    }
    if (i + 2 < code.count && sv_eq_char(code.svs[i + 2], ':') && !new_line) {
      fprintf(out, "\n");
    }
  }

  return 0;
}
