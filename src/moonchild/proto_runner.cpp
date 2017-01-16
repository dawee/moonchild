#include "Arduboy.h"
#include "moonchild.h"


static Arduboy arduboy;


static BOOL has_update;
static moon_reference update_reference;
static moon_closure * closure;
static moon_reference right_pressed_reference;


void arduboy_print(moon_closure * closure, BOOL has_params) {
  arduboy.setCursor(MOON_AS_INT(closure->registers[closure->base])->val, MOON_AS_INT(closure->registers[closure->base + 1])->val);
  arduboy.print(F("o"));
}


void moon_arch_run(PGM_VOID_P prototype_addr) {
  arduboy.begin();
  arduboy.setFrameRate(15);

  moon_set_to_false(&right_pressed_reference);

  moon_add_global("arduboy_right_pressed", &right_pressed_reference);
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

