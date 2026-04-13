#include "vec.h"
#include <math.h>

vec2_t vec2_add(vec2_t a, vec2_t b) { return (vec2_t){a.x + b.x, a.y + b.y}; }

vec2_t vec2_scale(vec2_t v, float s) { return (vec2_t){v.x * s, v.y * s}; }

float vec2_dot(vec2_t a, vec2_t b) { return a.x * b.x + a.y * b.y; }

float vec2_len(vec2_t v) { return sqrtf(vec2_dot(v, v)); }
