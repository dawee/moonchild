#ifdef MOONCHILD_SIMULATOR

#include "moonchild.h"
#include <stdio.h>

char progmem_read(PGMEM_ADDRESS mem_addr, uint16_t offset) {
  char * bytes = (char *) mem_addr;

  return bytes[offset];
}


void moon_arch_run(PGMEM_ADDRESS prototype_addr) {
  BOOL has_update;

  moon_reference key_reference;
  moon_reference update_reference;
  moon_closure * closure = moon_create_closure(prototype_addr);

  moon_create_string_value(&key_reference, "update");

  moon_run_closure(closure);

  has_update = moon_find_closure_value(&update_reference, closure, &key_reference);

  if (has_update == TRUE && MOON_AS_VALUE(&update_reference)->type == LUA_CLOSURE) {

    moon_run_closure(MOON_AS_CLOSURE(&update_reference));
    moon_debug("update exists\n");
  }
}

int main() {
  moon_init();
  moon_run_generated();
  return 0;
}

#endif