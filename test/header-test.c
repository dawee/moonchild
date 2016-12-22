#include <stdio.h>
#include <luavr/luavr.h>

static char program[] = {
  0x1b, 0x4c, 0x75, 0x61, 0x51, 0x00, 0x01, 0x04, 0x08, 0x04, 0x08, 0x00
};

int main() {
  luavr_header header;
  luavr_read_header(&header, program);

  if (header.version_number != 0x51) {
    printf("[header-test] version number != 0x51\n");
    return 1;
  }

  if (header.format_version != 0) {
    printf("[header-test] format version != 0\n");
    return 1;
  }

  printf("[header-test] ok\n");
  return 0;
}
