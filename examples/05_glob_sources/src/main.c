#include "shapes.h"
#include <stdio.h>

int main(void) {
    printf("circle  (r=5)     : %.2f\n", circle_area(5.0));
    printf("rect    (3x4)     : %.2f\n", rect_area(3.0, 4.0));
    printf("triangle (6, h=3) : %.2f\n", triangle_area(6.0, 3.0));
    return 0;
}
