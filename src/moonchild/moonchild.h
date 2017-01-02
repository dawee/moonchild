#ifndef MOONCHILD_H
#define MOONCHILD_H

#include <stdint.h>

typedef struct {
  uint8_t opcode;
  uint16_t a;
  uint32_t b;
  uint16_t c;
} moon_instruction;

typedef struct {
  uint16_t func_name_size;
  const char * func_name;
  uint16_t instructions_count;
  const moon_instruction instructions [];
  uint16_t constants_count;
  const int64_t constants [];
} moon_prototype;


#endif