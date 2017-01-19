#ifdef MOONCHILD_DEBUG

#include "monitor.h"
#include "moonchild.h"


void * moon_malloc(const char * name, size_t size) {
  return malloc(size);
}

void moon_free(const char * name, void * pointer) {
  return free(pointer);
}

#endif
