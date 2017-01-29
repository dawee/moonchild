#include "api.h"
#include "moonchild.h"
#include <util/delay.h>
#include <avr/io.h>


void moon_api_light_on(moon_closure * closure, BOOL has_params) {
  DDRB |= _BV(DDB5);
  PORTB |= _BV(PORTB5);
}

void moon_api_light_off(moon_closure * closure, BOOL has_params) {
  DDRB |= _BV(DDB5);
  PORTB &= ~_BV(PORTB5);
}

void moon_api_delay_ms(moon_closure * closure, BOOL has_params) {
  moon_reference buf_ref;
  int count = 0;

  moon_create_value_copy(&buf_ref, closure->registers[closure->base]);

  while (count < (MOON_AS_INT(&buf_ref)->val)) {
    _delay_ms(1);
    count++;
  }

  if (buf_ref.is_copy == TRUE) moon_delete_value((moon_value *) buf_ref.value_addr);
}


void moon_feed_api() {
 moon_add_global_api_func("light_on", &moon_api_light_on);
 moon_add_global_api_func("light_off", &moon_api_light_off);
 moon_add_global_api_func("delay_ms", &moon_api_delay_ms);
}
