#ifdef MOONCHILD_SIMULATOR

#include "moonchild.h"
#include <stdio.h>

char progmem_read(PGMEM_ADDRESS mem_addr, uint16_t offset) {
  char * bytes = (char *) mem_addr;

  return bytes[offset];
}


void moon_arch_run(PGMEM_ADDRESS prototype_addr) {
  char buffer[255];

  moon_run(prototype_addr, buffer);

  printf("%s\n", buffer);
}

int main() {
  moon_run_generated();
  return 0;
}

#endif