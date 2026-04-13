#include "strutil.h"
#include <ctype.h>
#include <string.h>

size_t str_count(const char *haystack, char needle) {
  size_t n = 0;
  for (; *haystack; haystack++)
    if (*haystack == needle)
      n++;
  return n;
}

void str_reverse(char *s) {
  size_t len = strlen(s);
  for (size_t i = 0; i < len / 2; i++) {
    char tmp = s[i];
    s[i] = s[len - 1 - i];
    s[len - 1 - i] = tmp;
  }
}

int str_starts_with(const char *s, const char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

int str_ends_with(const char *s, const char *suffix) {
  size_t ls = strlen(s), lsuf = strlen(suffix);
  if (lsuf > ls)
    return 0;
  return strcmp(s + ls - lsuf, suffix) == 0;
}

char *str_trim(char *s) {
  while (isspace((unsigned char)*s))
    s++;
  if (*s == '\0')
    return s;
  char *end = s + strlen(s) - 1;
  while (end > s && isspace((unsigned char)*end))
    end--;
  *(end + 1) = '\0';
  return s;
}
