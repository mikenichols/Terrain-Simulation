#ifndef TYPEDEFS_H
#define TYPEDEFS_H

typedef struct {
	float x;
	float y;
} vec2f;

typedef struct {
	int x;
	int y;
} vec2i;

typedef struct {
	float x;
	float y;
	float z;
} vec3f;

typedef struct {
	int x;
	int y;
	int z;
} vec3i;

typedef struct {
	int m;
	int top_bottom;
	int left_right;
	int L_region;
	int finest;
	int top_offset;
	int bottom_offset;
	int left_offset;
	int right_offset;
} indexsize_t;

typedef enum {
	DIR_UP,
	DIR_DOWN,
	DIR_LEFT,
	DIR_RIGHT,
	DIR_FORWARD,
	DIR_BACKWARD,
} direction_t;

typedef enum {
	INDEX_TOP = 0,
	INDEX_BOTTOM = 1,
	INDEX_LEFT = 2,
	INDEX_RIGHT = 3,
	INDEX_L_REGION = 4,
	INDEX_FINEST = 5,
} index_t;

// emulate the bool type of c++
typedef enum {
	false = 0,
	true = 1,
} bool;

vec2i add2i(vec2i a, vec2i b);

void set3f(vec3f *c, float x, float y, float z);
vec3f copy3f(vec3f *c);
void negate3f(vec3f *c);
vec3f add3f(vec3f a, vec3f b);
vec3i add3i(vec3i a, vec3i b);
vec3f subtract3f(vec3f a, vec3f b);
float dot3f(vec3f a, vec3f b);
vec3f cross3f(vec3f a, vec3f b);
void normalize3f(vec3f *c);

#endif // TYPEDEFS_H
