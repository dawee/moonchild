#include <luavr/luavr.h>
#include <string.h>
#include <stdlib.h>


luavr_header * luavr_init_header() {
  return (luavr_header *) malloc(sizeof(luavr_header));
}

void luavr_read_header(luavr_header * header, char * program) {
  memcpy(header, program, sizeof(luavr_header));
}

void luavr_free_header(luavr_header * header) {
  free(header);
  header = NULL;
}
