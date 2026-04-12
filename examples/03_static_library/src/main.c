#include "vec.h"
#include <stdio.h>

int main(void) {
    vec2_t a = {3.0f, 4.0f};
    vec2_t b = {1.0f, 2.0f};
    vec2_t c = vec2_add(a, b);
    printf("vec2: (%.1f, %.1f) + (%.1f, %.1f) = (%.1f, %.1f)\n",
           a.x, a.y, b.x, b.y, c.x, c.y);
    printf("vec2 len: %.3f\n", vec2_len(a));

    vec3_t u = {1.0f, 0.0f, 0.0f};
    vec3_t v = {0.0f, 1.0f, 0.0f};
    vec3_t w = vec3_cross(u, v);
    printf("vec3: (%.0f,%.0f,%.0f) x (%.0f,%.0f,%.0f) = (%.0f,%.0f,%.0f)\n",
           u.x, u.y, u.z, v.x, v.y, v.z, w.x, w.y, w.z);

    return 0;
}
