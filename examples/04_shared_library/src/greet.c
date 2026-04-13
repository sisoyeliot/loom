#include "greet.h"
#include <stdio.h>
#include <string.h>

static char buf[256];

const char *greet(const char *name) {
  snprintf(buf, sizeof(buf), "Hello, %s!", name);
  return buf;
}

const char *farewell(const char *name) {
  snprintf(buf, sizeof(buf), "Goodbye, %s!", name);
  return buf;
}
