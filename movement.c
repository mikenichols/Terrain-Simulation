#include <stdio.h>
#include <math.h>
#include "movement.h"
#include "globals.h"
#include "typedefs.h"

// axis defines the direction of movement
void move(direction_t dir, vec3f axis) {
	int dir_mod;

	if (dir == DIR_UP || dir == DIR_LEFT || dir == DIR_FORWARD) dir_mod = 1;
	else if (dir == DIR_DOWN || dir == DIR_RIGHT || dir == DIR_BACKWARD) dir_mod = -1;
	else unreachable(__FILE__, __LINE__);

	normalize3f(&axis);

	float X = MOVE_SCALE * dir_mod * axis.x;
	float Y = MOVE_SCALE * dir_mod * axis.y;
	float Z = MOVE_SCALE * dir_mod * axis.z;

	eye.x += X;
	eye.y += Y;
	eye.z += Z;

	return;
}

// axis defines the axis of rotation
// TODO: add support for rolling and pitching
void rotate(direction_t dir, vec3f axis) {
	int dir_mod;

	if (dir == DIR_UP || dir == DIR_LEFT || dir == DIR_FORWARD) dir_mod = -1;
	else if (dir == DIR_DOWN || dir == DIR_RIGHT || dir == DIR_BACKWARD) dir_mod = 1;
	else unreachable(__FILE__, __LINE__);

	y_rot += dir_mod * rotate_step;
	while (y_rot >= 2 * PI) y_rot -= 2 * PI;

	look.x = -cos(y_rot);
	look.y = sin(y_rot);

	return;
}
