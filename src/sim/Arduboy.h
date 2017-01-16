#ifndef SIM_ARDUBOY_H
#define SIM_ARDUBOY_H

#define F(x) x

class Arduboy {

public:
  Arduboy();

  void setCursor(int x, int y);
  void print(const char *);
  bool nextFrame();
  void clear();
  void display();

};

#endif