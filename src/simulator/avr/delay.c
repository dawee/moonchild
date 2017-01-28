#include <avr/delay.h>
#include <time.h>
#include <math.h>


void delay_ms(double ms) {
  struct timespec time_val;

  time_val.tv_sec = (time_t) floor(ms / 1000);
  time_val.tv_nsec = (long) ((ms - time_val.tv_sec) * 1000000);

  nanosleep(&time_val, NULL);
}
