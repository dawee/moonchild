#include "Arduboy.h"

#include <stdio.h>

Arduboy::Arduboy() {}

void Arduboy::setCursor(int x, int y) {
  printf("arduboy.setCursor(%d, %d)\n", x, y);
}

void Arduboy::print(const char * str) {
  printf("arduboy.print(%s)\n", str);
}

bool Arduboy::nextFrame(){
  return true;
}

void Arduboy::clear(){}

void Arduboy::display(){}

void Arduboy::begin() {
  printf("%s\n", "arduboy.begin");
}

void Arduboy::setFrameRate(int rate) {
  printf("%s\n", "arduboy.setFrameRate");
}
