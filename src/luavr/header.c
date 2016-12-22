#include <luavr/luavr.h>
#include <string.h>

void luavr_read_header(luavr_header * header, char * program) {
  memcpy(header, program, sizeof(luavr_header));
}
