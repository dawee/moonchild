#ifdef MOONCHILD_MONITOR

#ifndef MOONCHILD_MONITOR_H
#define MOONCHILD_MONITOR_H

#include <stdlib.h>


void * moon_monitor_malloc(const char * name, size_t size);
void moon_monitor_free(const char * name, void * pointer);
int moon_monitor_get_total();

#undef MOON_MALLOC
#define MOON_MALLOC(name, size) moon_monitor_malloc(name, size)
#undef MOON_FREE
#define MOON_FREE(name, pointer) moon_monitor_free(name, pointer)

#endif

#endif
