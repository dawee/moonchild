#include "moonchild.h"
#include "Arduboy.h"
#include <stdlib.h>
#include <stdio.h>


static Arduboy arduboy;

typedef struct {
  unsigned char opcode;
} lua_instruction;

typedef struct {
  unsigned luac_size;
  unsigned char * luac;
  unsigned cursor;
  unsigned top_func_name_size;
  char * top_func_name;
  unsigned instructions_count;
  lua_instruction * instructions;
  unsigned constants_count;
  unsigned first_constant;
} lua_program;


static unsigned char read_byte(lua_program * program) {
  unsigned char byte = program->luac[program->cursor];

  program->cursor++;
  return byte;
}

static skip_bytes(lua_program * program, unsigned size) {
  program->cursor += size;
}

static void read_literal(lua_program * program, char * output, unsigned size) {
  for (unsigned index = 0; index < size; ++index) {
    output[index] = (char *) read_byte(program);
  }

  output[size] = '\0';
}

static void read_top_func_name(lua_program * program) {
  program->top_func_name_size = (unsigned) read_byte(program) - 1;
  program->top_func_name = (char *) malloc(program->top_func_name_size + 1);

  read_literal(program, program->top_func_name, program->top_func_name_size);  
}

static int read_int(lua_program * program) {
  int res = read_byte(program);

  skip_bytes(program, 3);

  return res;
}

static void read_code(lua_program * program) {
  program->instructions_count = read_int(program);
  program->instructions = (lua_instruction *) malloc(program->instructions_count * sizeof(lua_instruction));

  for (unsigned index = 0; index < program->instructions_count; ++index) {
    program->instructions[index].opcode = read_byte(program);
    skip_bytes(program, 3);
  }
}

static void read_constants(lua_program * program) {
  program->constants_count = read_int(program);

  if (program->constants_count <= 0) return;

  skip_bytes(program, 1); // constant type
  program->first_constant = read_int(program);
}

void moonchild_run(unsigned char * luac, unsigned luac_size) {
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

  sprintf(buffer, "constant = %d", program->first_constant);


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
