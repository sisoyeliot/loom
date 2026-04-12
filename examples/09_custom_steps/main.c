#include <stdio.h>
#include "config.h"

int main(void) {
    printf("Build info:\n");
    printf("  date     : %s\n", BUILD_DATE);
    printf("  time     : %s\n", BUILD_TIME);
    printf("  compiler : %s\n", BUILD_COMPILER);
    printf("  os       : %s\n", BUILD_OS);
    printf("  arch     : %s\n", BUILD_ARCH);
    return 0;
}
