#include "strutil.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("usage: strcli <text>\n");
    return 1;
  }

  char buf[1024];
  strncpy(buf, argv[1], sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  printf("original : \"%s\"\n", buf);
  printf("length   : %zu\n", strlen(buf));
  printf("spaces   : %zu\n", str_count(buf, ' '));
  printf("starts 'H': %s\n", str_starts_with(buf, "H") ? "yes" : "no");
  printf("ends '!'  : %s\n", str_ends_with(buf, "!") ? "yes" : "no");

  str_reverse(buf);
  printf("reversed : \"%s\"\n", buf);

  return 0;
}
