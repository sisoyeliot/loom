#include <stdio.h>

#ifndef GREETING
#define GREETING "World"
#endif

int main(void) {
#ifdef USE_COLORS
    printf("\033[1;36mHello, %s!\033[0m\n", GREETING);
#else
    printf("Hello, %s!\n", GREETING);
#endif
    return 0;
}
