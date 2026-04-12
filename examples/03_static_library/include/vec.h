#ifndef VEC_H
#define VEC_H

typedef struct { float x, y; } vec2_t;
typedef struct { float x, y, z; } vec3_t;

vec2_t vec2_add(vec2_t a, vec2_t b);
vec2_t vec2_scale(vec2_t v, float s);
float  vec2_dot(vec2_t a, vec2_t b);
float  vec2_len(vec2_t v);

vec3_t vec3_add(vec3_t a, vec3_t b);
vec3_t vec3_cross(vec3_t a, vec3_t b);
float  vec3_dot(vec3_t a, vec3_t b);
float  vec3_len(vec3_t v);

#endif
