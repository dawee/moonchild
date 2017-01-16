#include <avr/pgmspace.h>

char pgm_read_byte_near(PGM_VOID_P mem_addr) {
  return *((char *) mem_addr);
}
