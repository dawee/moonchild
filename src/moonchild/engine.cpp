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

static int32_t read_int_value(moon_value * value) {
  int32_t data;
  progmem_cpy(&data, value->data_addr, sizeof(int32_t));

  return data;
}

static float read_number_value(moon_value * value) {
  float data;
  progmem_cpy(&data, value->data_addr, sizeof(float));

  return data;
}

static int32_t read_constant(moon_prototype * prototype, uint16_t index) {
  moon_value value;
  int32_t data = 0;

  progmem_cpy(&value, prototype->constants_addr, sizeof(moon_value), sizeof(moon_value) * index);

  switch(value.type) {
    case LUA_NUMBER:
      data = (int32_t) read_number_value(&value);
      break;
    case LUA_INT:
      data = read_int_value(&value);
      break;
    default:
      break;
  }

  return data;
}

static void run_instruction(moon_closure * closure, uint16_t index) {
  int8_t ccc;
  moon_instruction instruction;

  read_instruction(&instruction, closure->prototype, index);

  switch(instruction.opcode) {
    case OPCODE_LOADK:
      closure->registers[instruction.a] = read_constant(closure->prototype, instruction.b);
      break;

    case OPCODE_ADD:
      ccc = instruction.c & 0x7F;

      if (instruction.c & 0x80 == 0x80) {
        closure->registers[instruction.a] = closure->registers[instruction.b] + read_constant(closure->prototype, ccc);
      } else {
        closure->registers[instruction.a] = closure->registers[instruction.b] + closure->registers[ccc];
      }

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

  sprintf(result, "reg0 = %d", closure.registers[0]);
}
