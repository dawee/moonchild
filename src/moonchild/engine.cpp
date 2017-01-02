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

static uint64_t read_constant(moon_prototype * prototype, uint16_t index) {
  uint64_t constant;

  progmem_cpy(&constant, prototype->constants_addr, sizeof(uint64_t), sizeof(uint64_t) * index);

  return constant;
}

static void run_instruction(moon_closure * closure, uint16_t index) {
  moon_instruction instruction;

  read_instruction(&instruction, closure->prototype, index);

  switch(instruction.opcode) {
    case OPCODE_LOADK:
      closure->registers[instruction.a] = read_constant(closure->prototype, instruction.b);
      break;

    case OPCODE_ADD:
      closure->registers[instruction.a] = closure->registers[instruction.b] + closure->registers[instruction.c];
      break;

    case OPCODE_SUB:
      closure->registers[instruction.a] = closure->registers[instruction.b] - closure->registers[instruction.c];
      break;

    case OPCODE_MUL:
      closure->registers[instruction.a] = closure->registers[instruction.b] * closure->registers[instruction.c];
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

  sprintf(result, "reg0 = %d", (int) closure.registers[0]);
}
