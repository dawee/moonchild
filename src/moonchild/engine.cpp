#include "moonchild.h"
#include "Arduboy.h"
#include <stdlib.h>
#include <stdio.h>


static Arduboy arduboy;

typedef struct {
  unsigned luac_size;
  unsigned char * luac;
  unsigned cursor;
} lua_program;


static unsigned char read_byte(lua_program * program) {
  unsigned char byte = program->luac[program->cursor];

  program->cursor++;
  return byte;
}

static void read_literal(lua_program * program, char * output, unsigned size) {
  for (unsigned index = 0; index < size; ++index) {
    output[index] = (char *) read_byte(program);
  }

  output[size] = '\0';
}

void moonchild_run(unsigned char * luac, unsigned luac_size) {
  unsigned name_size;
  char buffer[255];
  lua_program * program = (lua_program *) malloc(sizeof(lua_program));

  program->luac = luac;
  program->luac_size = luac_size;
  program->cursor = 0;

  arduboy.begin();
  arduboy.setFrameRate(15);


  // header
  for (unsigned index = 0; index < 33; ++index) {
    read_byte(program);
  }

  // sizeup values
  read_byte(program);

  name_size = (unsigned) read_byte(program) - 1;

  read_literal(program, buffer, name_size);

  while(1) {
    if (!(arduboy.nextFrame())) continue;

    arduboy.clear();

    arduboy.setCursor(4, 9);
    arduboy.print(buffer);

    arduboy.display();    
  }

  free(program);
}
