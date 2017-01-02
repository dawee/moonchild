#ifndef MOONCHILD_SIMULATOR

#include "moonchild.h"
#include "Arduboy.h"

static Arduboy arduboy;


void moon_arch_run(PGMEM_ADDRESS prototype_addr) {
  char buffer[255];

  arduboy.begin();
  arduboy.setFrameRate(15);
  moon_run(prototype_addr, buffer);

  while(1) {
    if (!(arduboy.nextFrame())) continue;

    arduboy.clear();

    arduboy.setCursor(4, 9);
    arduboy.print(buffer);

    arduboy.display();    
  }
}

#endif