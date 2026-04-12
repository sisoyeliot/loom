#include "math_ops.h"
#include "print.h"

int main(void) {
  print_result("+", 10, 25, add(10, 25));
  print_result("-", 50, 18, sub(50, 18));
  print_result("*", 7, 6, mul(7, 6));
  return 0;
}
