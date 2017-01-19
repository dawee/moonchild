#ifndef MOONCHILD_MONITOR_H
#define MOONCHILD_MONITOR_H

#include <stdlib.h>


#ifndef MOONCHILD_DEBUG
  #define moon_malloc(name, size) malloc(size)
  #define moon_free(name, pointer) free(pointer)
#else
  void * moon_malloc(const char * name, size_t size);
  void moon_free(const char * name, void * pointer);
#endif

#endif
