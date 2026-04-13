#include "strutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg)                                                      \
  do {                                                                         \
    tests_run++;                                                               \
    if (cond) {                                                                \
      tests_passed++;                                                          \
    } else {                                                                   \
      printf("  FAIL: %s (line %d)\n", msg, __LINE__);                         \
    }                                                                          \
  } while (0)

static void test_count(void) {
  ASSERT(str_count("hello world", 'l') == 3, "count 'l' in hello world");
  ASSERT(str_count("aaa", 'a') == 3, "count 'a' in aaa");
  ASSERT(str_count("abc", 'z') == 0, "count 'z' in abc");
  ASSERT(str_count("", 'x') == 0, "count in empty string");
}

static void test_reverse(void) {
  char buf[64];

  strcpy(buf, "hello");
  str_reverse(buf);
  ASSERT(strcmp(buf, "olleh") == 0, "reverse hello");

  strcpy(buf, "a");
  str_reverse(buf);
  ASSERT(strcmp(buf, "a") == 0, "reverse single char");

  strcpy(buf, "ab");
  str_reverse(buf);
  ASSERT(strcmp(buf, "ba") == 0, "reverse two chars");
}

static void test_starts_with(void) {
  ASSERT(str_starts_with("hello", "hel"), "starts_with hel");
  ASSERT(str_starts_with("hello", "hello"), "starts_with full match");
  ASSERT(!str_starts_with("hello", "world"), "not starts_with world");
  ASSERT(str_starts_with("hello", ""), "starts_with empty prefix");
}

static void test_ends_with(void) {
  ASSERT(str_ends_with("hello", "llo"), "ends_with llo");
  ASSERT(str_ends_with("hello", "hello"), "ends_with full match");
  ASSERT(!str_ends_with("hello", "world"), "not ends_with world");
  ASSERT(!str_ends_with("hi", "hello"), "suffix longer than string");
}

static void test_trim(void) {
  char buf[64];

  strcpy(buf, "  hello  ");
  ASSERT(strcmp(str_trim(buf), "hello") == 0, "trim spaces");

  strcpy(buf, "\t\n  spaced \n\t");
  ASSERT(strcmp(str_trim(buf), "spaced") == 0, "trim whitespace");

  strcpy(buf, "clean");
  ASSERT(strcmp(str_trim(buf), "clean") == 0, "trim no-op");
}

int main(void) {
  printf("running strutil tests...\n");
  test_count();
  test_reverse();
  test_starts_with();
  test_ends_with();
  test_trim();

  printf("\n%d/%d tests passed\n", tests_passed, tests_run);
  return tests_passed == tests_run ? 0 : 1;
}
