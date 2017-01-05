#include "moonchild.h"

#include <stdio.h>


static void progmem_cpy(void * dest, PGMEM_ADDRESS src, uint16_t size, uint16_t offset = 0) {
  char * cdest = (char *)dest;

  for (uint16_t index = 0; index < size; ++index) {
    cdest[index] = progmem_read(src, index + offset);
  }
}

static void init_prototype(moon_prototype * prototype, PGMEM_ADDRESS prototype_addr) {
  progmem_cpy(prototype, prototype_addr, sizeof(moon_prototype));
}

static void read_instruction(moon_instruction * instruction, moon_prototype * prototype, uint16_t index) {
  progmem_cpy(instruction, prototype->instructions_addr, sizeof(moon_instruction), sizeof(moon_instruction) * index);
}



static void run_instruction(moon_closure * closure, uint16_t index) {
  moon_instruction instruction;

  read_instruction(&instruction, closure->prototype, index);

  switch(instruction.opcode) {
    case OPCODE_LOADK:
      break;

    default:
      break;
  };
}


void moon_run(PGMEM_ADDRESS prototype_addr, char * result) {
  moon_prototype prototype;
  moon_closure closure = {.prototype = &prototype};

  init_prototype(&prototype, prototype_addr);

  for (uint16_t index = 0; index < prototype.instructions_count; ++index) {
    run_instruction(&closure, index);
  }

  sprintf(result, "bee bop");
}
