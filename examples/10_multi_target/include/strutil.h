#ifndef STRUTIL_H
#define STRUTIL_H

#include <stddef.h>

size_t str_count(const char *haystack, char needle);
void str_reverse(char *s);
int str_starts_with(const char *s, const char *prefix);
int str_ends_with(const char *s, const char *suffix);
char *str_trim(char *s);

#endif
