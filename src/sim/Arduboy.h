#ifndef SIM_ARDUBOY_H
#define SIM_ARDUBOY_H

#define F(x) x

#define RIGHT_BUTTON 0

class Arduboy {

public:
  Arduboy();

  void setCursor(int x, int y);
  void print(const char *);
  bool nextFrame();
  void clear();
  void display();
  void begin();
  void setFrameRate(int rate);
  bool pressed(int key);

};

#endif