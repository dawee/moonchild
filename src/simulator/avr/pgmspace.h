#ifndef SIM_PGMSPACE_H
#define SIM_PGMSPACE_H

#define PROGMEM
typedef void * PGM_VOID_P;

char pgm_read_byte_near(PGM_VOID_P mem_addr);

#endif
