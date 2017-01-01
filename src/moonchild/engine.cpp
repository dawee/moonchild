#include "moonchild.h"
#include "Arduboy.h"
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>


static Arduboy arduboy;


typedef struct {
  uint8_t opcode;
  uint16_t a;
  uint32_t b;
  uint16_t c;
} lua_instruction;


typedef struct {
  uint16_t luac_size;
  uint16_t luac;
  uint16_t cursor;
  uint16_t top_func_name_size;
  char * top_func_name;
  uint16_t instructions_count;
  lua_instruction * instructions;
  uint16_t constants_count;
  int64_t * constants;
} lua_program;


static unsigned char read_byte(lua_program * program) {
  unsigned char byte = pgm_read_byte_near((program->luac) + (program->cursor));

  program->cursor++;
  return byte;
}

static skip_bytes(lua_program * program, uint16_t size) {
  program->cursor += size;
}

static void read_literal(lua_program * program, char * output, uint16_t size) {
  for (uint16_t index = 0; index < size; ++index) {
    output[index] = (char *) read_byte(program);
  }

  output[size] = '\0';
}

static void read_top_func_name(lua_program * program) {
  program->top_func_name_size = (uint16_t) read_byte(program) - 1;
  program->top_func_name = (char *) malloc(program->top_func_name_size + 1);

  read_literal(program, program->top_func_name, program->top_func_name_size);  
}

static int32_t read_int(lua_program * program) {
  int32_t res = 0;

  for (uint16_t index = 0; index < INT_SIZE; ++index) {
    res += (((int32_t) read_byte(program)) << (8 * index));
  }

  return res;
}

static int64_t read_lua_integer(lua_program * program) {
  int64_t res = 0;

  for (uint16_t index = 0; index < LUA_INTEGER_SIZE; ++index) {
    res += (((int64_t) read_byte(program)) << (8 * index));
  }

  return res;
}

static void read_code(lua_program * program) {
  program->instructions_count = read_int(program);
  program->instructions = (lua_instruction *) malloc(program->instructions_count * sizeof(lua_instruction));

  for (uint16_t index = 0; index < program->instructions_count; ++index) {
    uint32_t instruction = read_int(program);

    program->instructions[index].opcode = instruction & 0x3F;
      program->instructions[index].a = (instruction & 0x3FC0) >> 6;

    switch(program->instructions[index].opcode) {
      case OPCODE_LOADK:
      case OPCODE_LOADKX:
        program->instructions[index].b = (instruction & 0xFFFFC000) >> 14;        
        break;
      default:
        program->instructions[index].b = (instruction & 0xFF800000) >> 23;
        program->instructions[index].c = (instruction & 0x7FC000) >> 14;
        break;
    };
  }
}

static void read_constants(lua_program * program) {
  program->constants_count = read_int(program);
  program->constants = (int64_t *) malloc(program->constants_count * sizeof(int64_t));

  if (program->constants_count <= 0) return;

  for (uint8_t index = 0; index < program->constants_count; ++index) {
    skip_bytes(program, 1); // constant type
    program->constants[index] = read_lua_integer(program);    
  }
}

static void int64_to_str(char * str, int64_t val) {
  char buffer[32];

  if (val < 0) {
    int64_to_str(buffer, -val);
    sprintf(str, "-%s", buffer);
    return;
  }

  uint16_t cursor = 0;
  int64_t memo = val;
  int64_t divided = 0;
  int64_t divider = 1000000000000000000;

  for (uint16_t index = 0; index < 19; ++index) {
    divided = memo / divider;

    if (divided > 0 || cursor > 0) {
      sprintf(buffer, "%d", divided);
      str[cursor] = buffer[0];
      cursor++;
    }

    memo = memo - (divider * divided); 
    divider = divider / 10;
  }

  str[cursor] = '\0';
}


void moonchild_run(uint16_t luac, uint16_t luac_size) {
  lua_program * program = (lua_program *) malloc(sizeof(lua_program));

  program->luac = luac;
  program->luac_size = luac_size;
  program->cursor = 0;

  arduboy.begin();
  arduboy.setFrameRate(15);
  
  skip_bytes(program, 33);  // header
  skip_bytes(program, 1);  // sizeup values

  read_top_func_name(program);

  skip_bytes(program, 4);  // first line defined
  skip_bytes(program, 4);  // last line defined
  skip_bytes(program, 1);  // numparams
  skip_bytes(program, 1);  // isvarargs
  skip_bytes(program, 1);  // maxstacksize

  read_code(program);
  read_constants(program);

  char buffer[255];


  sprintf(buffer, "ins[2](a/b) = %d,%d", program->instructions[2].a, program->instructions[2].b);


  while(1) {
    if (!(arduboy.nextFrame())) continue;

    arduboy.clear();

    arduboy.setCursor(4, 9);
    arduboy.print(buffer);

    arduboy.display();    
  }

  free(program->top_func_name);
  free(program->instructions);
  free(program);
}
