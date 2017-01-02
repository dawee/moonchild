#include "moonchild.h"

#include <stdio.h>



static void moon__progmem_cpy(void * dest, PGMEM_ADDRESS src, uint16_t size) {
  char * cdest = (char *)dest;

  for (uint16_t index = 0; index < size; ++index) {
    cdest[index] = progmem_read(src, index);
  }
}

void moon_run(PGMEM_ADDRESS prototype_addr, char * result) {
  moon_prototype prototype;

  moon__progmem_cpy(&prototype, prototype_addr, sizeof(moon_prototype));
  moon__progmem_cpy(result, prototype.func_name_addr, prototype.func_name_size);

  result[prototype.func_name_size] = '\0';
}
