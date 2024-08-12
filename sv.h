#ifndef SV_H__
#define SV_H__

typedef struct {
  char *start;
  unsigned int len;
} sv_t;

#define SV_FMT            "%*.*s"
#define SV_UNPACK(__sv__) (__sv__).len, (__sv__).len, (__sv__).start

int sv_eq(sv_t a, sv_t b);
sv_t sv_from_cstr(char *str);
sv_t sv_union(sv_t a, sv_t b); 

#endif // SV_H__
       
#ifdef SV_IMPLEMENTATION

#include <string.h>

int sv_eq(sv_t a, sv_t b) {
  if (a.len != b.len) {
    return 0;
  }
  for (unsigned int i = 0; i < a.len; ++i) {
    if (a.start[i] != b.start[i]) {
      return 0;
    }
  }
  return 1;
}

sv_t sv_from_cstr(char *str) {
  return (sv_t){str, strlen(str)};
}

sv_t sv_union(sv_t a, sv_t b) {
  return (sv_t){a.start, b.start + b.len - a.start};
}

#endif // SV_IMPLEMENTATION
