#include "Arduboy.h"
#include "moonchild.h"


static Arduboy arduboy;




static BOOL has_update;
static moon_reference update_reference;
static moon_closure * closure;



void arduboy_print(moon_closure * closure, BOOL has_params) {
  moon_reference buf1_ref;
  moon_reference buf2_ref;
  moon_reference buf3_ref;
  char buffer[16];

  moon_create_value_copy(&buf1_ref, closure->registers[closure->base]);
  moon_create_value_copy(&buf2_ref, closure->registers[closure->base + 1]);
  moon_create_value_copy(&buf3_ref, closure->registers[closure->base + 2]);
  moon_ref_to_cstr(buffer, &buf3_ref);

  arduboy.setCursor(MOON_AS_INT(&buf1_ref)->val, MOON_AS_INT(&buf2_ref)->val);
  arduboy.print(F("o"));

  if (buf1_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf1_ref.value_addr);
  if (buf2_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf2_ref.value_addr);
  if (buf3_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf3_ref.value_addr);
}


void moon_arch_run(PGM_VOID_P prototype_addr) {
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
    moon_run_closure(MOON_AS_CLOSURE(&update_reference), closure);
  }

  arduboy.display();
}


#ifdef MOONCHILD_SIMULATOR
int main() {
  moon_init();
  moon_run_generated();
  moon_arch_update();
  moon_arch_update();
  moon_arch_update();
  moon_arch_update();
  moon_arch_update();
  return 0;
}
#endif

