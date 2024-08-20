#ifndef SV_H__
#define SV_H__

#define SV_ALLOCATOR_SIZE 1024

typedef struct {
  char *start;
  unsigned int len;
} sv_t;

typedef struct {
  char data[SV_ALLOCATOR_SIZE];
  int data_num;
} sv_allocator_t;

#define SV_FMT            "%*.*s"
#define SV_UNPACK(__sv__) (__sv__).len, (__sv__).len, (__sv__).start

int sv_eq(sv_t a, sv_t b);
sv_t sv_from_cstr(char *str);
sv_t sv_union(sv_t a, sv_t b); 

sv_t sv_alloc_cstr(sv_allocator_t *alloc, char *image);
sv_t sv_alloc_len(sv_allocator_t *alloc, unsigned int len);

#endif // SV_H__
       
#ifdef SV_IMPLEMENTATION

#include <string.h>
#include <assert.h>

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
  return (sv_t){str, (unsigned int)strlen(str)};
}

sv_t sv_union(sv_t a, sv_t b) {
  return (sv_t){a.start, (unsigned int)(b.start + b.len - a.start)};
}

sv_t sv_alloc_cstr(sv_allocator_t *alloc, char *image) {
  assert(alloc);
  assert(image);

  unsigned int len = strlen(image);
  assert(len);

  char *start = &alloc->data[alloc->data_num];
  alloc->data_num += len;
  assert(alloc->data_num < SV_ALLOCATOR_SIZE);

  return (sv_t) {start, len};
}

sv_t sv_alloc_len(sv_allocator_t *alloc, unsigned int len) {
  assert(alloc);
  assert(len);


  char *start = &(alloc->data[alloc->data_num]);
  alloc->data_num += len;
  assert(alloc->data_num < SV_ALLOCATOR_SIZE);

  return (sv_t) {start, len};
}

#endif // SV_IMPLEMENTATION
