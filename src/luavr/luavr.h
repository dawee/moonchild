#ifndef LUAVR_H
#define LUAVR_H

typedef struct {
  char signature[4];
  char version_number;
  char format_version;
  char endianness_flag;
  char int_size;
  char size_t_size;
  char instruction_size;
  char lua_number_size;
  char integral_flag;
} luavr_header;


luavr_header * luavr_init_header();
void luavr_read_header(luavr_header *, char *);
void luavr_free_header(luavr_header *);

#endif