#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "typedefs.h"

// axis defines the direction of movement
void move(direction_t dir, vec3f axis);

// axis defines the axis of rotation
void rotate(direction_t dir, vec3f axis);

#endif //MOVEMENT_H
