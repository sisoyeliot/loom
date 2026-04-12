#include <stdio.h>
#include <time.h>

static volatile long sink;

int main(void) {
  clock_t start = clock();

  long sum = 0;
  for (long i = 0; i < 100000000L; i++)
    sum += i;
  sink = sum;

  double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC;
  printf("sum = %ld\n", sum);
  printf("elapsed: %.4f s\n", elapsed);
  return 0;
}
