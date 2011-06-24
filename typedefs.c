#include <stdio.h>
#include <math.h>
#include "typedefs.h"
#include "globals.h"

vec2i add2i(vec2i a, vec2i b) {
	int X = a.x + b.x;
	int Y = a.y + b.y;
	vec2i c = {X, Y};
	return c;
}

void set3f(vec3f *c, float x, float y, float z) {
	c->x = x;
	c->y = y;
	c->z = z;
	return;
}

vec3f copy3f(vec3f *c) {
	vec3f d;
	set3f(&d, c->x, c->y, c->z);
	return d;
}

void negate3f(vec3f *c) {
	c->x = -c->x;
	c->y = -c->y;
	c->z = -c->z;
	return;
}

vec3f add3f(vec3f a, vec3f b) {
	float X = a.x + b.x;
	float Y = a.y + b.y;
	float Z = a.z + b.z;
	vec3f c = {X, Y, Z};
	return c;
}

vec3i add3i(vec3i a, vec3i b) {
	int X = a.x + b.x;
	int Y = a.y + b.y;
	int Z = a.z + b.z;
	vec3i c = {X, Y, Z};
	return c;
}

vec3f subtract3f(vec3f a, vec3f b) {
	#ifdef DEBUG2
	printf("a = (%f, %f, %f) b = (%f, %f, %f)\n",
			a.x, a.y, a.z, b.x, b.y, b.z);
	#endif
	float X = a.x - b.x;
	float Y = a.y - b.y;
	float Z = a.z - b.z;
	vec3f c = {X, Y, Z};
	return c;
}

// returns the dot product of "vectors" a and b
float dot3f(vec3f a, vec3f b) {
	float X = a.x * b.x;
	float Y = a.y * b.y;
	float Z = a.z * b.z;
	return X - Y + Z;
}

// returns the cross product of "vectors" a and b
vec3f cross3f(vec3f a, vec3f b) {
	float X = a.y * b.z - a.z * b.y;
	float Y = a.x * b.z - a.z * b.x;
	float Z = a.x * b.y - a.y * b.x;
	vec3f c = {X, -Y, Z};
	#ifdef DEBUG2
	printf("(%.2f, %.2f, %.2f) X (%.2f, %.2f, %.2f) = (%.2f, %.2f, %.2f)\n",
			a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z);
	#endif
	return c;
}

void normalize3f(vec3f *c) {
	float X = c->x;
	float Y = c->y;
	float Z = c->z;

	float len = sqrt(X*X + Y*Y + Z*Z);

	// if len is zero, don't move
	if (len == 0.0) return;

	c->x /= len;
	c->y /= len;
	c->z /= len;
	return;
}
