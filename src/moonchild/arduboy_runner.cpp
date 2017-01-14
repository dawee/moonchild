#ifndef MOONCHILD_SIMULATOR

#include "moonchild.h"
#include "Arduboy.h"

static Arduboy arduboy;
static BOOL has_update;
static moon_reference update_reference;
static moon_closure * closure;


void arduboy_print(moon_closure * closure, BOOL has_params) {
  moon_reference buf_ref;
  char buffer[255];
  
  moon_create_value_copy(&buf_ref, closure->registers[closure->base]);
  moon_ref_to_cstr(buffer, &buf_ref);

  arduboy.setCursor(4, 9);
  arduboy.print(buffer);

  if (buf_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf_ref.value_addr);  
}

void moon_arch_run(PGMEM_ADDRESS prototype_addr) {
  arduboy.begin();
  arduboy.setFrameRate(15);

  moon_add_global_api_func("arduboy_print", &arduboy_print);

  moon_reference key_reference;
  closure = moon_create_closure(prototype_addr);

  moon_create_string_value(&key_reference, "update");
  moon_run_closure(closure);
  has_update = moon_find_closure_value(&update_reference, closure, &key_reference);
}

void moon_arch_update() {
  if (!(arduboy.nextFrame())) return;
  
  arduboy.clear();

  if (has_update == TRUE && MOON_AS_VALUE(&update_reference)->type == LUA_CLOSURE) {
    moon_run_closure(MOON_AS_CLOSURE(&update_reference));
  }

  arduboy.display();
}

#endif